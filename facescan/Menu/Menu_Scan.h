#ifndef __MENU_SCAN__
#define __MENU_SCAN__

#include "MenuBase.h"
#include "CPUTSprite.h"
#include "../SampleUtil.h"
#include "CPUTText.h"

#ifndef DISABLE_RSSDK

enum ScanState
{
	ScanState_ColorFeed,
	ScanState_Scanning,
	ScanState_ScanComplete,
};

class Menu_Scan : public MenuBase
{
public:

	virtual void Init();
	virtual void Shutdown();

	void Update(float dt);
	void Render(CPUTRenderParameters &renderParams);

	virtual void HandleCPUTEvent(int eventID, int controlID, CPUTControl *control);
	virtual void ActivationChanged(bool active);

private:

	void SetState(ScanState state);

	ScanState mScanState;

	SCodeTextureWrap	CameraColorTexture;
	SCodeTextureWrap	ScanPreviewTexture;

	CPUTSprite *mFullscreenSprite;

	std::vector<CPUTText *> mStatusMessages;
	std::vector<CPUTText *> mHeadMessages;

	CPUTButton *mBeginScanButton;
	
	void AddMessageControls();
};

#else
class Menu_Scan : public MenuBase
{

};
#endif


#endif