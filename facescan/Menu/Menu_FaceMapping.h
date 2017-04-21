/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __MENU_FACEMAPPING__
#define __MENU_FACEMAPPING__

#include "MenuBase.h"
#include "../CFaceModel.h"
#include <vector>
#include "CPUT.h"
#include "CPUTRenderTarget.h"
#include "CPUTSprite.h"
#include "CPUTScene.h"
#include "CPUTCamera.h"
#include "CPUTText.h"

enum FaceMapTexture
{
	FaceMapTexture_Color,
	FaceMapTexture_Depth,
	FaceMapTexture_Count,
};

// User tweakable variables. Will try to create 
struct MappingTweaks
{
	float Scale;

	// Displacement applied in head space
	float3 DisplaceOffset;

	// Rotation applied to the RealSense model before creating the face maps
	float FaceYaw;
	float FacePitch;
	float FaceRoll;

	byte BlendColorRGB[3];
	byte BlendColor2RGB[3];
};

// data that goes along with the face maps providing information about how to do the projection
struct FaceMapInfo
{
	// distance between the left and right eye center in the face model space and map space
	float EyeDistance_FaceModelSpace;
	float EyeDistance_MapSpace;

	// coordinates of the anchor point projected onto the map textures x: -1 to 1, y: -1 to 1
	float2 Anchor_MapSpace;

	// the depth values in the depth image will be in range 0-1 DepthMap_ZRange is the Z distance that
	// range represents in world space. AKA: farClip - nearClip
	float DepthMap_ZRange;

	// The z value at which the face mesh starts. The near clipping plane is shifted away from the face to avoid artifacts
	// with near plane clipping
	float DepthMap_ZMeshStart;
};

struct HeadProjectionInfo
{
	// Converts positions in the head model's object space to face map texture coordinates
	float4x4 ProjectionMatrix;

	// max/min z displacement in head model space. The maximum displacement should be applied when the depth
	float ExtrudeMinZ;
	float ExtrudeMaxZ;

	// used to convert the depth value from the depth map into 
	float DepthMapRangeMin;
	float DepthMapRangeMax;
};

struct HairInfo
{
	const char *name;
	CPUTAssetSet *set;
};

class Menu_FaceMapping : public MenuBase
{
public:
	virtual void Init();
	virtual void Shutdown();

	void HandleCPUTEvent(int eventID, int controlID, CPUTControl *control);

	void Update(float dt);
	virtual CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message);

	void ActivationChanged(bool active);

	void LoadFace(const std::string &filename);

	void Render(CPUTRenderParameters &renderParams);

	bool IsFaceLoaded() { return mFaceCPUTModel != NULL; }

private:
	
	void UpdateHairModels();

	void CreateFaceMaps(CPUTRenderParameters &renderParams);
    void UpdateHeadProjectionInfo();

	void HandleMeshExportRequest(CPUTRenderParameters &renderParams);
	void SculptMesh(CPUTRenderParameters &renderParams, const char *pModelPath, const char *pMaterialPath, const char *pTexturePath);

	void UpdateGUIFromTweaks();
	void UpdateTweaksFromGUI();
	void ResetTweaks();

	CFaceModel mFaceModel;
	CPUTModel *mFaceCPUTModel;

	MappingTweaks mTweaks;

	HeadProjectionInfo mHeadProjInfo;
	FaceMapInfo mFaceMapInfo;
	CPUTRenderTargetColor   *mpFaceMapRTColor;
	CPUTRenderTargetColor   *mpFaceMapRTDepth;

	CPUTRenderTargetDepth   *mpFaceMapDepthBuffer;

	CPUTSprite *mMapSprites[FaceMapTexture_Count];

	CPUTCamera *mDisplacementCamera;

	CPUTSlider *mScalingSlider;
	CPUTSlider *mSlider_FaceYaw;
	CPUTSlider *mSlider_FacePitch;
	CPUTSlider *mSlider_FaceRoll;
	CPUTSlider *mSlider_FacePhysicalDisplaceOffsetZ;

	CPUTSlider *mSlider_HairType;

	CPUTScene *mScene;
	CPUTModel *mHeadModel;

	std::vector<HairInfo> mHairList;

	CPUTCameraModelViewer *mCameraController;

	// used to define a orthographic projection matrix to project maps onto the model
	CPUTCamera *mMapProjectionCamera;

	float3 mHeadLandmark_LeftEyeCenter;
	float3 mHeadLandmark_RightEyeCenter;
	float3 mHeadLandmark_NoseTip;

	std::string mObjFilename;
    bool mSaveSculptedObj;
};

#endif