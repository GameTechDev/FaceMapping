/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "CPUT.h"

class CPUTMesh;

class UVRemapUtils
{
private:
    struct Vertex
    {
        float3 position;
        float3 normal;
        float2 uv;
        float2 bary;
    };

public:
    static bool SaveRenderTargetToFile(
        CPUTRenderTargetColor *pRenderTarget, 
        const char *pPath
        );

    static float3 *GetPositionPointerFromMesh(
        const int vertex_index, 
        const void *pVertData, 
        const CPUTMesh *pMesh 
        );

    static bool SaveMeshToObjFile(
        const char *pModelName, 
        CPUTRenderParameters &renderParams, 
        const char *pObjPath, 
        const char *pMtlPath, 
        const char *pTexturePath
        );

    static bool SaveMeshToObjFile(
        D3D11_MAPPED_SUBRESOURCE verts, 
        D3D11_MAPPED_SUBRESOURCE indices, 
        CPUTMesh *pMesh, 
        CPUTRenderParameters &renderParams, 
        const char *pObjPath, 
        const char *pMaterialPath, 
        const char *pTexturePath
        );

private:
    static bool CopyVerticesFromMesh( 
        Vertex *pV1, 
        Vertex *pV2, 
        Vertex *pV3, 
        const int index_0, 
        const int index_1, 
        const int index_2, 
        const void *pVertData, 
        const CPUTMesh *pMesh 
        );
};
