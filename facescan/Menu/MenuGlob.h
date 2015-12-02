#ifndef __MENU_GLOB__
#define __MENU_GLOB__

#include "Menu_Scan.h"
#include "Menu_FaceScanPreview.h"
#include "Menu_FaceMapping.h"

#define MENU_CPUT_PANEL_ID 100

extern Menu_Scan gMenu_Scan;
extern Menu_FaceScanPreview gMenu_FaceScanPreview;
extern Menu_FaceMapping gMenu_FaceMapping;

void MenuGlob_Init();

class CPUTGuiController;
CPUTGuiController *MenuGlob_GUI();

void MenuGlob_GetScreenDim(int *width, int *height);
void MenuGlob_SetScreenDim(int width, int height);

#endif