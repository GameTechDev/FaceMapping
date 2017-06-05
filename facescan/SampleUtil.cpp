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
#include "SampleUtil.h"
#include "assert.h"
#include "CPUT.h"
#include "CPUT_DX11.h"
#include "CPUTTextureDX11.h"
#include "CPUTModel.h"
#include "CPUTScene.h"
#include "assert.h"
#include "ObjLoader.h"
#include "FileAPI.h"

#define CODE_TEXTURE_COUNT 4

struct SSampleUtilGlob
{
	CPUTTextureDX11 *codeTextures[CODE_TEXTURE_COUNT];
	CPUTMaterial *codeMaterial;
	CPUTMaterial *meshPreviewMaterial;

	std::string userDataDirectory;
	std::string myAssetsDirectory;

	CPUTScene *boxScene;
	CPUTScene *sphereScene;
	CPUTModel *boxModel;
	CPUTModel *sphereModel;
};

static SSampleUtilGlob gUtilGlob;

const std::string &GetUserDataDirectory()
{
	return gUtilGlob.userDataDirectory;
}

const std::string &GetMyAssetsDirectory()
{
	return gUtilGlob.myAssetsDirectory;
}

void SampleUtil_Init()
{
	std::string dir;
	CPUTFileSystem::GetExecutableDirectory(&dir);
	CPUTFileSystem::StripDirectoriesFromPath(&dir, 4);
	CPUTFileSystem::CombinePath(dir, "Media\\MyAssets", &gUtilGlob.myAssetsDirectory);
	CPUTFileSystem::CombinePath(dir, "userdata", &gUtilGlob.userDataDirectory);

	CPUTAssetLibrary *pAssetLibrary = CPUTAssetLibrary::GetAssetLibrary();
	for (int i = 0; i < CODE_TEXTURE_COUNT; i++)
	{
		char textureName[64];
		snprintf(textureName, sizeof(textureName), "$CODETEXTURE%d", i);
		std::string textureNameString = std::string(textureName);
		gUtilGlob.codeTextures[i] = (CPUTTextureDX11*)CPUTTextureDX11::Create(textureNameString, NULL, NULL);
		pAssetLibrary->AddTexture(textureName, "", "", gUtilGlob.codeTextures[i]);
	}
	gUtilGlob.codeMaterial = pAssetLibrary->GetMaterial("MyAssets/codesprite");

	pAssetLibrary->SetMediaDirectoryName(GetMyAssetsDirectory() + "\\");
	gUtilGlob.meshPreviewMaterial = pAssetLibrary->GetMaterial("meshpreview");


	// Load cube and sphere
	pAssetLibrary->SetRootRelativeMediaDirectory("");

	std::string filename;
	CPUTFileSystem::GetMediaDirectory(&filename);
	CPUTFileSystem::CombinePath(filename, "common.scene", &filename);
	gUtilGlob.boxScene = CPUTScene::Create();
	gUtilGlob.boxScene->LoadScene(filename.c_str());
	gUtilGlob.boxModel = pAssetLibrary->FindModel("pCube1", true);
}

static FILE *gDebugFile = NULL;

void DebugPrintf(char *fmt, ...)
{
	char txt[2048];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(txt, fmt, args);
	va_end(args);
	OutputDebugStringA(txt);
	if (gDebugFile != NULL)
	{
		fprintf(gDebugFile, txt);
	}
}

void DebugPrintBeginFileCapture(const char *filename)
{
	if ( gDebugFile == NULL )
		gDebugFile = fopen(filename, "w");
}

void DebugPrintEndFileCapture()
{
	if (gDebugFile != NULL)
	{
		fclose(gDebugFile);
		gDebugFile = NULL;
	}
}

uint64 Sys_GetTicks()
{
	LARGE_INTEGER outInt;
	QueryPerformanceCounter(&outInt);
	return outInt.QuadPart;
}

uint64 Sys_TicksPerSecond()
{
	LARGE_INTEGER result;
	QueryPerformanceFrequency(&result);
	return result.QuadPart;

}

uint64 Sys_TicksToMicrosecond(uint64 ticks)
{
	return (1000 * 1000 * ticks) / Sys_TicksPerSecond();
}

ProfileBlockScoped::ProfileBlockScoped(std::string blockName)
{
	mBlockName = blockName;
	mStartTime = Sys_GetTicks();
}
ProfileBlockScoped::~ProfileBlockScoped()
{
	uint64 diff = Sys_GetTicks() - mStartTime;
	uint64 us = Sys_TicksToMicrosecond(diff);
	DebugPrintf("Block '%s' : %.3fms", mBlockName.c_str(), (float)us / 1000.0f);
}

void SampleUtil_Shutdown()
{
	SAFE_RELEASE(gUtilGlob.codeMaterial);
	SAFE_DELETE(gUtilGlob.boxScene);
}


bool TextureWrap_Init(SCodeTextureWrap* wrap, D3D11_TEXTURE2D_DESC *desc)
{
	TextureWrap_Release( wrap );
	wrap->Desc = *desc;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc->Format;
	srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	HRESULT hr = CPUT_DX11::GetDevice()->CreateTexture2D(desc, NULL, &wrap->Texture);
	assert(SUCCEEDED(hr));
	hr = CPUT_DX11::GetDevice()->CreateShaderResourceView(wrap->Texture, &srvDesc, &wrap->SRV);
	assert(SUCCEEDED(hr));

	//HRESULT hr = resource->QueryInterface(IID_ID3D11Resource, (void **)&wrap->);

	return true;
}

bool TextureWrap_AdjustSize(SCodeTextureWrap* wrap, int width, int height)
{
	if (wrap->Desc.Width != width || wrap->Desc.Height != height)
	{
		TextureWrap_Release(wrap);
		wrap->Desc.Width = width;
		wrap->Desc.Height = height;
		return TextureWrap_Init(wrap, &wrap->Desc);
	}
	return true;
}

void TextureWrap_Release(SCodeTextureWrap* wrap)
{
	SAFE_RELEASE(wrap->Texture);
	SAFE_RELEASE(wrap->SRV);
}

#ifndef DISABLE_RSSDK
void CopyPXCImageToTexture(PXCImage *srcImage, SCodeTextureWrap *dstTexture)
{
	if (srcImage)
	{
		ID3D11DeviceContext *context = CPUT_DX11::GetContext();
		D3D11_MAPPED_SUBRESOURCE mapData;
		PXCImage::ImageData colorImageData;
		srcImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PixelFormat::PIXEL_FORMAT_RGB24, &colorImageData);
		PXCImage::ImageInfo colorInfo = srcImage->QueryInfo();
		TextureWrap_AdjustSize(dstTexture, colorInfo.width, colorInfo.height);
		context->Map(dstTexture->Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapData);
		uint32 *data = (uint32*)mapData.pData;
		for (int y = 0; y < colorInfo.height; y++)
		{
			byte *srcData = ((byte*)colorImageData.planes[0]) + colorImageData.pitches[0] * y;
			byte *dstData = ((byte*)mapData.pData) + mapData.RowPitch * (colorInfo.height - y - 1);
			for (int x = 0; x < colorInfo.width; x++)
			{

				dstData[0] = srcData[2];
				dstData[1] = srcData[1];
				dstData[2] = srcData[0];
				dstData[3] = 0xff;
				srcData += 3;
				dstData += 4;
			}
		}
		srcImage->ReleaseAccess(&colorImageData);
		context->Unmap(dstTexture->Texture, 0);
	}
}
#endif
#define CODE_TEXTURE_COUNT 4

void SetCodeTexture(int index, ID3D11ShaderResourceView *srv)
{
	assert(index < CODE_TEXTURE_COUNT);
	gUtilGlob.codeTextures[index]->SetTextureAndShaderResourceView(NULL, srv);
}

void SetCodeTexture(int index, SCodeTextureWrap *texture)
{
	assert(index < CODE_TEXTURE_COUNT);
	gUtilGlob.codeTextures[index]->SetTextureAndShaderResourceView(NULL, (texture != NULL) ? texture->SRV : NULL);
}

CPUTMaterial *GetCodeSpriteMaterial()
{
	return gUtilGlob.codeMaterial;
}

CPUTMaterial *GetMeshPreviewMaterial()
{
	return gUtilGlob.meshPreviewMaterial;
}

void DrawCube(CPUTRenderParameters &renderParams, float3 position, float size, CPUTColor4 color)
{
	DrawBox(renderParams, position, float3(size, size, size), color);
}

void DrawBox(CPUTRenderParameters &renderParams, float3 position, float3 size, CPUTColor4 color)
{
	size *= 0.01f;
	float4x4 parentMatrix = float4x4Scale(size) * float4x4Translation(position);
	gUtilGlob.boxModel->SetParentMatrix(parentMatrix);
	gUtilGlob.boxModel->mUserData1 = color.ToFloat4();
	gUtilGlob.boxModel->Render(renderParams, 0);
}
