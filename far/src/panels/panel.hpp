#pragma once

/*
panel.hpp

Parent class для панелей
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>
#include "scrobj.hpp"
#include "FARString.hpp"
#include "plugins.hpp"

class DizList;

struct Column
{
	unsigned int Type;
	int Width;
	int WidthType;
};

struct PanelViewSettings
{
	std::vector<Column> PanelColumns;
	std::vector<Column> StatusColumns;
	int FullScreen;
	int AlignExtensions;
	int FolderAlignExtensions;
	int FolderUpperCase;
	int FileLowerCase;
	int FileUpperToLowerCase;
};

enum
{
	FILE_PANEL,
	TREE_PANEL,
	QVIEW_PANEL,
	INFO_PANEL
};

enum PanelSortMode
{
	UNSORTED,
	BY_NAME,
	BY_EXT,
	BY_MTIME,
	BY_CTIME,
	BY_ATIME,
	BY_SIZE,
	BY_DIZ,
	BY_OWNER,
	BY_PHYSICALSIZE,
	BY_NUMLINKS,
	BY_FULLNAME,
	BY_CHTIME,
	BY_CUSTOMDATA,

	COUNT,

	BY_USER = 100000
};

enum
{
	VIEW_0 = 0,
	VIEW_1,
	VIEW_2,
	VIEW_3,
	VIEW_4,
	VIEW_5,
	VIEW_6,
	VIEW_7,
	VIEW_8,
	VIEW_9
};

enum
{
	UPDATE_KEEP_SELECTION  = 1,
	UPDATE_SECONDARY       = 2,
	UPDATE_IGNORE_VISIBLE  = 4,
	UPDATE_DRAW_MESSAGE    = 8,
	UPDATE_CAN_BE_ANNOYING = 16
};

enum
{
	NORMAL_PANEL,
	PLUGIN_PANEL
};

enum
{
	DRIVE_DEL_FAIL,
	DRIVE_DEL_SUCCESS,
	DRIVE_DEL_EJECT,
	DRIVE_DEL_NONE
};

enum
{
	UIC_UPDATE_NORMAL,
	UIC_UPDATE_FORCE,
	UIC_UPDATE_FORCE_NOTIFICATION
};

class VMenu;
class Edit;
struct PanelMenuItem;

class Panel : public ScreenObject
{
protected:
	FARString strCurDir;
	int Focus;
	int Type;
	int EnableUpdate;
	int PanelMode;
	int SortMode;
	int SortOrder;
	int SortGroups;
	int PrevViewMode, ViewMode;
	int CurTopFile;
	int CurFile;
	int NumericSort;
	int CaseSensitiveSort;
	int DirectoriesFirst;
	int ModalMode;
	int PluginCommand;
	FARString strPluginParam;

public:
	PanelViewSettings ViewSettings;
	int ProcessingPluginCommand;

private:
	int ChangeDiskMenu(int Pos, int FirstCall);
	int DisconnectDrive(PanelMenuItem *item, VMenu &ChDisk);
	void FastFindShow(int FindX, int FindY);
	void FastFindProcessName(Edit *FindEdit, const wchar_t *Src, FARString &strLastName, FARString &strName);
	void DragMessage(int X, int Y, int Move);
	int OnFCtlSetLocation(const FarPanelLocation *location);

protected:
	bool SetLocation_Directory(const wchar_t *path);
	bool SetLocation_Plugin(bool file_plugin, class Plugin *plugin, const wchar_t *path,
			const wchar_t *host_file, LONG_PTR item);

	void FastFind(int FirstKey);
	void DrawSeparator(int Y);
	void ShowScreensCount() const;
	int IsDragging() const;
	virtual void ClearAllItem() {}

public:
	Panel();
	virtual ~Panel();

public:
	virtual int SendKeyToPlugin(FarKey Key, BOOL Pred = FALSE) { return FALSE; }
	virtual bool SetCurDir(const wchar_t *NewDir, bool ClosePlugin, bool ShowMessage = true);
	virtual void ChangeDirToCurrent();

	virtual int GetCurDir(FARString &strCurDir);

	virtual int GetCurDirPluginAware(FARString &strCurDir, bool AppendHostFile = true);

	virtual int GetSelCount() const { return 0; }
	virtual int GetRealSelCount() const { return 0; }
	virtual int
	GetSelName(FARString *strName, DWORD &FileAttr, DWORD &FileMode, FAR_FIND_DATA_EX *fd = nullptr)
	{
		return FALSE;
	}

	int GetSelNameCompat(FARString *strName, DWORD &FileAttr, FAR_FIND_DATA_EX *fd = nullptr)
	{
		DWORD FileMode = 0;
		return GetSelName(strName, FileAttr, FileMode, fd);
	}

	virtual void UngetSelName() {}
	virtual void ClearLastGetSelection() {}
	virtual uint64_t GetLastSelectedSize() { return (uint64_t)(-1); }

	virtual int GetCurName(FARString &strName);
	virtual int GetCurBaseName(FARString &strName);
	virtual int GetFileName(FARString &strName, int Pos, DWORD &FileAttr) const { return FALSE; }

	virtual int GetCurrentPos() const { return 0; }
	virtual void SetFocus();
	virtual void KillFocus();
	virtual void Update(int Mode) {}
	/*$ 22.06.2001 SKV
	  Параметр для игнорирования времени последнего Update.
	  Используется для Update после исполнения команды.
	*/
	virtual int UpdateIfChanged(int UpdateMode) { return 0; }
	/* $ 19.03.2002 DJ
	   UpdateIfRequired() - обновить, если апдейт был пропущен из-за того,
	   что панель невидима
	*/
	virtual void UpdateIfRequired() {}

	virtual void CloseChangeNotification() {}
	virtual bool
	FindPartName(const wchar_t *Name, int Next, int Direct = 1, int ExcludeSets = 0, bool UseXlat = false)
	{
		return false;
	}
	bool FindPartNameXLat(const wchar_t *Name, int Next, int Direct = 1, int ExcludeSets = 0);

	virtual int GoToFile(long idxItem) { return TRUE; }
	virtual int GoToFile(const wchar_t *Name, BOOL OnlyPartName = FALSE) { return TRUE; }
	virtual long FindFile(const wchar_t *Name, BOOL OnlyPartName = FALSE) { return -1; }

	virtual bool IsSelected(const wchar_t *Name) { return false; }
	virtual bool IsSelected(long indItem) { return false; }

	virtual long FindFirst(const wchar_t *Name) { return -1; }
	virtual long FindNext(int StartPos, const wchar_t *Name) { return -1; }

	virtual void SetSelectedFirstMode(int) {}
	virtual int GetSelectedFirstMode() { return 0; }
	int GetMode() { return (PanelMode); }
	void SetMode(int Mode) { PanelMode = Mode; }
	int GetModalMode() { return (ModalMode); }
	void SetModalMode(int ModalMode) { Panel::ModalMode = ModalMode; }
	int GetViewMode() { return (ViewMode); }
	virtual void SetViewMode(int ViewMode);
	virtual int GetPrevViewMode() { return (PrevViewMode); }
	void SetPrevViewMode(int PrevViewMode) { Panel::PrevViewMode = PrevViewMode; }
	virtual int GetPrevSortMode() { return (SortMode); }
	virtual int GetPrevSortOrder() { return (SortOrder); }
	int GetSortMode() { return (SortMode); }
	virtual int GetPrevNumericSort() { return NumericSort; }
	int GetNumericSort() { return NumericSort; }
	void SetNumericSort(int Mode) { NumericSort = Mode; }
	virtual void ChangeNumericSort(int Mode) { SetNumericSort(Mode); }
	virtual int GetPrevCaseSensitiveSort() { return CaseSensitiveSort; }
	int GetCaseSensitiveSort() { return CaseSensitiveSort; }
	void SetCaseSensitiveSort(int Mode) { CaseSensitiveSort = Mode; }
	virtual void ChangeCaseSensitiveSort(int Mode) { SetCaseSensitiveSort(Mode); }
	virtual int GetPrevDirectoriesFirst() { return DirectoriesFirst; }
	int GetDirectoriesFirst() { return DirectoriesFirst; }
	void SetDirectoriesFirst(int Mode) { DirectoriesFirst = Mode; }
	virtual void ChangeDirectoriesFirst(int Mode) { SetDirectoriesFirst(Mode); }
	virtual void SetSortMode(int SortMode, bool KeepOrder = false) { Panel::SortMode = SortMode; }
	virtual void SetCustomSortMode(int Mode, int Order, bool InvertByDefault) {}
	int GetSortOrder() { return (SortOrder); }
	void SetSortOrder(int SortOrder) { Panel::SortOrder = SortOrder; }
	virtual void ChangeSortOrder(int NewOrder) { SetSortOrder(NewOrder); }
	int GetSortGroups() { return (SortGroups); }
	void SetSortGroups(int SortGroups) { Panel::SortGroups = SortGroups; }
	void InitCurDir(const wchar_t *CurDir);
	virtual void CloseFile() {}
	virtual void UpdateViewPanel() {}
	virtual void CompareDir() {}
	virtual void MoveToMouse(MOUSE_EVENT_RECORD *MouseEvent) {}
	virtual void ClearSelection() {}
	virtual void SaveSelection() {}
	virtual void RestoreSelection() {}
	virtual void SortFileList(bool KeepPosition) {}
	virtual void EditFilter() {}
	virtual bool FileInFilter(long idxItem) { return true; }
	virtual void ReadDiz(struct PluginPanelItem *ItemList = nullptr, int ItemLength = 0, DWORD dwFlags = 0) {}
	virtual void DeleteDiz(const wchar_t *Name) {}
	virtual void GetDizName(FARString &strDizName) {}
	virtual void FlushDiz() {}
	virtual void CopyDiz(const wchar_t *Name, const wchar_t *DestName, DizList *DestDiz) {}
	virtual int IsFullScreen() const { return ViewSettings.FullScreen; }
	virtual int IsDizDisplayed() const { return FALSE; }
	virtual int IsColumnDisplayed(int Type) const { return FALSE; }
	virtual int GetColumnsCount() const { return 1; }
	virtual void QViewDelTempName() {}
	virtual void GetOpenPluginInfo(struct OpenPluginInfo *Info) {}
	virtual void SetPluginMode(PHPTR hPlugin, const wchar_t *PluginFile, bool SendOnFocus = false) {}
	virtual void SetPluginModified() {}
	virtual int ProcessPluginEvent(int Event, void *Param) { return FALSE; }
	virtual PHPTR GetPluginHandle() const { return nullptr; }
	virtual void SetTitle();
	virtual FARString &GetTitle(FARString &Title, int SubLen = -1, int TruncSize = 0);

	virtual int64_t VMProcess(int OpCode, void *vParam = nullptr, int64_t iParam = 0);

	/* $ 30.04.2001 DJ
	   функция вызывается для обновления кейбара; если возвращает FALSE,
	   используется стандартный кейбар
	*/
	virtual BOOL UpdateKeyBar() { return FALSE; }

	virtual long GetFileCount() const { return 0; }

	bool ExecShortcutFolder(int Pos);
	bool SaveShortcutFolder(int Pos);

	static void EndDrag();
	virtual void Hide();
	virtual void Show();
	int SetPluginCommand(int Command, int Param1, LONG_PTR Param2);
	int PanelProcessMouse(MOUSE_EVENT_RECORD *MouseEvent, int &RetCode);
	void ChangeDisk();
	int GetFocus() { return (Focus); }
	int GetType() { return (Type); }
	void SetUpdateMode(int Mode) { EnableUpdate = Mode; }
	bool MakeListFile(FARString &strListFileName, const wchar_t *Modifers = nullptr);
	int SetCurPath();

	BOOL NeedUpdatePanel(Panel *AnotherPanel);
};
