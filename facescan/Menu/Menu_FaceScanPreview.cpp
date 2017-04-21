/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "Menu_FaceScanPreview.h"
#include "MenuController.h"
#include "CPUTGuiControllerDX11.h"
#include "MenuGlob.h"
#include "CPUTModel.h"
#include "CPUTMesh.h"
#include "CPUTTextureDX11.h"
#include "CPUTMaterial.h"
#include "../ObjLoader.h"
#include "../CFaceModel.h"
#include <time.h>
#include "../GameFaceScan.h"

enum MainMenuIds
{
	MainMenuIds_Accept = 0,
	MainMenuIds_Retry,
	MainMenuIds_LandmarkDisplayAll,
	MainMenuIds_Back
};

Menu_FaceScanPreview::Menu_FaceScanPreview()
{
	mMode = FaceScanPreviewMode_ApproveScan;
	displayModel = NULL;
	cameraController = new CPUTCameraModelViewer();
	cameraController->SetTarget(float3(0,0,0));
	cameraController->SetDistance(10, 3, 20);
	cameraController->SetViewAngles(0,0);
}

Menu_FaceScanPreview::~Menu_FaceScanPreview()
{
	SAFE_DELETE(cameraController);
}

void Menu_FaceScanPreview::Init()
{
	MenuBase::Init();
}

void Menu_FaceScanPreview::SetFaceScanMode(FaceScanPreviewMode mode)
{
	mMode = mode;
}

void Menu_FaceScanPreview::ActivationChanged(bool active)
{
	MenuBase::ActivationChanged(active);
	if (active)
	{
		CPUTGuiController *pGUI = MenuGlob_GUI();

		if (mMode == FaceScanPreviewMode_ApproveScan)
		{
			pGUI->CreateButton("Accept Scan", MainMenuIds_Accept, MENU_CPUT_PANEL_ID);
			pGUI->CreateButton("Retry Scan", MainMenuIds_Retry, MENU_CPUT_PANEL_ID);
		}
		else
		{
			pGUI->CreateButton("Back", MainMenuIds_Back, MENU_CPUT_PANEL_ID);
		}
		pGUI->CreateCheckbox("Landmarks", MainMenuIds_LandmarkDisplayAll, MENU_CPUT_PANEL_ID, &mLandmarkCheckbox);
	}
	else
	{
		mLandmarkCheckbox = NULL;
	}
}

void Menu_FaceScanPreview::HandleCPUTEvent(int eventID, int controlID, CPUTControl *control)
{
	if (eventID == CPUT_EVENT_DOWN)
	{
		switch (controlID)
		{
		case MainMenuIds_Retry:
		{
			FaceScan_MoveScanData(mModelFilename.c_str(), NULL);
			MenuController_PopTo(&gMenu_Scan);
		} break;
		case MainMenuIds_Accept:
		{
			char buffer[256];
			time_t rawtime;
			struct tm * timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(buffer, sizeof(buffer), "%b_%d_%Y_%H_%M_%S", timeinfo);
			std::string modelName = std::string("model_") + buffer + ".obj";

			OPENFILENAMEA ofn = { 0 };
			char ofnFilename[MAX_PATH];
			strncpy(ofnFilename, modelName.c_str(), MAX_PATH);

			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrTitle = "Save Scanned Face";
			ofn.hwndOwner = NULL;
			ofn.lpstrFilter = "Obj Files (*.obj)\0*.obj\0";
			ofn.lpstrFile = ofnFilename;
			ofn.nMaxFile = sizeof(ofnFilename);
			ofn.Flags = OFN_EXPLORER;
			ofn.lpstrDefExt = "obj";

			if (TRUE == GetSaveFileNameA(&ofn))
			{
				FaceScan_MoveScanData(mModelFilename.c_str(), ofnFilename);
			}

			gMenu_FaceMapping.LoadFace(ofnFilename);
			MenuController_PushMenu(&gMenu_FaceMapping, true);
		} break;
		case MainMenuIds_Back:
		{
			MenuController_PopMenu(true);
		} break;
		}
	}
}

CPUTEventHandledCode Menu_FaceScanPreview::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message)
{
	return cameraController->HandleMouseEvent(x, y, wheel, state, message);
}

void Menu_FaceScanPreview::Update(float dt)
{
	cameraController->Update(dt);
}

void Menu_FaceScanPreview::Render( CPUTRenderParameters &renderParams )
{
	renderParams.mpCamera = (CPUTCamera*)cameraController->GetCamera();
	if (displayModel != NULL)
	{
		displayModel->Render(renderParams, 0);

		if (mLandmarkCheckbox->GetCheckboxState() == CPUT_CHECKBOX_CHECKED)
		{
			for (auto it = mFaceModel.Landmarks.begin(); it != mFaceModel.Landmarks.end(); it++) 
			{
				CPUTColor4 color = CPUTColor4(1.0f, 1.0f, 0.0f, 1.0f);
				DrawCube(renderParams, it->second, 0.1f, color);
			}
		}
	}
}

void Menu_FaceScanPreview::LoadFaceObj( std::string filename, bool absoluteFilename, bool forceReload )
{
	ProfileBlockScoped block("Load OBJ");
	if (!absoluteFilename)
	{
		CPUTFileSystem::CombinePath(GetUserDataDirectory(), filename, &filename );
	}

	if (filename == mModelFilename && !forceReload)
		return;
	
	mFaceModel.LoadObjFilename(filename);
	mModelFilename = filename;

	CPUTTexture *texture;
	SAFE_RELEASE(displayModel);
	displayModel = mFaceModel.LoadCPUTModel(&texture);

	CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
	pAssetLibrary->SetRootRelativeMediaDirectory("MyAssets");
	std::string matName = pAssetLibrary->GetMaterialDirectoryName();
	CPUTFileSystem::CombinePath(matName, "meshpreview.mtl", &matName);

	CPUTMaterial *material = CPUTMaterial::Create(matName);
	material->OverridePSTexture(0, texture);
	displayModel->SetMaterial(0, &material, 1);
}