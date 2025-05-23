/*
TMPCLASS.HPP

Temporary panel plugin class header file

*/

#ifndef __TMPCLASS_HPP__
#define __TMPCLASS_HPP__

#define REMOVE_FLAG 1

#include <farplug-wide.h>

class TmpPanel
{
private:
	void RemoveDups();
	void RemoveEmptyItems();
	void UpdateItems(int ShowOwners, int ShowGroups, int ShowLinks);
	int IsOwnersDisplayed(LPCTSTR ColumnTypes);
	int IsGroupsDisplayed(LPCTSTR ColumnTypes);
	int IsLinksDisplayed(LPCTSTR ColumnTypes);
	void ProcessRemoveKey();
	void ProcessSaveListKey();
	void ProcessPanelSwitchMenu();
	void SwitchToPanel(int NewPanelIndex);
	void FindSearchResultsPanel();
	void SaveListFile(const wchar_t *Path);
	bool IsCurrentFileCorrect(wchar_t **pCurFileName);

	PluginPanelItem *TmpPanelItem;
	int TmpItemsNumber;
	int LastOwnersRead;
	int LastGroupsRead;
	int LastLinksRead;
	int UpdateNotNeeded;
	wchar_t* HostFile;

public:
	TmpPanel(const wchar_t *pHostFile = nullptr);
	~TmpPanel();
	int PanelIndex;
	//    int OpenFrom;
	int GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
	void GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int SetDirectory(const wchar_t *Dir, int OpMode);

	int
	PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
	HANDLE BeginPutFiles();
	void CommitPutFiles(HANDLE hRestoreScreen, int Success);
	int PutDirectoryContents(const wchar_t *Path);
	int PutOneFile(const wchar_t *SrcPath, PluginPanelItem &PanelItem);
	int PutOneFile(const wchar_t *FilePath);

	int SetFindList(const struct PluginPanelItem *PanelItem, int ItemsNumber);
	int ProcessEvent(int Event, void *Param);
	int ProcessKey(int Key, unsigned int ControlState);
	static bool GetFileInfoAndValidate(const wchar_t *FilePath, FAR_FIND_DATA *FindData, int Any);
	void IfOptCommonPanel(void);
};

#endif	/* __TMPCLASS_HPP__ */
