#ifndef __MENU_CONTROLLER__
#define __MENU_CONTROLLER__

#include "MenuBase.h"

class CPUTControl;

void MenuController_Init();

void MenuController_PushMenu(MenuBase *menu, bool deferToUpdate=true);

void MenuController_PopMenu(bool deferToUpdate = true);
void MenuController_PopTo(MenuBase *menu, bool deferToUpdate = true);

void MenuController_Render(CPUTRenderParameters &renderParams);
void MenuController_Update(float dt);

void MenuController_Resize(int width, int height);

void MenuController_HandleCPUTEvent(int event, int controlID, CPUTControl * pControl);
CPUTEventHandledCode MenuController_HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message);

#endif