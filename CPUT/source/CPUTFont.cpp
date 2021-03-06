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
#include "CPUTFont.h"
#include "CPUTOSServices.h"
#include <algorithm>

CPUTFont *CPUTFont::Create(const std::string& FontName, const std::string& AbsolutePathAndFilename)
{
    CPUTFont *pFont;
    pFont = CPUTFont::LoadBMFont( FontName, AbsolutePathAndFilename );
    return pFont;
}

CPUTFont::CPUTFont() :
    mpFontInfo(NULL),
    mpFontCommon(NULL),
    mpFontPages(NULL),
    mpFontChars(NULL),
    mpFontKerningPairs(NULL),
    mNumChars(0),
    mNumKerningPairs(0),
    mFontScale(0.0f)
{
}

//
// Loads a binary font description file generated by BMFont: http://www.angelcode.com/products/bmfont/
// Does not load the texture file as that is handled through the material system
//
CPUTFont *CPUTFont::LoadBMFont(const std::string & FontName, const std::string & AbsolutePathAndFilename)
{
    UINT      fileSize   = 0;
    uint8_t  *pData      = NULL;
    uint32_t  currentPos = 0;
    CPUTFont *pNewFont   = new CPUTFont();
	pNewFont->mFontScale = 1.0f;

    //
    // Load font file contents into a local buffer for processing
    //
    std::string  fontDescriptionFile(AbsolutePathAndFilename);
    CPUTFileSystem::ReadFileContents(fontDescriptionFile.c_str(), &fileSize, (void **) &pData, false, true);

    //
    // Validate the header info for the file
    //
    uint8_t fileIdentifier[3];

    fileIdentifier[0] = *(pData + currentPos++);
    fileIdentifier[1] = *(pData + currentPos++);
    fileIdentifier[2] = *(pData + currentPos++);

    if ((fileIdentifier[0] != 66) || (fileIdentifier[1] != 77) || (fileIdentifier[2] != 70)) {
        DEBUG_PRINT("File Identifier not valid for a BMFont binary file.\n");
        return NULL;
    }

    uint8_t version;
    version = *(pData + currentPos++);
    if (version != 3) {
        DEBUG_PRINT("BMFont file is not the correct version. Must be version 3.\n");
        return NULL;
    }

    //
    // Load the various blocks of the BMFont file
    //
    while (currentPos < fileSize) {
        uint8_t  blockType;
        uint32_t blockSize;
        blockType = *(pData + currentPos++);
        memcpy(&blockSize, pData + currentPos, 4);
        currentPos += 4;

        switch (blockType) {
        case 1:
            pNewFont->mpFontInfo = (BMFontInfo *) malloc(blockSize);

			if (pNewFont->mpFontInfo == 0) {
				return 0;
			}

            memcpy(pNewFont->mpFontInfo, pData + currentPos, blockSize);
            currentPos += blockSize;
            break;
        case 2:
            pNewFont->mpFontCommon = (BMFontCommon *) malloc(blockSize);

			if (pNewFont->mpFontCommon == 0) {
				return 0;
			}

            memcpy(pNewFont->mpFontCommon, pData + currentPos, blockSize);
            currentPos += blockSize;
            break;
        case 3:
            pNewFont->mpFontPages = (BMFontPages *) malloc(blockSize);

			if (pNewFont->mpFontPages == 0) {
				return 0;
			}

            memcpy(pNewFont->mpFontPages, pData + currentPos, blockSize);
            currentPos += blockSize;
            break;
        case 4:
            pNewFont->mNumChars = blockSize / 20; // could do a check to make sure the block size is evenly divided by 20
            pNewFont->mpFontChars = (BMFontChars *) malloc(blockSize);

			if (pNewFont->mpFontChars == 0) {
				return 0;
			}

            memcpy(pNewFont->mpFontChars, pData + currentPos, blockSize);
            currentPos += blockSize;
            break;
        case 5:
            pNewFont->mNumKerningPairs = blockSize / 10; // could do a check to make sure the block size is evenly divided by 10
            pNewFont->mpFontKerningPairs = (BMFontKerningPairs *) malloc(blockSize);

			if (pNewFont->mpFontKerningPairs == 0) {
				return 0;
			}

            memcpy(pNewFont->mpFontKerningPairs, pData + currentPos, blockSize);
            currentPos += blockSize;
            break;
        default:
            DEBUG_PRINT("Invalid block type\n");
            break;
        }
    }

    delete pData;

    return pNewFont;
}

void CPUTFont::LayoutText(CPUTGUIVertex *pVtxBuffer, int *pWidth, int *pHeight, const std::string& text, int tlx, int tly)
{
    int currentLine = 0;
    int x = tlx, y = tly;
    BMFontChars *pPreviousChar = NULL;

    *pWidth = x - tlx;

    for (uint32_t j = 0, index = 0; j < text.length(); j++) {
        int charIndex = -1;
        for (uint32_t i = 0; i < mNumChars; i++) {
            if (((BMFontChars *)(((uint8_t *)(mpFontChars)) + (20 * i)))->id == text[j]) {
                charIndex = i;
            }
        }

        if (charIndex == -1)
        {
            // Handle special characters
            switch (text[j])
            {
                // Line feed
                case '\n':
                    *pWidth = std::max(*pWidth, x - tlx);
                    x = tlx;
                    currentLine++;
                    continue;

                // Carriage return
                case '\r':
                    currentLine++;
                    continue;

                default:
                    DEBUG_PRINT("Invalid character being searched for value: %d, char: %c\n", (int)text[j], text[j]);
            }
        }

        BMFontChars *pChar = ((BMFontChars *)(((uint8_t *)(mpFontChars)) + (20 * charIndex)));

        int kernAmount = 0;
        if (pPreviousChar != NULL) {
            for (uint32_t i = 0; i < mNumKerningPairs; i++) {
                if ((((BMFontKerningPairs *)(((uint8_t *)(mpFontKerningPairs)) + (10 * i)))->first == pPreviousChar->id) &&
                   (((BMFontKerningPairs *)(((uint8_t *)(mpFontKerningPairs)) + (10 * i)))->second == pChar->id)) {
                    kernAmount = ((BMFontKerningPairs *)(((uint8_t *)(mpFontKerningPairs)) + (10 * i)))->amount;
                }
            }
        }
        pPreviousChar = pChar;

        GUIColor fontColor;
        fontColor.r = fontColor.a = fontColor.g = fontColor.b = 1.0f;

        float texWidth = (float) mpFontCommon->scaleW;
        float texHeight = (float) mpFontCommon->scaleH;
        if(pVtxBuffer)
        {
            pVtxBuffer[index+0].Pos = float3((float)(x + mFontScale * (0 + pChar->xoffset + kernAmount)), (float)(y + mFontScale * (0 + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+0].UV    = float2(pChar->x / texWidth, pChar->y / texHeight);
            pVtxBuffer[index+0].Color = fontColor;

            pVtxBuffer[index+1].Pos = float3((float)(x + mFontScale * (pChar->width + pChar->xoffset + kernAmount)), float(y + mFontScale * (0 + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+1].UV    = float2((pChar->x + pChar->width) / texWidth, pChar->y / texHeight);
            pVtxBuffer[index+1].Color = fontColor;

            pVtxBuffer[index+2].Pos = float3((float)(x + mFontScale * (0.0f + pChar->xoffset + kernAmount)), (float)(y + mFontScale * (pChar->height + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+2].UV    = float2(pChar->x / texWidth, (pChar->y + pChar->height) / texHeight);
            pVtxBuffer[index+2].Color = fontColor;

            pVtxBuffer[index+3].Pos = float3((float)(x + mFontScale * (pChar->width + pChar->xoffset + kernAmount)), (float)(y + mFontScale * (0 + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+3].UV    = float2((pChar->x + pChar->width) / texWidth, pChar->y / texHeight);
            pVtxBuffer[index+3].Color = fontColor;

            pVtxBuffer[index+4].Pos = float3((float)(x + mFontScale * (pChar->width + pChar->xoffset + kernAmount)), (float)(y + mFontScale * (pChar->height + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+4].UV    = float2((pChar->x + pChar->width) / texWidth, (pChar->y + pChar->height) / texHeight);
            pVtxBuffer[index+4].Color = fontColor;

            pVtxBuffer[index+5].Pos = float3((float)(x + mFontScale * (0 + pChar->xoffset + kernAmount)), (float)(y + mFontScale * (pChar->height + pChar->yoffset) + mpFontCommon->lineHeight * mFontScale * currentLine), 1.0f);
            pVtxBuffer[index+5].UV    = float2(pChar->x / texWidth, (pChar->y + pChar->height) / texHeight);
            pVtxBuffer[index+5].Color = fontColor;
        }
        x += int((pChar->xadvance + kernAmount) * mFontScale);
        index += 6;
    }

    *pWidth = std::max(*pWidth, x - tlx);
    *pHeight = int(mpFontCommon->lineHeight * mFontScale * (currentLine + 1));
}

CPUTFont::~CPUTFont()
{
    SAFE_DELETE(mpFontInfo);
    SAFE_DELETE(mpFontCommon);
    SAFE_DELETE(mpFontPages);
    SAFE_DELETE(mpFontChars);
    SAFE_DELETE(mpFontKerningPairs);
}

