#include "MenuGlob.h"
#include "CPUTGuiControllerDX11.h"
#include "Menu_FaceScanPreview.h"

Menu_Scan gMenu_Scan;
Menu_FaceScanPreview gMenu_FaceScanPreview;
Menu_FaceMapping gMenu_FaceMapping;

static int gScreenWidth = 0;
static int gScreenHeight = 0;

void MenuGlob_Init()
{
	gMenu_Scan.Init();
	gMenu_FaceScanPreview.Init();
	gMenu_FaceMapping.Init();
}

void MenuGlob_Shutdown()
{
	gMenu_Scan.Shutdown();
	gMenu_FaceScanPreview.Shutdown();
	gMenu_FaceMapping.Shutdown();
}

CPUTGuiController *MenuGlob_GUI()
{
	return (CPUTGuiController*)CPUTGuiControllerDX11::GetController();
}

void MenuGlob_GetScreenDim(int *width, int *height)
{
	*width = gScreenWidth;
	*height = gScreenHeight;
}

void MenuGlob_SetScreenDim(int width, int height)
{
	gScreenWidth = width;
	gScreenHeight = height;
}