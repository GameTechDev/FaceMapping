/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

// Control Texture Color Coding:
// R - Weight for depth displacement
// G - Weight for color mapping
// B - Weight for jawbone area

// ********************************************************************************************************
Texture2D  DisplacementMap;
Texture2D  Albedo;
Texture2D ControlMap;
Texture2D SkinDetailMap;
Texture2D SkinFeatureMap;
Texture2D ColorTransfer;
Texture2D  _Shadow;

SamplerState           SAMPLER0 : register(s0);
SamplerComparisonState SAMPLER1 : register(s1);
SamplerState           SAMPLER2 : register(s2);

// ********************************************************************************************************
cbuffer cbPerModelValues
{
	row_major float4x4 World : WORLD;
	row_major float4x4 NormalMatrix : WORLD;
	row_major float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
	row_major float4x4 InverseWorld : WORLDINVERSE;
	row_major float4x4 LightWorldViewProjection;
	float4   BoundingBoxCenterWorldSpace  < string UIWidget = "None"; >;
	float4   BoundingBoxHalfWorldSpace    < string UIWidget = "None"; >;
	float4   BoundingBoxCenterObjectSpace < string UIWidget = "None"; >;
	float4   BoundingBoxHalfObjectSpace   < string UIWidget = "None"; >;
	float4 UserData1; // displacement constants
	float4 UserData2; // skin color
	float4 UserData3;
};

// ********************************************************************************************************
cbuffer cbPerFrameValues
{
	row_major float4x4  View;
	row_major float4x4  InverseView : ViewInverse	< string UIWidget = "None"; >;
	row_major float4x4  Projection;
	row_major float4x4  ViewProjection;
	float4    AmbientColor < string UIWidget = "None"; > = .20;
	float4    LightColor < string UIWidget = "None"; > = 1.0f;
	float4    LightDirection  : Direction < string UIName = "Light Direction";  string Object = "TargetLight"; string Space = "World"; int Ref_ID = 0; > = { 0, 0, -1, 0 };
	float4    EyePosition;
	float4    TotalTimeInSeconds < string UIWidget = "None"; >;
};

// ********************************************************************************************************
struct VS_INPUT
{
	float3 Pos      : POSITION; // Projected position
	float2 Uv       : TEXCOORD0;
	float3 Norm     : NORMAL;
#ifdef FACEMAP_PREBUILD
	uint VertexId   : SV_VERTEXID;
#endif
};

struct PS_INPUT
{
	float4 Pos      : SV_POSITION;
	float2 ProjUV   : TEXCOORD0; // UV on the prerendered color map
	float2 Uvs      : TEXCOORD1; // head mesh UVs
#ifdef FACEMAP_PREBUILD
	float4 Disp     : TEXCOORD2; //displaced pos for Stream Out
#else
	float3 Norm     : NORMAL;
#endif
};

float RemapRange(float value, float r1Min, float r1Max, float r2Min, float r2Max)
{
	//value = clamp(value, 0.0f, 1.0f);
	float ratio = (value - r1Min) / (r1Max - r1Min);
	ratio = clamp(ratio, 0.0f, 1.0f);
	return r2Min + ratio * (r2Max - r2Min);
}

// ********************************************************************************************************
PS_INPUT VSMain(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;

	float3 objectPos = input.Pos;

	// convert the object position to the generate map texture coordinates.
	// LightWorldViewProjection is the map projection matrix calculated in code
	float2 projectedUV = mul(float4(objectPos, 1.0f), LightWorldViewProjection).xy;
	projectedUV = float2(0.5f + projectedUV.x / 2.0f, 0.5f - projectedUV.y / 2.0f);

	// Calculate the displaced Z based on the control ranges passed in UserData1
	float ExtrudeMinZ = UserData1.x;
	float ExtrudeMaxZ = UserData1.y;
	float DepthMapRangeMin = UserData1.z;
	float DepthMapRangeMax = UserData1.w;
	float depthMapSamp = DisplacementMap.SampleLevel(SAMPLER0, projectedUV, 0).x;

	float displacedZ = RemapRange(depthMapSamp, DepthMapRangeMin, DepthMapRangeMax, ExtrudeMinZ, ExtrudeMaxZ);

	float4 controlWeight = ControlMap.SampleLevel(SAMPLER0, input.Uv, 0);

	// Blend between displaced Z and default model z based on the control texture's red value
	objectPos.z = lerp(objectPos.z, displacedZ, controlWeight.r);
	
	output.ProjUV = projectedUV;
	output.Uvs = input.Uv;

#ifdef FACEMAP_PREBUILD
	output.Disp.x = objectPos.z;
	output.Disp.y = input.VertexId;
	output.Disp.z = 0.0;
	output.Disp.w = 0.0;
	output.Pos.x = input.Uv.x * 2.0 - 1.0;
	output.Pos.y = (1.0 - input.Uv.y) * 2.0 - 1.0;
	output.Pos.z = 0.5;
	output.Pos.w = 1.0;
#else
	output.Pos = mul(float4(objectPos, 1.0f), WorldViewProjection);
	output.Norm = mul((float3x3)World, input.Norm);
#endif

	return output;
}

// ********************************************************************************************************
float4 PSMain(PS_INPUT input) : SV_Target
{
	float detailMapUVScale = 20.0f;

	float3 controlWeight = ControlMap.Sample(SAMPLER0, input.Uvs.xy);
	float3 skinFeatureSample = SkinFeatureMap.Sample(SAMPLER2, input.Uvs.xy);
	float3 skinDetailSample = SkinDetailMap.Sample(SAMPLER2, input.Uvs.xy * detailMapUVScale);
	float3 faceAlbedo = Albedo.Sample(SAMPLER0, input.ProjUV).rgb;

	float3 colorTransfer = ColorTransfer.Sample(SAMPLER0, input.Uvs.xy).rgb;

	float3 baseColor1 = UserData2;
	float3 baseColor2 = UserData3;
	float3 baseColor = lerp(baseColor1, baseColor2, colorTransfer.r);

	// multiple blend detail maps. 0.5f color maps to 1.0f (multiple by 2)
	baseColor = baseColor * (skinFeatureSample * 2.0f) * (skinDetailSample * 2.0f);

	// blend between player's face color and model color based on cotrol weight
	float3 diffuse = lerp(baseColor, faceAlbedo, controlWeight.g);

#ifdef FACEMAP_PREBUILD
	return float4(diffuse, 1.0f).bgra;
#else
	float3 normal = normalize(input.Norm);
	float  nDotL = dot(normal, -normalize(LightDirection.xyz));
	float3 dirLightWeight = LightColor * saturate( nDotL );
	float3 ambLightWeight = AmbientColor;
	return float4(diffuse * (dirLightWeight+ambLightWeight), 1.0f);
#endif
	
}
