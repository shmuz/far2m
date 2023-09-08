#ifndef __FARPANEL_H__
#define __FARPANEL_H__

#include <farplug-wide.h>
#include <memory>
#include "plugincfg.h"

#define NO_PANEL_INDEX (PanelIndex)(-1)

struct PanelData {
	struct PanelMode panelModesArray[PanelModeMax];
	struct KeyBarTitles keyBar;
	struct OpenPluginInfo openInfo;
	PanelData() {
		memset(panelModesArray, 0, sizeof(panelModesArray));
		memset(&keyBar, 0, sizeof(keyBar));
		memset(&openInfo, 0, sizeof(openInfo));
		openInfo.StructSize = sizeof(OpenPluginInfo);
		openInfo.PanelModesArray = panelModesArray;
		openInfo.PanelModesNumber = ARRAYSIZE(panelModesArray);
		openInfo.KeyBar = &keyBar;
	};
};

class FarPanel: public PluginCfg {
private:
	PanelIndex index;
	std::unique_ptr<PanelData> data;
public:
	virtual int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) = 0;
	virtual int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) = 0;
	virtual void GetOpenPluginInfo(struct OpenPluginInfo * info);
	virtual void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber);

	// towstr can use only for ASCII symbols
	std::wstring towstr(const char * name) const;
	std::string tostr(const wchar_t * name) const;

	const wchar_t * DublicateCountString(int64_t value) const;
	const wchar_t * DublicateFileSizeString(uint64_t value) const;

	void GetPanelInfo(PanelInfo & pi);
	const wchar_t * GetPanelTitle(void);
	const wchar_t * GetPanelTitleKey(int key, unsigned int controlState = 0) const;
	bool IsPanelProcessKey(int key, unsigned int controlState) const;

	PluginPanelItem * GetPanelItem(intptr_t itemNum) const;
	void FreePanelItem(PluginPanelItem * ppi) const;
	PluginPanelItem * GetCurrentPanelItem(PanelInfo * piret = nullptr) const;
	PluginPanelItem * GetSelectedPanelItem(intptr_t selectedItemNum);
	const wchar_t * GetMsg(int msgId) const;
	int Select(HANDLE hDlg, const wchar_t ** elements, int count, uint32_t setIndex);
	int SelectNum(HANDLE hDlg, const wchar_t ** elements, int count, const wchar_t * subTitle, uint32_t setIndex);

	FarPanel(uint32_t index);
	FarPanel();
	virtual ~FarPanel();
};

#endif /* __FARPANEL_H__ */
