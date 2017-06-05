/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imlied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "SampleStart.h"
#include "CPUTMaterial.h"
#include "CPUTLight.h"
#ifdef CPUT_FOR_DX11
#include "CPUTBufferDX11.h"
#include "CPUTTextureDX11.h"
#include "CPUTGuiControllerDX11.h"
#include <DXGIDebug.h>
#endif

#include "CPUTRenderTarget.h"
#include "CPUTFont.h"

#include "Menu/MenuGlob.h"
#include "Menu/MenuController.h"
    
CPUTCamera* GetCamera(CPUTScene* pScene);

std::string GetFilename();

MySample * MySample::Instance = NULL;


const UINT SHADOW_WIDTH_HEIGHT = 2048;
//-----------------------------------------------------------------------------
void MySample::Create()
{
    CreateResources();
    
	int windowWidth, windowHeight;
	mpWindow->GetClientDimensions(&windowWidth, &windowHeight);

	SampleUtil_Init();
	MenuGlob_Init();
	MenuGlob_SetScreenDim(windowWidth, windowHeight);
	MenuController_Init();
	
	std::string userDir = GetUserDataDirectory();
	

#ifndef DISABLE_RSSDK
	MenuController_PushMenu(&gMenu_Scan);

	// go directly to the face mapping menu
	//std::string debugFace;
	//CPUTFileSystem::CombinePath(userDir, "joe.obj", &debugFace);
	//gMenu_FaceMapping.LoadFace(debugFace);
	//MenuController_PushMenu(&gMenu_FaceMapping);

	// Go directly to the face scan preview menu
	//gMenu_FaceScanPreview.LoadFaceObj("", true);
	//gMenu_FaceScanPreview.SetFaceScanMode( FaceScanPreviewMode_ApproveScan );
	//MenuController_PushMenu(&gMenu_FaceScanPreview);
	
#else
	std::string debugFace;
	CPUTFileSystem::CombinePath(userDir, "joe.obj", &debugFace);
	gMenu_FaceMapping.LoadFace( debugFace );
	MenuController_PushMenu(&gMenu_FaceMapping);

	// go directly to the face preview menu
	//gMenu_FaceScanPreview.LoadFaceObj("", true);
	//MenuController_PushMenu(&gMenu_FaceScanPreview);

#endif

	MenuGlob_GUI()->SetActivePanel(MENU_CPUT_PANEL_ID);

}

//-----------------------------------------------------------------------------
void MySample::Update(double deltaSeconds)
{

	if (mpWindow->DoesWindowHaveFocus())
	{
		mpCameraController->SetCamera(mpCamera);
		if (mpCameraController)
			mpCameraController->Update((float)deltaSeconds);
	}

	MenuController_Update((float)deltaSeconds);

    CPUTGetGuiController()->ControlModified();
    CPUTGetGuiController()->Update();
}

// Handle keyboard events
//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleKeyboardEvent(CPUTKey key, CPUTKeyState state)
{
    static bool panelToggle = false;
    CPUTEventHandledCode    handled = CPUT_EVENT_UNHANDLED;

    CPUTGuiController*  pGUI = CPUTGetGuiController();

    switch(key)
    {   
    case KEY_ESCAPE:
        handled = CPUT_EVENT_HANDLED;
        PostQuitMessage(0);
        break;
    
    default:
        break;
    }
    // pass it to the camera controller
    if(handled == CPUT_EVENT_UNHANDLED)
    {
        if (mpCameraController)
        handled = mpCameraController->HandleKeyboardEvent(key, state);
    }
    return handled;
}

// Handle mouse events
//-----------------------------------------------------------------------------
CPUTEventHandledCode MySample::HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message)
{
	CPUTEventHandledCode code = MenuController_HandleMouseEvent( x, y, wheel, state, message );
	if (code != CPUT_EVENT_HANDLED)
	{
		if (mpCameraController)
		{
			return mpCameraController->HandleMouseEvent(x, y, wheel, state, message);
		}
	}
    return code;
}

// Handle any control callback events
//-----------------------------------------------------------------------------
void MySample::HandleCallbackEvent( CPUTEventID Event, CPUTControlID ControlID, CPUTControl *pControl )
{
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(pControl);
    std::string SelectedItem;
    static bool resize = false;

    switch(ControlID)
    {
    case ID_FULLSCREEN_BUTTON:
        CPUTToggleFullScreenMode();
        break;
	
    default:
        break;
    }

	MenuController_HandleCPUTEvent(Event, ControlID, pControl);
}

//-----------------------------------------------------------------------------
void MySample::ResizeWindow(UINT width, UINT height)
{
    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    // Before we can resize the swap chain, we must release any references to it.
    // We could have a "AssetLibrary::ReleaseSwapChainResources(), or similar.  But,
    // Generic "release all" works, is simpler to implement/maintain, and is not performance critical.

    CPUT_DX11::ResizeWindow( width, height );

    // Resize any application-specific render targets here
    if( mpCamera ) 
        mpCamera->SetAspectRatio(((float)width)/((float)height));
    CPUTGetGuiController()->Resize(width, height);

	MenuController_Resize(width, height);
}

//-----------------------------------------------------------------------------
void MySample::Render(double deltaSeconds)
{
    CPUTRenderParameters renderParams;

    const int DEFAULT_MATERIAL = 0;
    const int SHADOW_MATERIAL = 1;
    renderParams.mpShadowCamera = NULL;
    renderParams.mpCamera = mpShadowCamera;
    renderParams.mpPerFrameConstants = (CPUTBuffer*)mpPerFrameConstantBuffer;
    renderParams.mpPerModelConstants = (CPUTBuffer*)mpPerModelConstantBuffer;
    //Animation
    renderParams.mpSkinningData = (CPUTBuffer*)mpSkinningDataConstantBuffer;
    int windowWidth, windowHeight;
    mpWindow->GetClientDimensions( &windowWidth, &windowHeight);
    renderParams.mWidth = windowWidth;
    renderParams.mHeight = windowHeight;
    renderParams.mRenderOnlyVisibleModels = false;
    //*******************************
    // Draw the shadow scene
    //*******************************
    UpdatePerFrameConstantBuffer(renderParams, deltaSeconds);

    renderParams.mWidth = windowWidth;
    renderParams.mHeight = windowHeight;
    renderParams.mpCamera = mpCamera;
    renderParams.mpShadowCamera = mpShadowCamera;
    UpdatePerFrameConstantBuffer(renderParams, deltaSeconds);

    // Clear back buffer
    const float clearColor[] = { 0.0993f, 0.0993f, 0.0993f, 1.0f };
    mpContext->ClearRenderTargetView( mpBackBufferRTV,  clearColor );
    mpContext->ClearDepthStencilView( mpDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

    if(mpCameraController->GetCamera() == mpShadowCamera)
    {
        mpDebugSprite->DrawSprite(renderParams);
    }

	MenuController_Render(renderParams);

	CPUTDrawGUI();
}

void MySample::SetCommandLineArguments(const CommandParser& commandArguments)
{
    mParsedCommandLine = commandArguments;
}

void MySample::ReleaseResources()
{
    // Note: these two are defined in the base.  We release them because we addref them.
    SAFE_RELEASE(mpCamera);
    SAFE_RELEASE(mpShadowCamera);
    SAFE_DELETE(mpCameraController);
    SAFE_DELETE(mpDebugSprite);
    SAFE_DELETE(mpShadowRenderTarget);
    CPUTAssetLibrary::GetAssetLibrary()->ReleaseAllLibraryLists();
    CPUT_DX11::ReleaseResources();
}

std::string GetFilename()
{
#ifdef CPUT_OS_WINDOWS
    OPENFILENAMEA filename;
    CHAR file[MAX_PATH];
    ZeroMemory(file, sizeof(file));
    ZeroMemory(&filename, sizeof(OPENFILENAME));
    filename.lStructSize = sizeof(OPENFILENAME);
    filename.lpstrFile = file;
    filename.nMaxFile = MAX_PATH;
    filename.lpstrFilter = "*.scene\0\0";
    filename.nFilterIndex = 1;
    filename.lpstrInitialDir = NULL;
    filename.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    GetOpenFileNameA(&filename);
    return std::string(filename.lpstrFile);
#else
    return std::string("");
#endif
}

CPUTCamera* GetCamera(CPUTScene* pScene)
{
    for (unsigned int i = 0; i < pScene->GetNumAssetSets(); i++)
    {
        CPUTCamera* pCamera = pScene->GetAssetSet(i)->GetFirstCamera();
        if (pCamera)
            return pCamera;
    }
    return NULL;
}

void MySample::CreateResources()
{
    CPUT_DX11::CreateResources();
    int width, height;
    mpWindow->GetClientDimensions(&width, &height);

    CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();

    std::string executableDirectory;
    std::string mediaDirectory;

    CPUTFileSystem::GetExecutableDirectory(&executableDirectory);
    CPUTFileSystem::ResolveAbsolutePathAndFilename(executableDirectory + "../../../../Media/", &mediaDirectory);
    
	pAssetLibrary->SetMediaDirectoryName(mediaDirectory + "gui_assets/");
    pAssetLibrary->SetSystemDirectoryName(mediaDirectory + "System/");
    pAssetLibrary->SetFontDirectoryName(mediaDirectory + "gui_assets/Font/");

    CPUTGuiController *pGUI = CPUTGetGuiController();
    pGUI->Initialize("guimaterial_dds_16", "arial_16.fnt");

    pGUI->SetCallback(this);

    pAssetLibrary->SetMediaDirectoryName(mediaDirectory);

    mpShadowRenderTarget = CPUTRenderTargetDepth::Create();
    mpShadowRenderTarget->CreateRenderTarget(std::string("$shadow_depth"), SHADOW_WIDTH_HEIGHT, SHADOW_WIDTH_HEIGHT, DXGI_FORMAT_D32_FLOAT);

    CPUTMaterial* pDebugMaterial = pAssetLibrary->GetMaterial("%sprite");
    mpDebugSprite = CPUTSprite::Create(-1.0f, -1.0f, 0.5f, 0.5f, pDebugMaterial);
    SAFE_RELEASE(pDebugMaterial);

    mpCameraController = CPUTCameraControllerFPS::Create();
    mpShadowCamera = CPUTCamera::Create(CPUT_ORTHOGRAPHIC);
     
    pGUI->Resize(width, height);
	
}


