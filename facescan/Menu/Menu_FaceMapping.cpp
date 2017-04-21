/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "Menu_FaceMapping.h"
#include "MenuController.h"
#include "CPUTGuiControllerDX11.h"
#include "MenuGlob.h"
#include "CPUTModel.h"
#include "CPUTMaterial.h"
#include "CPUTSprite.h"
#include "CPUTScene.h"
#include "CPUT_DX11.h"
#include "../SampleStart.h"
#include <string>
#include "UVRemapUtils.h"
#include "CPUTVertexShaderDX11.h"
#include "CPUTGeometryShaderDX11.h"

const UINT  FACE_DISPLACEMENT_WIDTH_HEIGHT = 1024;

enum FaceMappingMenuId
{
	FaceMappingMenuId_LoadScan = 0,
	FaceMappingMenuId_ScanNewFace,
	FaceMappingMenuId_ShowRSMesh,
	FaceMappingMenuId_SliderScale,
	FaceMappingMenuId_SliderYaw,
	FaceMappingMenuId_SliderPitch,
	FaceMappingMenuId_SliderRoll,
	FaceMappingMenuId_SliderPhysicalDisplaceOffset,
	FaceMappingMenuId_BaseColor,
	FaceMappingMenuId_BaseColor2,
	FaceMappingMenuId_ResetTweaks,
	FaceMappingMenuId_SaveSculptedObj,
	FaceMappingMenuId_SliderHairType
};

void SetDefaultTweaks(MappingTweaks *tweaks)
{
	tweaks->Scale = 1.2f;
	tweaks->FaceYaw = 0.0f;
	tweaks->FacePitch = 0.0f;
	tweaks->FaceRoll = 0.0f;
	tweaks->DisplaceOffset = float3(0.0f, 0.0f, 0.0f);
	tweaks->BlendColorRGB[0] = 228;
	tweaks->BlendColorRGB[1] = 194;
	tweaks->BlendColorRGB[2] = 171;
	tweaks->BlendColor2RGB[0] = 205;
	tweaks->BlendColor2RGB[1] = 50;
	tweaks->BlendColor2RGB[2] = 50;
}

void Menu_FaceMapping::UpdateGUIFromTweaks()
{
	char text[1024];
	
	mScalingSlider->SetValue(mTweaks.Scale);
	sprintf(text, "Scaling (%.2f)", mScalingSlider->GetValue());
	mScalingSlider->SetText(std::string(text));

	mSlider_FaceYaw->SetValue(mTweaks.FaceYaw);
	sprintf(text, "Yaw (%d degrees)", (int)mSlider_FaceYaw->GetValue());
	mSlider_FaceYaw->SetText(std::string(text));

	mSlider_FacePitch->SetValue(mTweaks.FacePitch);
	sprintf(text, "Pitch (%d degrees)", (int)mSlider_FacePitch->GetValue());
	mSlider_FacePitch->SetText(std::string(text));

	mSlider_FaceRoll->SetValue(mTweaks.FaceRoll);
	sprintf(text, "Roll (%d degrees)", (int)mSlider_FaceRoll->GetValue());
	mSlider_FaceRoll->SetText(std::string(text));

	mSlider_FacePhysicalDisplaceOffsetZ->SetValue(mTweaks.DisplaceOffset.z);
	sprintf(text, "Z Displace (%.2f)", mSlider_FacePhysicalDisplaceOffsetZ->GetValue());
	mSlider_FacePhysicalDisplaceOffsetZ->SetText(std::string(text));
}

void Menu_FaceMapping::UpdateTweaksFromGUI()
{
	mTweaks.Scale = mScalingSlider->GetValue();
	mTweaks.FaceYaw = mSlider_FaceYaw->GetValue();
	mTweaks.FacePitch = mSlider_FacePitch->GetValue();
	mTweaks.FaceRoll = mSlider_FaceRoll->GetValue();
	mTweaks.DisplaceOffset.z = mSlider_FacePhysicalDisplaceOffsetZ->GetValue();
}


bool GetCPUTLandmark(std::string &name, float3 *position)
{
	std::string nodeName = std::string(".landmarks") + name;
	CPUTNullNode *node = CPUTAssetLibrary::GetAssetLibrary()->FindNullNode( nodeName, true);
	*position = (node != NULL) ? node->GetPosition() : float3(0, 0, 0);
	return node != NULL;
}

void Menu_FaceMapping::Init()
{
	MenuBase::Init();

	SetDefaultTweaks(&mTweaks);

	CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

	mpFaceMapRTColor = CPUTRenderTargetColor::Create();
	mpFaceMapRTColor->CreateRenderTarget("$FaceColor", FACE_DISPLACEMENT_WIDTH_HEIGHT, FACE_DISPLACEMENT_WIDTH_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	mpFaceMapRTDepth = CPUTRenderTargetColor::Create();
	mpFaceMapRTDepth->CreateRenderTarget("$FaceDisplacement", FACE_DISPLACEMENT_WIDTH_HEIGHT, FACE_DISPLACEMENT_WIDTH_HEIGHT, DXGI_FORMAT_R32_FLOAT);

	mpFaceMapDepthBuffer = CPUTRenderTargetDepth::Create();
	mpFaceMapDepthBuffer->CreateRenderTarget("$FaceDisplacementDepth", FACE_DISPLACEMENT_WIDTH_HEIGHT, FACE_DISPLACEMENT_WIDTH_HEIGHT, DXGI_FORMAT_D32_FLOAT);

	mDisplacementCamera = CPUTCamera::Create(CPUT_ORTHOGRAPHIC);
	mDisplacementCamera->SetPosition(0.0f, 0.0f, -10.0f);
	mDisplacementCamera->LookAt( 0.0f, 0.0f, 0.0f );
	
	mDisplacementCamera->SetFarPlaneDistance(20);
	mDisplacementCamera->SetNearPlaneDistance(1.0f);
	mDisplacementCamera->SetAspectRatio(1.0f);
	mDisplacementCamera->SetWidth(10);
	mDisplacementCamera->SetHeight(10);
	mDisplacementCamera->Update();
	
	pAssetLibrary->SetRootRelativeMediaDirectory("MyAssets");
	CPUTMaterial* mat = pAssetLibrary->GetMaterial("render_map_diffuse");
	mMapSprites[FaceMapTexture_Color] = CPUTSprite::Create(-1.0f, -1.0f, 0.5f, 0.5f, mat);
	SAFE_RELEASE(mat);
	mat = pAssetLibrary->GetMaterial("render_map_depth");
	mMapSprites[FaceMapTexture_Depth] = CPUTSprite::Create(-1.0f, -0.4f, 0.5f, 0.5f, mat);
	SAFE_RELEASE(mat);

	// load the head model
	std::string mediaFilename;
	CPUTFileSystem::GetMediaDirectory(&mediaFilename);
	pAssetLibrary->SetMediaDirectoryName(mediaFilename+"\\");
	CPUTFileSystem::CombinePath(mediaFilename, "basicHead_03.scene", &mediaFilename);
	mScene = CPUTScene::Create();
	mScene->LoadScene(mediaFilename);
	mHeadModel = pAssetLibrary->FindModel("head4", true);

	CPUTAssetSet *hairSetMale = pAssetLibrary->FindAssetByNameNoPath("hairMale_01.set", CPUTAssetLibrary::mpAssetSetList);
	CPUTAssetSet *hairSetMale_Black = pAssetLibrary->FindAssetByNameNoPath("hairMale_01_black.set", CPUTAssetLibrary::mpAssetSetList);
	CPUTAssetSet *hairSetFemale = pAssetLibrary->FindAssetByNameNoPath("hairGeekGirl.set", CPUTAssetLibrary::mpAssetSetList);
	CPUTAssetSet *hairSetFemale_Blonde = pAssetLibrary->FindAssetByNameNoPath("hairGeekGirl_blonde.set", CPUTAssetLibrary::mpAssetSetList);
	CPUTAssetSet *hairSetFemale_Black = pAssetLibrary->FindAssetByNameNoPath("hairGeekGirl_black.set", CPUTAssetLibrary::mpAssetSetList);
	CPUTAssetSet *hairSetCrazy = pAssetLibrary->FindAssetByNameNoPath("hair_01.set", CPUTAssetLibrary::mpAssetSetList);

	HairInfo info;
	info.name = "Hair Short";
	info.set = hairSetMale;
	mHairList.push_back(info);

	info.name = "Hair Short Black";
	info.set = hairSetMale_Black;
	mHairList.push_back(info);

	info.name = "Hair Long";
	info.set = hairSetFemale;
	mHairList.push_back(info);

	info.name = "Hair Long Blonde";
	info.set = hairSetFemale_Blonde;
	mHairList.push_back(info);

	info.name = "Hair Long Black";
	info.set = hairSetFemale_Black;
	mHairList.push_back(info);

	info.name = "Madness";
	info.set = hairSetCrazy;
	mHairList.push_back(info);


	GetCPUTLandmark( std::string( "LANDMARK_EYE_LEFT_CENTER" ), &mHeadLandmark_LeftEyeCenter);
	GetCPUTLandmark( std::string( "LANDMARK_EYE_RIGHT_CENTER" ), &mHeadLandmark_RightEyeCenter);
	GetCPUTLandmark(std::string("LANDMARK_NOSE_TIP"), &mHeadLandmark_NoseTip);

	mCameraController = new CPUTCameraModelViewer();
	mCameraController->SetTarget(float3(0, 0, 0));
	mCameraController->SetDistance( 20, 10, 40);
	mCameraController->SetViewAngles(0, 0);

	mMapProjectionCamera = CPUTCamera::Create(CPUT_ORTHOGRAPHIC);
	float y = 5.0f;
	mMapProjectionCamera->SetPosition(0.0f, y, -10.0f );
	mMapProjectionCamera->LookAt(0.0f, y, 0.0f);

	mMapProjectionCamera->SetFarPlaneDistance(100);
	mMapProjectionCamera->SetNearPlaneDistance(0.1f);
	mMapProjectionCamera->SetAspectRatio(1.0f);
	mMapProjectionCamera->SetWidth(15);
	mMapProjectionCamera->SetHeight(15);
	mMapProjectionCamera->Update();

    mSaveSculptedObj = false;
}

void Menu_FaceMapping::Shutdown()
{
	SAFE_DELETE(mpFaceMapRTColor);
	SAFE_DELETE(mpFaceMapRTDepth);
	SAFE_DELETE(mpFaceMapRTDepth);
	SAFE_DELETE(mpFaceMapDepthBuffer);
	SAFE_DELETE(mScene);
}

void Menu_FaceMapping::LoadFace(const std::string &filename)
{
	mObjFilename = filename;
	mFaceModel.LoadObjFilename(filename);

	CPUTTexture *texture;
	mFaceCPUTModel = mFaceModel.LoadCPUTModel(&texture);

	CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
	pAssetLibrary->SetRootRelativeMediaDirectory("MyAssets");
	std::string matName = pAssetLibrary->GetMaterialDirectoryName();
	CPUTFileSystem::CombinePath(matName, "displace_map_render.mtl", &matName);

	CPUTMaterial *material = CPUTMaterial::Create(matName);
	material->OverridePSTexture(0, texture);
	mFaceCPUTModel->SetMaterial(0, &material, 1);
}

void Menu_FaceMapping::UpdateHairModels()
{
	float sliderValue = mSlider_HairType->GetValue();
	int activeSetIndex = (int)sliderValue;
	for (int i = 0; i < (int)mHairList.size(); i++)
	{
		if (mHairList[i].set != NULL)
		{
			mHairList[i].set->IsEnabled = activeSetIndex == i;
		}
	}
}

void Menu_FaceMapping::ActivationChanged(bool active)
{
	MenuBase::ActivationChanged(active);
	if (active)
	{
		CPUTGuiController *pGUI = MenuGlob_GUI();
		pGUI->CreateButton("Load Scan", FaceMappingMenuId_LoadScan, MENU_CPUT_PANEL_ID);
		pGUI->CreateButton("New Scan", FaceMappingMenuId_ScanNewFace, MENU_CPUT_PANEL_ID);
		pGUI->CreateButton("Save Sculpted Obj", FaceMappingMenuId_SaveSculptedObj, MENU_CPUT_PANEL_ID);
		pGUI->CreateButton("Show RS Mesh", FaceMappingMenuId_ShowRSMesh, MENU_CPUT_PANEL_ID);
		pGUI->CreateSlider("Scaling", FaceMappingMenuId_SliderScale, MENU_CPUT_PANEL_ID, &mScalingSlider, 1.0f);
		
		pGUI->CreateSlider("Yaw", FaceMappingMenuId_SliderYaw, MENU_CPUT_PANEL_ID, &mSlider_FaceYaw, 1.0f);
		pGUI->CreateSlider("Pitch", FaceMappingMenuId_SliderPitch, MENU_CPUT_PANEL_ID, &mSlider_FacePitch, 1.0f);
		pGUI->CreateSlider("Roll", FaceMappingMenuId_SliderRoll, MENU_CPUT_PANEL_ID, &mSlider_FaceRoll, 1.0f);
		pGUI->CreateSlider("Z Displace", FaceMappingMenuId_SliderPhysicalDisplaceOffset, MENU_CPUT_PANEL_ID, &mSlider_FacePhysicalDisplaceOffsetZ, 1.0f);
		
		pGUI->CreateButton("Select Color", FaceMappingMenuId_BaseColor, MENU_CPUT_PANEL_ID);
		pGUI->CreateButton("Select Color 2", FaceMappingMenuId_BaseColor2, MENU_CPUT_PANEL_ID);

		pGUI->CreateSlider("Hair Style", FaceMappingMenuId_SliderHairType, MENU_CPUT_PANEL_ID, &mSlider_HairType, 1.0f);
		mSlider_HairType->SetScale(0, (float)mHairList.size(), (int)mHairList.size()+1);

		pGUI->CreateButton("Reset", FaceMappingMenuId_ResetTweaks, MENU_CPUT_PANEL_ID);
		
		mScalingSlider->SetScale(0.5f, 1.5f, 21);

		mSlider_FaceYaw->SetScale(-20, 20, 41);
		mSlider_FacePitch->SetScale(-20, 20, 41);
		mSlider_FaceRoll->SetScale(-20, 20, 41);

		mSlider_FacePhysicalDisplaceOffsetZ->SetScale(-1, 1, 21);

		UpdateGUIFromTweaks();
	}
}

void Menu_FaceMapping::HandleCPUTEvent(int eventID, int controlID, CPUTControl *control)
{
	switch (controlID)
	{
	case FaceMappingMenuId_LoadScan:
	{
		OPENFILENAMEA ofn;
		char filename[512];
		memset(&ofn, 0, sizeof(OPENFILENAMEA));
		filename[0] = 0;
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = MySample::Instance->GetHWnd();
		ofn.lpstrDefExt = ".obj";
		ofn.lpstrFilter = "Obj Files(*.obj)\0*.obj\0\0";
		std::string cwd = GetUserDataDirectory();
		ofn.lpstrInitialDir = cwd.c_str();
		ofn.lpstrTitle = "Select An DDS File";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);
		if (GetOpenFileNameA(&ofn))
		{
			LoadFace(std::string(ofn.lpstrFile));
		}
		
	} break;
	case FaceMappingMenuId_ScanNewFace:
	{
		MenuController_PopTo(&gMenu_Scan, true);
	} break;
	case FaceMappingMenuId_SaveSculptedObj:
	{
        mSaveSculptedObj = true;

	} break;
	case FaceMappingMenuId_ShowRSMesh:
	{
		if (IsFaceLoaded())
		{
			gMenu_FaceScanPreview.LoadFaceObj(mObjFilename, true);
			gMenu_FaceScanPreview.SetFaceScanMode(FaceScanPReviewMode_ViewScan);
			MenuController_PushMenu(&gMenu_FaceScanPreview);
		}
	} break;
	case FaceMappingMenuId_SliderPitch:
	case FaceMappingMenuId_SliderYaw:
	case FaceMappingMenuId_SliderScale:
	case FaceMappingMenuId_SliderRoll:
	case FaceMappingMenuId_SliderPhysicalDisplaceOffset:
	{
		UpdateTweaksFromGUI();
		UpdateGUIFromTweaks();
	} break;
	case FaceMappingMenuId_BaseColor:
	case FaceMappingMenuId_BaseColor2:
	{
		byte *colors = (controlID == FaceMappingMenuId_BaseColor) ? &mTweaks.BlendColorRGB[0] : &mTweaks.BlendColor2RGB[0];
		CHOOSECOLOR cc = {};
		COLORREF customColors[16];
		cc.lStructSize = sizeof(CHOOSECOLOR);
		cc.hwndOwner = MySample::Instance->GetHWnd();
		cc.rgbResult = RGB(colors[0], colors[1], colors[2]);
		customColors[0] = RGB(228, 194, 171);
		customColors[1] = RGB(205, 50, 50);
		cc.lpCustColors = customColors;
		cc.Flags = CC_RGBINIT | CC_FULLOPEN;
		if (ChooseColor(&cc))
		{
			colors[0] = (byte)(cc.rgbResult & 0x000000ff);
			colors[1] = (byte)((cc.rgbResult & 0x0000ff00) >> 8);
			colors[2] = (byte)((cc.rgbResult & 0x00ff0000) >> 16);
		}
	} break;
	case FaceMappingMenuId_ResetTweaks:
	{
		SetDefaultTweaks(&mTweaks);
		UpdateGUIFromTweaks();
	} break;
	}
}

CPUTEventHandledCode Menu_FaceMapping::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message)
{
	return mCameraController->HandleMouseEvent(x, y, wheel, state, message);
}

void Menu_FaceMapping::Update(float dt)
{
	UpdateHairModels();
	mCameraController->Update(dt);
}

void Menu_FaceMapping::CreateFaceMaps(CPUTRenderParameters &renderParams)
{
	CPUTCamera *pLastCamera = renderParams.mpCamera;

	renderParams.mpCamera = mDisplacementCamera;
	float pBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	// Render the displacement map - set it here only so we can use the restore feature at the end of this file
	mpFaceMapRTDepth->SetRenderTarget(renderParams, mpFaceMapDepthBuffer, 0, pBlack, true);

	// CPUT doesn't support MRT, go directly to DX
	ID3D11RenderTargetView *rtvs[2];
	rtvs[0] = mpFaceMapRTColor->GetColorBufferView();
	rtvs[1] = mpFaceMapRTDepth->GetColorBufferView();

	float3 minFaceZone = mFaceModel.AABBMin;
	float3 maxFaceZone = mFaceModel.AABBMax;

	float nearClippingPlaneOffset = 2.0f;
	
	mFaceMapInfo.DepthMap_ZRange = (maxFaceZone.z - minFaceZone.z) + nearClippingPlaneOffset;
	mFaceMapInfo.DepthMap_ZMeshStart = 1.0f - (nearClippingPlaneOffset / mFaceMapInfo.DepthMap_ZRange);
	float cameraDistance = 10.0f;
	float nearPlane = cameraDistance + minFaceZone.z - nearClippingPlaneOffset;
	
	mDisplacementCamera->SetPosition(0.0f, 0.0f, -cameraDistance);
	mDisplacementCamera->LookAt(0.0f, 0.0f, 0.0f);
	mDisplacementCamera->SetNearPlaneDistance(nearPlane);
	mDisplacementCamera->SetFarPlaneDistance(nearPlane + mFaceMapInfo.DepthMap_ZRange);
	mDisplacementCamera->Update();

	CPUT_DX11::GetContext()->OMSetRenderTargets(2, rtvs, mpFaceMapDepthBuffer->GetDepthBufferView());

	float4x4 modelMatrix = float4x4Identity();
	modelMatrix = modelMatrix * float4x4RotationX(DegToRad(mTweaks.FacePitch));
	modelMatrix = modelMatrix * float4x4RotationY(DegToRad(mTweaks.FaceYaw));
	modelMatrix = modelMatrix * float4x4RotationZ(DegToRad(mTweaks.FaceRoll));
	mFaceCPUTModel->SetParentMatrix(modelMatrix);

	mFaceCPUTModel->Render(renderParams, 0);
	mpFaceMapRTDepth->RestoreRenderTarget(renderParams);
	
	// calculate mapping details
	float4x4 displacementMapMatrix = *mDisplacementCamera->GetViewMatrix() * *mDisplacementCamera->GetProjectionMatrix();
		
	float4 leftEye = float4(mFaceModel.Landmarks["LANDMARK_EYE_LEFT_CENTER"], 1.0f) * modelMatrix;
	float4 rightEye = float4(mFaceModel.Landmarks["LANDMARK_EYE_RIGHT_CENTER"], 1.0f) * modelMatrix;
	mFaceMapInfo.EyeDistance_FaceModelSpace = abs(leftEye.x - rightEye.x);
	leftEye = leftEye * displacementMapMatrix;
	rightEye = rightEye * displacementMapMatrix;
	mFaceMapInfo.EyeDistance_MapSpace = abs(leftEye.x - rightEye.x);

	float4 anchor = (rightEye + leftEye) / 2;
	mFaceMapInfo.Anchor_MapSpace = float2(anchor.x, anchor.y);
}

void Menu_FaceMapping::SculptMesh( CPUTRenderParameters &renderParams, const char *pModelPath, const char *pMaterialPath, const char *pTexturePath)
{
	ID3D11Device *pDevice = CPUT_DX11::GetDevice();
	ID3D11DeviceContext *pContext = CPUT_DX11::GetContext();
	HRESULT hr;

    CPUTRenderTargetColor *pFaceMapSculpt = CPUTRenderTargetColor::Create();
    pFaceMapSculpt->CreateRenderTarget("$Sculpt", FACE_DISPLACEMENT_WIDTH_HEIGHT, FACE_DISPLACEMENT_WIDTH_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	CPUTMesh *pMesh = mHeadModel->GetMesh(0);

	CPUTMaterialDX11 *pMaterial = (CPUTMaterialDX11 *)mHeadModel->GetMaterial(0, 1);
    CPUTVertexShaderDX11 *pVertexShader = pMaterial->GetVertexShader();

    int index_count = ((CPUTMeshDX11 *) pMesh)->GetIndexCount( );
	
	ID3D11Buffer *pOutBuffer, *pOutStagingBuffer;
	// Create the stream out buffer that will receive the displaced vertex positions
	// and a staging buffer for reading it back
	{
		D3D11_BUFFER_DESC outBufferDesc = {};
		outBufferDesc.ByteWidth = index_count * sizeof(float4);
		outBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		outBufferDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT;
		hr = pDevice->CreateBuffer(&outBufferDesc, NULL, &pOutBuffer);

		D3D11_BUFFER_DESC outStagingBufferDesc = {};
		outStagingBufferDesc.ByteWidth = outBufferDesc.ByteWidth;
		outStagingBufferDesc.Usage = D3D11_USAGE_STAGING;
		outStagingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		hr = pDevice->CreateBuffer(&outStagingBufferDesc, NULL, &pOutStagingBuffer);
	}

    UINT offset = 0;
	pContext->SOSetTargets(1, &pOutBuffer, &offset);

    ID3D11GeometryShader *pStreamOutGS;

	// semantic name, semantic index, start component, component count, output slot
	D3D11_SO_DECLARATION_ENTRY decl[] = { { 0, "TEXCOORD", 2, 0, 4, 0 } };
    hr = pDevice->CreateGeometryShaderWithStreamOutput( 
        pVertexShader->GetBlob()->GetBufferPointer(), 
        pVertexShader->GetBlob()->GetBufferSize(), 
        decl, 
        sizeof(decl) / sizeof(D3D11_SO_DECLARATION_ENTRY), NULL, 0, 0, NULL, &pStreamOutGS );

    CPUTGeometryShaderDX11 *pCputShader = CPUTGeometryShaderDX11::CreateFromNativeGeometryShader( pStreamOutGS );
    pMaterial->OverrideGeometryShader( pCputShader );

    float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    pFaceMapSculpt->SetRenderTarget(renderParams, mpFaceMapDepthBuffer, 0, black, true);

	mHeadModel->Render(renderParams, 1);

	// Copy stream out data
	pContext->CopyResource(pOutStagingBuffer, pOutBuffer);

	// Map it
    D3D11_MAPPED_SUBRESOURCE mappedStaging;
	pContext->Map(pOutStagingBuffer, 0, D3D11_MAP_READ, (UINT)0, &mappedStaging);
	float4 *pDispData = (float4 *)mappedStaging.pData;

    D3D11_MAPPED_SUBRESOURCE verts = pMesh->MapVertices( renderParams, CPUT_MAP_READ );
    D3D11_MAPPED_SUBRESOURCE indices = pMesh->MapIndices( renderParams, CPUT_MAP_READ );

    for (int i = 0; i < index_count; i++)
    {
        float z = pDispData[i].x;
        int vert_id = (int) pDispData[i].y;
    
        float3 *pPos = UVRemapUtils::GetPositionPointerFromMesh( vert_id, verts.pData, pMesh);
        pPos->z = z;
    }

    UVRemapUtils::SaveMeshToObjFile( verts, indices, pMesh, renderParams, pModelPath, pMaterialPath, pTexturePath );

	pContext->Unmap(pOutStagingBuffer, 0);

    pMesh->UnmapVertices( renderParams );
    pMesh->UnmapIndices( renderParams );

    pFaceMapSculpt->RestoreRenderTarget(renderParams);

    UVRemapUtils::SaveRenderTargetToFile( pFaceMapSculpt, pTexturePath );
    
	pOutStagingBuffer->Release();
	pOutBuffer->Release();

    delete pFaceMapSculpt;
}

void Menu_FaceMapping::UpdateHeadProjectionInfo()
{
	float eyeDistanceMS; // model space
	float2 mapScaleFactor;
	float zScaleMSToRS;

	// calculate head model scale metric
	eyeDistanceMS = abs(mHeadLandmark_LeftEyeCenter.x - mHeadLandmark_RightEyeCenter.x);
	float3 anchor = (mHeadLandmark_LeftEyeCenter + mHeadLandmark_RightEyeCenter) / 2.0f;
	float2 anchorMS = float2(anchor.x, anchor.y);

	// calculate scaling
	mapScaleFactor.x = (eyeDistanceMS / mFaceMapInfo.EyeDistance_MapSpace) * mTweaks.Scale;
	mapScaleFactor.y = (eyeDistanceMS / mFaceMapInfo.EyeDistance_MapSpace) * mTweaks.Scale;
	zScaleMSToRS = (eyeDistanceMS / mFaceMapInfo.EyeDistance_FaceModelSpace) * mTweaks.Scale;
	float zScaleRSToMS = 1.0f / zScaleMSToRS;

	// calculate the projection onto the head model
	float2 headProjectionPos;
	headProjectionPos.y = anchorMS.y + (mFaceMapInfo.Anchor_MapSpace.y * mapScaleFactor.y);
	headProjectionPos.x = anchorMS.x - (mFaceMapInfo.Anchor_MapSpace.x * mapScaleFactor.x);

	mMapProjectionCamera->SetWidth(2.0f * mapScaleFactor.x);
	mMapProjectionCamera->SetHeight(2.0f * mapScaleFactor.y);
	mMapProjectionCamera->SetPosition(headProjectionPos.x, headProjectionPos.y, -10.0f);
	mMapProjectionCamera->LookAt(headProjectionPos.x, headProjectionPos.y, 0.0f);
	mMapProjectionCamera->Update();
	mHeadProjInfo.ProjectionMatrix = *mMapProjectionCamera->GetViewMatrix() * *mMapProjectionCamera->GetProjectionMatrix();

	// 5 units in model space. This is a hardcoded offset from the tip of the nose. This could be specified with a model landmark
	float extrudeMaxZ = 5;
	float headMSUnitsPerRSUnitsZNormalized = zScaleRSToMS / mFaceMapInfo.DepthMap_ZRange;
	float extrudeBaseZ = mHeadLandmark_NoseTip.z + extrudeMaxZ;
	
	// max is nose tip
	mHeadProjInfo.ExtrudeMinZ = mHeadLandmark_NoseTip.z + extrudeMaxZ + mTweaks.DisplaceOffset.z;
	mHeadProjInfo.ExtrudeMaxZ = mHeadProjInfo.ExtrudeMinZ - extrudeMaxZ * mTweaks.Scale;
	mHeadProjInfo.DepthMapRangeMin = mFaceMapInfo.DepthMap_ZMeshStart - extrudeMaxZ * headMSUnitsPerRSUnitsZNormalized;
	mHeadProjInfo.DepthMapRangeMax = mFaceMapInfo.DepthMap_ZMeshStart;
}

void Menu_FaceMapping::HandleMeshExportRequest(CPUTRenderParameters &renderParams)
{
	OPENFILENAME ofn = { 0 };
	wchar_t filename[MAX_PATH] = L"sculpted_model";

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrTitle = L"Save Sculpted Obj";
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = L"Obj Files (*.obj)\0*.obj\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.Flags = OFN_EXPLORER;
	ofn.lpstrDefExt = L"obj";

	char mb_obj_string[sizeof(filename) * sizeof(wchar_t)];
	WideCharToMultiByte(CP_UTF8, 0, filename, -1, mb_obj_string, sizeof(mb_obj_string) - 1, NULL, NULL);

	if (TRUE == GetSaveFileName(&ofn))
	{
		WideCharToMultiByte(CP_UTF8, 0, filename, -1, mb_obj_string, sizeof(mb_obj_string) - 1, NULL, NULL);

		//displace and remap
		{
			char mb_texture_string[sizeof(filename)];
			strcpy_s(mb_texture_string, sizeof(mb_texture_string), mb_obj_string);

			char *pSlash = strrchr(mb_texture_string, '.');
			if (NULL != pSlash) *pSlash = NULL;

			strcat_s(mb_texture_string, sizeof(mb_texture_string), ".bmp");

			char mb_material_string[sizeof(filename)];
			strcpy_s(mb_material_string, sizeof(mb_material_string), mb_obj_string);

			pSlash = strrchr(mb_material_string, '.');
			if (NULL != pSlash) *pSlash = NULL;

			strcat_s(mb_material_string, sizeof(mb_material_string), ".mtl");

			SculptMesh( renderParams, mb_obj_string, mb_material_string, mb_texture_string);
		}
	}
}

void Menu_FaceMapping::Render(CPUTRenderParameters &renderParams)
{
	if (IsFaceLoaded())
	{
        CPUTCamera *pLastCamera = renderParams.mpCamera;

        CreateFaceMaps(renderParams);
		UpdateHeadProjectionInfo();

		// Setup shader parameters
		{
			renderParams.mpCamera = (CPUTCamera*)mCameraController->GetCamera();
			
			// hijack shadow matrix to store the face map projection matrix
			renderParams.mpShadowCamera = NULL;
			renderParams.mLightMatrixOverride = mHeadProjInfo.ProjectionMatrix;

			mHeadModel->mUserData1.x = mHeadProjInfo.ExtrudeMinZ;
			mHeadModel->mUserData1.y = mHeadProjInfo.ExtrudeMaxZ;
			mHeadModel->mUserData1.z = mHeadProjInfo.DepthMapRangeMin;
			mHeadModel->mUserData1.w = mHeadProjInfo.DepthMapRangeMax;

			// convert to linear RGB
			mHeadModel->mUserData2.x = pow(((float)mTweaks.BlendColorRGB[0] / 255.0f), 2.2f);
			mHeadModel->mUserData2.y = pow(((float)mTweaks.BlendColorRGB[1] / 255.0f), 2.2f);
			mHeadModel->mUserData2.z = pow(((float)mTweaks.BlendColorRGB[2] / 255.0f), 2.2f);
			mHeadModel->mUserData3.x = pow(((float)mTweaks.BlendColor2RGB[0] / 255.0f), 2.2f);
			mHeadModel->mUserData3.y = pow(((float)mTweaks.BlendColor2RGB[1] / 255.0f), 2.2f);
			mHeadModel->mUserData3.z = pow(((float)mTweaks.BlendColor2RGB[2] / 255.0f), 2.2f);
		}

		if (mSaveSculptedObj)
		{
			HandleMeshExportRequest(renderParams);
			mSaveSculptedObj = false;
		}

		// Render the head/hair
		mScene->Render(renderParams);

		// Draw overlay of displacement maps
		float spriteDim = floatMin((float)renderParams.mWidth, (float)renderParams.mHeight) / 4.0f;
		for (int i = 0; i < FaceMapTexture_Count; i++)
		{
			mMapSprites[i]->SetCoordType(SpriteCoordType_Screen);
			mMapSprites[i]->SetTL(10, 10 + (10 + spriteDim) * i, spriteDim, spriteDim);
			mMapSprites[i]->DrawSprite(renderParams);
		}
	}
}