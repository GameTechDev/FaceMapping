/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "CFaceModel.h"
#include "CPUTModel.h"
#include "CPUTMesh.h"
#include "CPUTMaterial.h"
#include "SampleUtil.h"
#include "CPUTTexture.h"

CFaceModel::CFaceModel() :
mObjFilename("")
{

}

CFaceModel::~CFaceModel()
{
}

void CFaceModel::LoadObjFilename(const std::string &filename, bool landmarks)
{
	ProfileBlockScoped block("Load OBJ");

	AABBMin = float3(10000.0f, 10000.0f, 10000.0f);
	AABBMax = -AABBMin;

	mObjFilename = filename;
	mObjModel.m_indices.clear();
	mObjModel.m_segments.clear();
	mObjModel.m_vertices.clear();

	objLoader(filename.c_str(), mObjModel);

	float boxSize = 8.0f;
	if (mObjModel.m_vertices.size() > 0)
	{
		float3 vmin = float3(mObjModel.m_vertices[0].x, mObjModel.m_vertices[0].y, mObjModel.m_vertices[0].z);
		float3 vmax = float3(mObjModel.m_vertices[0].x, mObjModel.m_vertices[0].y, mObjModel.m_vertices[0].z);

		for (std::vector<tVertex>::const_iterator it = mObjModel.m_vertices.begin(); it != mObjModel.m_vertices.end(); ++it)
		{
			vmin.x = floatMin(vmin.x, it->x);
			vmin.y = floatMin(vmin.y, it->y);
			vmin.z = floatMin(vmin.z, it->z);
			vmax.x = floatMax(vmax.x, it->x);
			vmax.y = floatMax(vmax.y, it->y);
			vmax.z = floatMax(vmax.z, it->z);
		}

		float3 center = (vmax + vmin) / 2.0f;
		float3 dim = vmax - vmin;

		float maxDim = floatMax(dim.x, floatMax(dim.y, dim.z));

		mVertOffset = -center;
		mVertScale = boxSize / maxDim;

		// center to origin, scale, and rotate
		// 
		int vertCount = (int)mObjModel.m_vertices.size();
		for (int i = 0; i < vertCount; i++)
		{
			tVertex *it = &mObjModel.m_vertices[i];
			float3 pos = float3(it->x, it->y, it->z);
			pos -= center;
			pos *= mVertScale;

			// save scale and rotate from RSSDK format. Z is up in rssdk
			it->x = pos.x;
			it->y = pos.z;
			it->z = pos.y;
			it->v = 1.0f - it->v;

			AABBMin.x = floatMin(it->x, AABBMin.x);
			AABBMin.y = floatMin(it->y, AABBMin.y);
			AABBMin.z = floatMin(it->z, AABBMin.z);
			AABBMax.x = floatMax(it->x, AABBMax.x);
			AABBMax.y = floatMax(it->y, AABBMax.y);
			AABBMax.z = floatMax(it->z, AABBMax.z);

		}
	}
	LoadLandmarks();
}

CPUTModel *CFaceModel::LoadCPUTModel( CPUTTexture **outColorImage )
{
	ProfileBlockScoped block("LoadCPUTModel");
	
	CPUTModel *model = CPUTModel::Create();
	CPUTMesh *mesh = CPUTMesh::Create();
	CPUTBufferElementInfo pVertexInfo[] = {
			{ "POSITION", 0, 0, CPUT_F32, 3, 3 * sizeof(float), 0 },
			{ "TEXCOORD", 0, 1, CPUT_F32, 2, 2 * sizeof(float), 3 * sizeof(float) },
			{ "NORMAL", 0, 2, CPUT_F32, 3, 3 * sizeof(float), 5 * sizeof(float) },
	};
	static int size = sizeof(tVertex);
	CPUTBufferElementInfo indexElementInfo = { "POSITION", 0, 0, CPUT_U32, 1, sizeof(ObjIndexInt), 0 };
	mesh->SetMeshTopology(CPUT_TOPOLOGY_INDEXED_TRIANGLE_LIST);
	mesh->CreateNativeResources(model, 0, 3, pVertexInfo, (int)mObjModel.m_vertices.size(), &mObjModel.m_vertices[0], NULL, 0, NULL);

	model->mpMaterialCount = new int[1];
	model->mpMaterial = new CPUTMaterial**[1];
	model->mpMaterial[0] = new CPUTMaterial*[1];
	model->mpMaterial[0][0] = NULL;
	model->mpMaterialCount[0] = 1;

	if (outColorImage != NULL)
	{
		std::string fnString = mObjFilename;
		int lastindex = (int)fnString.find_last_of(".");
		std::string rawname = fnString.substr(0, lastindex);
		std::string textureName = rawname.append("image1.png");
		*outColorImage = CPUTTexture::Create(std::string("facetexture"), textureName, false);
	}
	
	model->SetMeshes(&mesh, 1);

	return model;
}

bool CFaceModel::LoadLandmarks(const std::string &filename, std::map<std::string, float3> &outMap, int formatVersion)
{
	char line[1024];
	
	FILE *file = fopen(filename.c_str(), "r");

	if (file == NULL)
		return false;
	
	char toRemove[] = { '\t', ' ', '{', '}', '[', ']', '\"', '\n', ',', 'x', 'y', 'z' };
	const char *toRemoveStrings[] = { "Name", "Confidence", "Position" };
	while (fgets(line, sizeof(line), file))
	{
		bool hasConfidence = strstr(line, "Confidence") != NULL;
		for (int i = 0; i < ARRAYSIZE(toRemoveStrings); i++)
		{
			char *loc = line;
			while (loc = strstr(loc, toRemoveStrings[i]))
			{
				int toRemoveLen = (int)strlen(toRemoveStrings[i]);
				memmove(loc, loc + toRemoveLen, 1 + strlen(loc + toRemoveLen));
			}
		}
		{
			int writeLoc = 0;
			char sLine[1024];
			
			for (int i = 0; i < 1024 && line[i] != 0; i++)
			{
				char c = line[i];
				bool skip = false;
				for (int j = 0; j < ARRAYSIZE(toRemove); j++)
				{
					if (toRemove[j] == c)
						skip = true;
				}
				if (c == ':')
					c = ' ';
				if (!skip)
					sLine[writeLoc++] = c;
			}
			sLine[writeLoc] = 0;

			char landmarkName[256];
			float3 pos;
			int confidence = 100;
			bool scanValid = true;
			if (hasConfidence)
			{
				int result = sscanf_s(sLine, "%s %d %f %f %f", landmarkName, sizeof(landmarkName), &confidence, &pos.x, &pos.y, &pos.z);
				scanValid = result == 5;
			}
			else
			{
				int result = sscanf_s(sLine, "%s %f %f %f", landmarkName, sizeof(landmarkName), &pos.x, &pos.y, &pos.z);
				scanValid = result == 4;
			}
			
			if (scanValid)
			{
				pos += mVertOffset;
				pos *= mVertScale;
				floatSwap(&pos.y, &pos.z);
				outMap[std::string(landmarkName)] = pos;
			}
		}
	}
	fclose(file);

	// Based on the left/right eye position print out suggested roll and yaw
	if (outMap.find("LANDMARK_EYE_LEFT_CENTER") != outMap.end() && outMap.find("LANDMARK_EYE_RIGHT_CENTER") != outMap.end())
	{
		float3 leftEye = outMap["LANDMARK_EYE_LEFT_CENTER"];
		float3 rightEye = outMap["LANDMARK_EYE_RIGHT_CENTER"];
		float3 delta = (leftEye - rightEye);
		float distance = delta.length();
		float xDistance = abs((rightEye.x - leftEye.x));
		float yDistance = abs((rightEye.y - leftEye.y));
		float zDistance = abs((rightEye.z - leftEye.z));
		float suggestedYaw = asin(zDistance / distance);
		float suggestedRoll = asin(yDistance / distance);
		DebugPrintf("Landmark Suggested Yaw: %0.2f\n", RadToDeg( suggestedYaw));
		DebugPrintf("Landmark Suggested Roll: %0.2f\n", RadToDeg(suggestedRoll));
	}
	


	return outMap.size() != 0;
}

bool CFaceModel::LoadLandmarks()
{
	Landmarks.clear();
	std::string fnString = mObjFilename;
	int lastindex = (int)fnString.find_last_of(".");
	std::string rawname = fnString.substr(0, lastindex);
	std::string jsonName = rawname.append(".json");

	if (LoadLandmarks(jsonName, Landmarks, 0))
		return true;
	return LoadLandmarks(jsonName, Landmarks, 1);
}

