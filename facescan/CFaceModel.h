/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __OBJ_MODEL__
#define __OBJ_MODEL__

#include "ObjLoader.h"
#include <map>
#include "CPUTMath.h"

class CPUTModel;
class CPUTTexture;

class CFaceModel
{
public:

	CFaceModel();
	~CFaceModel();

	void LoadObjFilename(const std::string &filename, bool landmarks = true);
	CPUTModel *LoadCPUTModel(CPUTTexture **outColorTexture);

	bool LoadLandmarks();

	std::map<std::string, float3> Landmarks;

	float3 AABBMin;
	float3 AABBMax;

private:
	std::string mObjFilename;
	tObjModel mObjModel;

	// values that were applied to the objmodel after it was loaded
	// in an effort to normalize the mesh
	float mVertScale;
	float3 mVertOffset;
	
	bool LoadLandmarks( const std::string &filename, std::map<std::string, float3> &outMap, int formatVersion = 0 );
	
};

#endif // __OBJ_MODEL__