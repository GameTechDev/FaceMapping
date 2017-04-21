/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "CPUTControl.h"

// Constructor
//------------------------------------------------------------------------------
CPUTControl::CPUTControl():mControlVisible(true),
    mControlAutoArranged(true),
    mHotkey(KEY_NONE),
    mControlType(CPUT_CONTROL_UNKNOWN),
    mControlID(0),
    mpCallbackHandler(NULL),
    mControlState(CPUT_CONTROL_ACTIVE)
{
    mColor.r = mColor.g = mColor.b = mColor.a = 1.0f;
}

// Destructor
//------------------------------------------------------------------------------
CPUTControl::~CPUTControl()
{
}

// Control type/identifier routines that have a common implementation for all controls

// Sets the control's ID used for identification purposes (hopefully unique)
//------------------------------------------------------------------------------
void CPUTControl::SetControlID(CPUTControlID id)
{
    mControlID = id;
}

// Get the ID for this control
//------------------------------------------------------------------------------
CPUTControlID CPUTControl::GetControlID()
{
    return mControlID;
}

// Get the type of control this is (button/dropdown/etc)
//------------------------------------------------------------------------------
CPUTControlType CPUTControl::GetType()
{
    return mControlType;
}

// Set callback handler
//------------------------------------------------------------------------------
void CPUTControl::SetControlCallback(CPUTCallbackHandler *pHandler)
{
    mpCallbackHandler = pHandler;
}

// set whether controls is visible or not (it is still there, but not visible)
//------------------------------------------------------------------------------
void CPUTControl::SetVisibility(bool bVisible)
{
    mControlVisible = bVisible;
}

// visibility state
//------------------------------------------------------------------------------
bool CPUTControl::IsVisible()
{
    return mControlVisible;
}

// Set the hot key for keyboard events for this control
//------------------------------------------------------------------------------
void CPUTControl::SetHotkey(CPUTKey hotKey)
{
    mHotkey = hotKey;
}

// Get the hot key set for this control
//------------------------------------------------------------------------------
CPUTKey CPUTControl::GetHotkey()
{
    return mHotkey;
}

// Should this control be auto-arranged?
//------------------------------------------------------------------------------
void CPUTControl::SetAutoArranged(bool bIsAutoArranged)
{
    mControlAutoArranged = bIsAutoArranged;
}

//------------------------------------------------------------------------------
bool CPUTControl::IsAutoArranged()
{
    return mControlAutoArranged;
}

// Set the control to enabled or greyed out
//------------------------------------------------------------------------------
void CPUTControl::SetEnable(bool bEnabled)
{
    if(!bEnabled)
    {
        mControlState = CPUT_CONTROL_INACTIVE;
    }
    else
    {
        mControlState = CPUT_CONTROL_ACTIVE;
    }
}

// Return bool if the control is enabled/greyed out
//------------------------------------------------------------------------------
bool CPUTControl::IsEnabled()
{
    if(mControlState == CPUT_CONTROL_INACTIVE)
    {
        return false;
    }

    return true;
}

// This generates a quad with the supplied coordinates/uv's/etc.
//------------------------------------------------------------------------
void CPUTControl::AddQuadIntoMirrorBuffer(CPUTGUIVertex *pMirrorBuffer, 
    int index, 
    float x, 
    float y, 
    float w, 
    float h, 
    float3 uv1, 
    float3 uv2,
    GUIColor color)
{
    pMirrorBuffer[index+0].Pos = float3( x + 0.0f, y + 0.0f, 1.0f);
    pMirrorBuffer[index+0].UV = float2(uv1.x, uv1.y);
    pMirrorBuffer[index+0].Color = color;

    pMirrorBuffer[index+1].Pos = float3( x + w, y + 0.0f, 1.0f);
    pMirrorBuffer[index+1].UV = float2(uv2.x, uv1.y);
    pMirrorBuffer[index+1].Color = color;

    pMirrorBuffer[index+2].Pos = float3( x + 0.0f, y + h, 1.0f);
    pMirrorBuffer[index+2].UV = float2(uv1.x, uv2.y);
    pMirrorBuffer[index+2].Color = color;

    pMirrorBuffer[index+3].Pos = float3( x + w, y + 0.0f, 1.0f);
    pMirrorBuffer[index+3].UV = float2(uv2.x, uv1.y);
    pMirrorBuffer[index+3].Color = color;

    pMirrorBuffer[index+4].Pos = float3( x + w, y + h, 1.0f);
    pMirrorBuffer[index+4].UV = float2(uv2.x, uv2.y);
    pMirrorBuffer[index+4].Color = color;

    pMirrorBuffer[index+5].Pos = float3( x + 0.0f, y +h, 1.0f);
    pMirrorBuffer[index+5].UV = float2(uv1.x, uv2.y);
    pMirrorBuffer[index+5].Color = color;
}