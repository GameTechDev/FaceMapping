/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "UVRemapUtils.h"
#include "CPUT_DX11.h"
#include "CPUTMeshDX11.h"
#include "CPUTModel.h"

bool UVRemapUtils::SaveRenderTargetToFile(CPUTRenderTargetColor *pRenderTarget, const char *pPath)
{
    ID3D11Texture2D *pStagingTexture = NULL;
    ID3D11DeviceContext *pContext = NULL;
    ID3D11Resource *pResource = NULL;
    FILE *pFile = NULL;

    bool success = false;

    do
    {
        ID3D11Device *pDevice = CPUT_DX11::GetDevice();
        D3D11_TEXTURE2D_DESC stagingDesc = pRenderTarget->GetColorDesc();

        stagingDesc.BindFlags = 0;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        HRESULT hr = pDevice->CreateTexture2D(&stagingDesc, NULL, &pStagingTexture);
        if ( FALSE == SUCCEEDED(hr) ) break;

        pDevice->GetImmediateContext(&pContext);

        pRenderTarget->GetColorResourceView()->GetResource(&pResource);
        pContext->CopyResource(pStagingTexture, pResource);

        D3D11_MAPPED_SUBRESOURCE mappedData;
        pContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
            
        BITMAPFILEHEADER bitmapFileHeader = { 0 };
        BITMAPINFOHEADER bitmapInfoHeader = { 0 };
		
        bitmapFileHeader.bfOffBits = sizeof(bitmapFileHeader) + sizeof(bitmapInfoHeader);
        bitmapFileHeader.bfSize = stagingDesc.Width * stagingDesc.Height * 4 + sizeof(bitmapInfoHeader) + sizeof(bitmapFileHeader);
        bitmapFileHeader.bfType = 0x4d42;
            
        bitmapInfoHeader.biBitCount = 32;
        bitmapInfoHeader.biCompression = BI_RGB;
        bitmapInfoHeader.biHeight = stagingDesc.Height;
        bitmapInfoHeader.biWidth = stagingDesc.Width;
        bitmapInfoHeader.biPlanes = 1;
        bitmapInfoHeader.biSize = sizeof(bitmapInfoHeader);
        bitmapInfoHeader.biSizeImage = stagingDesc.Width * stagingDesc.Height * 4;

        errno_t err = fopen_s(&pFile, pPath, "wb");
        if (0 != err) break;

        fwrite( &bitmapFileHeader, sizeof(bitmapFileHeader), 1, pFile );
	    fwrite( &bitmapInfoHeader, sizeof(bitmapInfoHeader), 1, pFile );
            
        byte *pDestHead = (byte *) malloc( stagingDesc.Height * stagingDesc.Width * 4 );

        if (mappedData.RowPitch == stagingDesc.Width * 4)
            memcpy(pDestHead, mappedData.pData, stagingDesc.Width * stagingDesc.Height * 4);
        else
        {
            byte *pDest = pDestHead;
    
            for ( unsigned int y = 0; y < stagingDesc.Height; y++ )
            {
                byte *pSource = (byte *) mappedData.pData + (y * mappedData.RowPitch);

                memcpy(pDest, pSource, stagingDesc.Width * 4);
                pDest += stagingDesc.Width * 4;
            }
        }

        fwrite( pDestHead, stagingDesc.Width * stagingDesc.Height * 4, 1, pFile );

        free( pDestHead );
    
        success = true;

    } while ( false );

     if ( NULL != pFile ) fclose( pFile );
     if ( NULL != pContext ) pContext->Unmap(pStagingTexture, 0);
     if ( NULL != pResource ) pResource->Release();
     if ( NULL != pContext ) pContext->Release();
     if ( NULL != pStagingTexture ) pStagingTexture->Release();

     return success;
}

float3 *UVRemapUtils::GetPositionPointerFromMesh( const int vertex_index, const void *pVertData, const CPUTMesh *pMesh )
{
    // Cache offset data so we only need to look it up once
    // per mesh change
    static const CPUTMesh *s_pMesh;
    static int s_pos_offset = -1;
    static int s_uv_offset = -1;
    static int s_normal_offset = -1;

    static unsigned int s_stride;

    if (pMesh != s_pMesh)
    {
        s_pMesh = pMesh;

        s_pos_offset = -1;
        s_uv_offset = -1;
        s_normal_offset = -1;

        CPUTMeshDX11 *pDx11Mesh = (CPUTMeshDX11 *) pMesh;
    
        s_stride = pDx11Mesh->GetVertexStride();

        D3D11_INPUT_ELEMENT_DESC *pDesc = pDx11Mesh->GetLayoutDescription();
    
        int i = 0;

        while (true)
        {
            if (0 == pDesc[i].Format)
                break;

            if (-1 == s_pos_offset && 0 == strcmp(pDesc[i].SemanticName, "POSITION"))
                s_pos_offset = pDesc[i].AlignedByteOffset;
            else if (-1 == s_uv_offset && 0 == strcmp(pDesc[i].SemanticName, "TEXCOORD"))
                s_uv_offset = pDesc[i].AlignedByteOffset;
            else if (-1 == s_normal_offset && 0 == strcmp(pDesc[i].SemanticName, "NORMAL"))
                s_normal_offset = pDesc[i].AlignedByteOffset;

            if (-1 != s_uv_offset && -1 != s_pos_offset && -1 != s_normal_offset)
                break;

            ++i;
        }
    }

    if (-1 == s_pos_offset || -1 == s_uv_offset)
        return NULL;

    const byte *pVerts = (const byte *) pVertData;

    const byte *pVert1 = pVerts + vertex_index * s_stride;

    return (float3 *) (pVert1 + s_pos_offset);
}

bool UVRemapUtils::SaveMeshToObjFile(
    const char *pModelName, 
    CPUTRenderParameters &renderParams, 
    const char *pObjPath, 
    const char *pMtlPath, 
    const char *pTexturePath
    )
{
    CPUTModel *pModel = CPUTAssetLibrary::GetAssetLibrary()->GetModelByName( pModelName );
    CPUTMesh *pMesh = pModel->GetMesh( 0 );

    D3D11_MAPPED_SUBRESOURCE verts = pMesh->MapVertices( renderParams, CPUT_MAP_READ );
    D3D11_MAPPED_SUBRESOURCE indices = pMesh->MapIndices( renderParams, CPUT_MAP_READ );

    SaveMeshToObjFile( verts, indices, pMesh, renderParams, pObjPath, pMtlPath, pTexturePath );

    pMesh->UnmapVertices( renderParams );
    pMesh->UnmapIndices( renderParams );

    return true;
}

bool UVRemapUtils::SaveMeshToObjFile(
    D3D11_MAPPED_SUBRESOURCE verts, 
    D3D11_MAPPED_SUBRESOURCE indices, 
    CPUTMesh *pMesh, 
    CPUTRenderParameters &renderParams, 
    const char *pObjPath, 
    const char *pMaterialPath, 
    const char *pTexturePath
    )
{
    FILE *pFile;

    const char *pMaterialName = strrchr(pMaterialPath, '\\');
    if (NULL != pMaterialName) pMaterialName++;

    const char *pTextureName = strrchr(pTexturePath, '\\');
    if (NULL != pTextureName) pTextureName++;
    
    errno_t err = fopen_s( &pFile, pMaterialPath, "w" );

    if (0 != err)
        return false;

    fprintf(pFile, 
        "newmtl material0\n"
        "Ka 1.000 1.000 1.000\n"
        "Kd 1.000 1.000 1.000\n"
        "Ks 0.000 0.000 0.000\n"
        "d 1.0\n"
        "illum 1\n"
        "map_Kd %s\n", pTextureName);

    fclose(pFile);
    
    err = fopen_s( &pFile, pObjPath, "w" );

    if (0 != err)
        return false;

    int i;
    
    int index_count  = ((CPUTMeshDX11 *) pMesh)->GetIndexCount( );
    int vertex_count = ((CPUTMeshDX11 *) pMesh)->GetVertexCount( );

    fprintf(pFile, "mtllib %s\n", pMaterialName);

    Vertex v1, v2, v3;

    for (i = 0; i < vertex_count; i++)
    {
        if (false == CopyVerticesFromMesh( &v1, NULL, NULL, i, -1, -1, verts.pData, pMesh))
            break;

        fprintf(pFile, "v %f %f %f\n", v1.position.x, v1.position.y, v1.position.z);
    }

    for (i = 0; i < vertex_count; i++)
    {
        if (false == CopyVerticesFromMesh( &v1, NULL, NULL, i, -1, -1, verts.pData, pMesh))
            break;

        fprintf(pFile, "vt %f %f\n", v1.uv.x, v1.uv.y);
    }

    for (i = 0; i < vertex_count; i++)
    {
        if (false == CopyVerticesFromMesh( &v1, NULL, NULL, i, -1, -1, verts.pData, pMesh))
            break;

        fprintf(pFile, "vn %f %f %f\n", v1.normal.x, v1.normal.y, v1.normal.z);
    }

    fprintf(pFile, "s off\n");
    fprintf(pFile, "usemtl material0\n");

    int *pIndices = (int *) indices.pData;

    for (i = 0; i < index_count; i += 3)
    {
        int vert_id_1 = pIndices[i + 0];
        int vert_id_2 = pIndices[i + 1];
        int vert_id_3 = pIndices[i + 2];

        fprintf(pFile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", 
            vert_id_1 + 1, vert_id_1 + 1, vert_id_1 + 1,
            vert_id_2 + 1, vert_id_2 + 1, vert_id_2 + 1,
            vert_id_3 + 1, vert_id_3 + 1, vert_id_3 + 1);
    }

    fclose( pFile );

    return true;
}


//Instead of this, the mesh class should probably have a way to parse the vertices out (because it knows it's API and it's format)
bool UVRemapUtils::CopyVerticesFromMesh( 
    UVRemapUtils::Vertex *pV1, 
    UVRemapUtils::Vertex *pV2, 
    UVRemapUtils::Vertex *pV3, 
    const int index_0, 
    const int index_1, 
    const int index_2, 
    const void *pVertData, 
    const CPUTMesh *pMesh 
    )
{
    // Cache offset data so we only need to look it up once
    // per mesh change
    static const CPUTMesh *s_pMesh;
    static int s_pos_offset = -1;
    static int s_uv_offset = -1;
    static int s_normal_offset = -1;

    static unsigned int s_stride;

    if (pMesh != s_pMesh)
    {
        s_pMesh = pMesh;

        s_pos_offset = -1;
        s_uv_offset = -1;
        s_normal_offset = -1;

        CPUTMeshDX11 *pDx11Mesh = (CPUTMeshDX11 *) pMesh;
    
        s_stride = pDx11Mesh->GetVertexStride();

        D3D11_INPUT_ELEMENT_DESC *pDesc = pDx11Mesh->GetLayoutDescription();
    
        int i = 0;

        while (true)
        {
            if (0 == pDesc[i].Format)
                break;

            if (-1 == s_pos_offset && 0 == strcmp(pDesc[i].SemanticName, "POSITION"))
                s_pos_offset = pDesc[i].AlignedByteOffset;
            else if (-1 == s_uv_offset && 0 == strcmp(pDesc[i].SemanticName, "TEXCOORD"))
                s_uv_offset = pDesc[i].AlignedByteOffset;
            else if (-1 == s_normal_offset && 0 == strcmp(pDesc[i].SemanticName, "NORMAL"))
                s_normal_offset = pDesc[i].AlignedByteOffset;

            if (-1 != s_uv_offset && -1 != s_pos_offset && -1 != s_normal_offset)
                break;

            ++i;
        }
    }

    if (-1 == s_pos_offset || -1 == s_uv_offset)
        return false;

    const byte *pVerts = (const byte *) pVertData;

    if (NULL != pV1)
    {
        const byte *pVert1 = pVerts + index_0 * s_stride;

        pV1->position = *(float3 *) (pVert1 + s_pos_offset);
        pV1->normal = *(float3 *) (pVert1 + s_normal_offset);
        pV1->uv = *(float2 *) (pVert1 + s_uv_offset);
        pV1->bary = float2(1, 0);
    }

    if (NULL != pV2)
    {
        const byte *pVert2 = pVerts + index_1 * s_stride;

        pV2->position = *(float3 *) (pVert2 + s_pos_offset);
        pV2->normal = *(float3 *) (pVert2 + s_normal_offset);
        pV2->uv = *(float2 *) (pVert2 + s_uv_offset);
        pV2->bary = float2(0, 1);
    }

    if (NULL != pV3)
    {
        const byte *pVert3 = pVerts + index_2 * s_stride;

        pV3->position = *(float3 *) (pVert3 + s_pos_offset);
        pV3->normal = *(float3 *) (pVert3 + s_normal_offset);
        pV3->uv = *(float2 *) (pVert3 + s_uv_offset);
        pV3->bary = float2(0, 0);
    }

    return true;
}