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

#if defined CPUT_FOR_DX11
const std::string WINDOW_TITLE = "CPUTWindow DirectX 11";
#elif defined CPUT_FOR_OGL
const std::string WINDOW_TITLE = "CPUTWindow OpenGL 4";
#endif

#ifdef CPUT_OS_LINUX
const std::string GUI_DIR = "../Media/gui_assets/";
const std::string SYSTEM_DIR = "../Media/System/";
const std::string MEDIA_DIR = "../Media/Geartower/";
const std::string DEFAULT_SCENE = "../Media/defaultscene.scene";
#else
const std::string GUI_DIR = "../../../Media/gui_assets/";
const std::string SYSTEM_DIR = "../../../Media/System/";
#endif

#ifdef CPUT_FOR_OGL
void GLAPIENTRY openglCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, void const* userParam);
#endif

// Application entry point.  Execution begins here.
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
#ifdef DEBUG
#ifdef CPUT_OS_WINDOWS
    // tell VS to report leaks at any exit of the program
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //http://msdn.microsoft.com/en-us/library/x98tx3cf%28v=vs.100%29.aspx
    //Add a watch for �{,,msvcr110d.dll}_crtBreakAlloc� to the watch window
    //Set the value of the watch to the memory allocation number reported by your sample at exit.
    //Note that the �msvcr110d.dll� is for MSVC2012.  Other versions of MSVC use different versions of this dll; you�ll need to specify the appropriate version.
#endif
#endif
    CPUTResult result = CPUT_SUCCESS;
    int returnCode = 0;

    // create an instance of my sample
    MySample *pSample = new MySample();
   
    CommandParser mParsedCommandLine;
    mParsedCommandLine.ParseConfigurationOptions(argc, argv, "--");
    pSample->SetCommandLineArguments(mParsedCommandLine);

    // window and device parameters
    CPUTWindowCreationParams params;
    params.startFullscreen = mParsedCommandLine.GetParameter("fullscreen");
    mParsedCommandLine.GetParameter("height",      &(params.windowHeight));
    mParsedCommandLine.GetParameter("width",       &(params.windowWidth));
    mParsedCommandLine.GetParameter("xpos",        &(params.windowPositionX));
    mParsedCommandLine.GetParameter("ypos",        &(params.windowPositionY));
#ifdef CPUT_FOR_DX11
    mParsedCommandLine.GetParameter("refreshrate", &(params.deviceParams.refreshRate));
#endif
    
    result = pSample->CPUTCreateWindowAndContext(WINDOW_TITLE, params);
    ASSERT(CPUTSUCCESS(result), "CPUT Error creating window and context.");
#ifdef CPUT_FOR_OGL
    glDebugMessageCallback(openglCallbackFunction, NULL);
#endif
   
    pSample->Create();
    returnCode = pSample->CPUTMessageLoop();
    pSample->ReleaseResources();
    pSample->DeviceShutdown();

    // cleanup resources
    SAFE_DELETE(pSample);

#if defined CPUT_FOR_DX11 && defined SUPER_DEBUG_DX
    typedef HRESULT(__stdcall *fPtrDXGIGetDebugInterface)(const IID&, void**);
    HMODULE hMod = GetModuleHandle(L"Dxgidebug.dll");
    fPtrDXGIGetDebugInterface DXGIGetDebugInterface = (fPtrDXGIGetDebugInterface)GetProcAddress(hMod, "DXGIGetDebugInterface"); 
 
    IDXGIDebug *pDebugInterface;
    DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDebugInterface); 

    pDebugInterface->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_ALL);
#endif 
    return returnCode;
}

//TODO move this to CPUT_OGL_GL
#ifdef CPUT_FOR_OGL
void GLAPIENTRY openglCallbackFunction(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    void const* userParam){

    std::cout << "---------------------opengl-callback-start------------" << std::endl;
    std::cout << "message: " << message << std::endl;
    std::cout << "type: ";
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        std::cout << "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        std::cout << "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        std::cout << "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        std::cout << "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        std::cout << "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        std::cout << "OTHER";
        break;
    }
    std::cout << std::endl;

    std::cout << "id: " << id;
    std::cout << "severity: ";
    switch (severity){
    case GL_DEBUG_SEVERITY_LOW:
        std::cout << "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        std::cout << "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        std::cout << "HIGH";
        break;
    }
    std::cout << std::endl;
    std::cout << "---------------------opengl-callback-end--------------" << std::endl;
}
#endif

