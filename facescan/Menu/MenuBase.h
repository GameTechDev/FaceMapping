#ifndef __MENU_BASE__
#define __MENU_BASE__

#include "CPUTRenderParams.h"
#include "CPUTEventHandler.h"

class CPUTControl;

class MenuBase
{

public:

	MenuBase() {}

	virtual void Resize(int width, int height ) {}

	virtual void Init() {}
	virtual void Shutdown() {}

	virtual void ActivationChanged(bool active) {}
	
	virtual void Render(CPUTRenderParameters &renderParams) {}
	virtual void Update(float dt) {}

	virtual void HandleCPUTEvent(int eventID, int controlID, CPUTControl *control) {}
	virtual CPUTEventHandledCode HandleMouseEvent(int x, int y, int wheel, CPUTMouseState state, CPUTEventID message) { return CPUT_EVENT_UNHANDLED; }

protected:

};

#endif