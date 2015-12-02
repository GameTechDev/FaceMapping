#ifndef __GAME_FACE_SCAN__
#define __GAME_FACE_SCAN__

#ifndef DISABLE_RSSDK

enum FaceStatusDetected
{
	FaceStatusDetected_Detected,
	FaceStatusDetected_NotDetected
};

enum FaceStatusXAxis
{
	FaceStatusXAxis_InRange,
	FaceStatusXAxis_TooFarLeft,
	FaceStatusXAxis_TooFarRight
};

enum FaceStatusYAxis
{
	FaceStatusYAxis_InRange,
	FaceStatusYAxis_TooFarUp,
	FaceStatusYAxis_TooFarDown
};

enum FaceStatusZAxis
{
	FaceStatusZAxis_InRange,
	FaceStatusZAxis_TooClose,
	FaceStatusZAxis_TooFar
};

enum FaceStatusYaw
{
	FaceStatusYaw_InRange,
	FaceStatusYaw_TooFarLeft,
	FaceStatusYaw_TooFarRight,
};

enum FaceStatusPitch
{
	FaceStatusPitch_InRange,
	FaceStatusPitch_TooFarUp,
	FaceStatusPitch_TooFarDown,
};


#include "pxcimage.h"
#include "pxc3dscan.h"

void FaceScan_Init();
void FaceScan_Shutdown();

bool FaceScan_FrameAcquire();
void FaceScan_FrameRelease();

PXCImage *FaceScan_GetColorImage();
PXCImage *FaceScan_GetScanPreviewImage();

void FaceScan_StartScan();
void FaceScan_CancelScan();
void FaceScan_SaveScan(const char *filename);

bool FaceScan_IsScanning();


FaceStatusDetected FaceScan_StatusDetected();
FaceStatusXAxis FaceScan_StatusXAxis();
FaceStatusYAxis FaceScan_StatusYAxis();
FaceStatusZAxis FaceScan_StatusZAxis();
FaceStatusYaw FaceScan_StatusYaw();
FaceStatusPitch FaceScan_StatusPitch();

PXC3DScan::AlertData FaceScan_GetAlertData();

#endif

void FaceScan_MoveScanData(const char *srcObjFilename, const char *dstObjFilename);

#endif // #ifndef __GAME_FACE_SCAN__