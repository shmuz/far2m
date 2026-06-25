#pragma once

/*
filelist.hpp

Файловая панель - общие функции
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

#include "list"
#include "panel.hpp"
#include "dizlist.hpp"
#include "filefilterparams.hpp"
#include "plugins.hpp"
#include "ConfigRW.hpp"
#include "FSNotify.h"
#include <memory>
#include <map>

extern const HighlightDataColor ZeroColors;

class FileFilter;

enum sort_order
{
	first,

	flip_or_default = first,
	keep,
	ascend,
	descend,

	last = descend
};

struct FileListItem
{
	FileListItem(const FileListItem &)            = delete;
	FileListItem &operator=(const FileListItem &) = delete;

	FileListItem() {}

	FARString strName;
	FARString strOwner, strGroup;
	FARString strCustomData;

	uint64_t FileSize{};
	uint64_t PhysicalSize{};

	FILETIME CreationTime{};
	FILETIME AccessTime{};
	FILETIME WriteTime{};
	FILETIME ChangeTime{};

	wchar_t *DizText{};
	wchar_t **CustomColumnData{};

	DWORD_PTR UserData{};

	const HighlightDataColor *ColorsPtr = &ZeroColors;

	DWORD NumberOfLinks{};
	DWORD UserFlags{};
	DWORD FileAttr{};
	DWORD FileMode{};
	DWORD CRC32{};

	int Position{};
	int SortGroup{};
	int CustomColumnNumber{};

	bool Selected{};
	bool PrevSelected{};
	bool DeleteDiz{};
	uint8_t ShowFolderSize{};

	/// temporary values used to optimize sorting, they fit into
	/// 8-bytes alignment gap so there is no memory waisted
	unsigned short FileNamePos{};    // offset from beginning of StrName
	unsigned short FileExtPos{};     // offset from FileNamePos
};

struct PluginsListItem
{
	PHPTR hPlugin;
	FARString strHostFile;
	FARString strPrevOriginalCurDir;
	std::map<FARString, FARString> Dir2CursorFile;
	bool Modified;
	int PrevViewMode;
	int PrevSortMode;
	int PrevSortOrder;
	int PrevNumericSort;
	int PrevCaseSensitiveSort;
	int PrevDirectoriesFirst;
	PanelViewSettings PrevViewSettings;
};

struct PrevDataItem
{
	FileListItem **PrevListData;
	int PrevFileCount;
	FARString strPrevName;
	int PrevTopFile;
};

class FileList : public Panel
{
private:
	FileFilter *Filter;
	DizList Diz;
	bool DizRead;
	/* $ 09.11.2001 IS
		 Открывающий и закрывающий символ, которые используются для показа
		 имени, которое не помещается в панели. По умолчанию - фигурные скобки.
	*/
	wchar_t openBracket[2], closeBracket[2];

	FARString strOriginalCurDir;
	FARString strPluginDizName;
	FileListItem **ListData;
	std::map<std::wstring, FARString> SymlinksCache;
	int FileCount;
	PHPTR hPlugin;
	std::list<PrevDataItem *> PrevDataList;
	std::list<PluginsListItem *> PluginsList;
	std::unique_ptr<IFSNotify> ListChange;
	long UpperFolderTopFile, LastCurFile;
	bool ReturnCurrentFile;
	long SelFileCount;
	long GetSelPosition, LastSelPosition;
	long TotalFileCount;
	uint64_t SelFileSize;
	uint64_t TotalFileSize;
	uint64_t FreeDiskSize;
	size_t MarkLM;
	clock_t LastUpdateTime;
	int Height, Columns;

	int ColumnsInGlobal;

	int LeftPos;
	int ShiftSelection;
	int MouseSelection;
	int SelectedFirst;
	bool IsEmpty;    // указывает на полностью пустую колонку
	bool AccessTimeUpdateRequired;

	bool UpdateRequired;
	int UpdateRequiredMode, UpdateDisabled;
	bool SortGroupsRead;
	int InternalProcessKey;

	long CacheSelIndex, CacheSelPos;
	long CacheSelClearIndex, CacheSelClearPos;

	wchar_t CustomSortIndicator[2];
	std::vector<PluginPanelItem> SelItems;

private:
	void SetSelectedFirstMode(int Mode) override;
	int GetSelectedFirstMode() override { return SelectedFirst; }
	void DisplayObject() override;
	static void DeleteListData(FileListItem **(&ListData), int &FileCount);
	void Up(int Count);
	void Down(int Count);
	void Scroll(int Count);
	void CorrectPosition();
	void ShowFileList(bool Fast);
	void ShowList(bool ShowStatus, int StartColumn, OpenPluginInfo &Info);
	void SetShowColor(int Position, int ColorType = HIGHLIGHTCOLORTYPE_FILE);
	DWORD64 GetShowColor(int Position, int ColorType);
	void ShowSelectedSize();
	void ShowTotalSize(OpenPluginInfo &Info);
	bool ResolveSymlink(FARString &target_path, const wchar_t *link_name, FileListItem *fi);
	int ConvertName(FARString &strDest, const wchar_t *SrcName, int MaxLength, bool RightAlign,
			bool ShowStatus, DWORD dwFileAttr, FileListItem *fi);

	void Select(FileListItem *SelPtr, bool Selection);
	long SelectFiles(int Mode, const wchar_t *Mask = nullptr);
	void ProcessEnter(bool EnableExec, bool SeparateWindow, bool EnableAssoc = true, bool RunAs = false,
			OPENFILEPLUGINTYPE Type = OFP_NORMAL);
	// ChangeDir возвращает FALSE, eсли не смогла выставить заданный путь
	bool ChangeDir(const wchar_t *NewDir, bool ShowMessage = true);
	void CountDirSize(DWORD PluginFlags);
	/* $ 19.03.2002 DJ
	   IgnoreVisible - обновить, даже если панель невидима
	*/
	void ReadFileNames(int KeepSelection, int IgnoreVisible, int DrawMessage, int CanBeAnnoying);
	void UpdatePlugin(int KeepSelection, int IgnoreVisible);

	void MoveSelection(FileListItem **FileList, long FileCount, FileListItem **OldList, long OldFileCount);
	int GetSelCount() const override;
	bool GetSelName(FARString *strName, DWORD &FileAttr, DWORD &FileMode, FAR_FIND_DATA_EX *fde = nullptr) override;
	void UngetSelName() override;
	void ClearLastGetSelection() override;

	uint64_t GetLastSelectedSize() override;
	const FileListItem *GetLastSelectedItem();

	bool GetCurName(FARString &strName) override;
	bool GetCurBaseName(FARString &strName) override;

	void PushPlugin(const wchar_t *HostFile);
	bool PopPlugin(bool EnableRestoreViewMode);
	void CopyFiles();
	void CopyNames(bool FullPathName, bool RealName);
	void SelectSortMode();
	bool ApplyCommand();
	void DescribeFiles();
	void CreatePluginItemList(bool AddTwoDot = true);
	void DeletePluginItemList();
	PHPTR OpenPluginForFile(const wchar_t *FileName, DWORD FileAttr, OPENFILEPLUGINTYPE Type);
	int PreparePanelView(PanelViewSettings *PanelView);
	int PrepareColumnWidths(std::vector<Column> &Columns, bool FullScreen);
	void PrepareViewSettings(int ViewMode, OpenPluginInfo *PlugInfo);

	void PluginDelete();
	void PutDizToPlugin(FileList *DestPanel, bool Delete, bool Move, DizList *SrcDiz, DizList *DestDiz);
	void PluginGetFiles(const wchar_t **DestPath, bool Move);
	void PluginToPluginFiles(bool Move);
	void PluginHostGetFiles();
	void PluginPutFilesToNew();
	// возвращает то, что возвращает PutFiles
	int PluginPutFilesToAnother(bool Move, Panel *AnotherPanel);
	void ProcessPluginCommand();
	void PluginClearSelection();
	void ProcessCopyKeys(FarKey Key);
	void ReadSortGroups(bool UpdateFilterCurrentTime = true);
	FileListItem *
	AddParentPoint(long CurFilePos, FILETIME *Times = nullptr, FARString Owner = L"", FARString Group = L"");
	int ProcessOneHostFile(int Idx);

protected:
	void ClearAllItem() override;

public:
	FileList();
	~FileList() override;

public:
	int ProcessKey(FarKey Key) override;
	int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	int64_t VMProcess(int OpCode, void *vParam = nullptr, int64_t iParam = 0) override;
	void MoveToMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	void SetFocus() override;
	void Update(int Mode) override;
	/*$ 22.06.2001 SKV
	  Параметр для игнорирования времени последнего Update.
	  Используется для Update после исполнения команды.
	*/
	bool UpdateIfChanged(int UpdateMode) override;

	/* $ 19.03.2002 DJ
	   UpdateIfRequired() - обновить, если апдейт был пропущен из-за того,
	   что панель невидима
	*/
	void UpdateIfRequired() override;

	bool SendKeyToPlugin(FarKey Key, bool Pred = false) override;
	void CreateChangeNotification(bool CheckTree);
	void CloseChangeNotification() override;
	void SortFileList(bool KeepPosition) override;
	void SetViewMode(int ViewMode) override;
	void SetSortMode(int SortMode, bool KeepOrder = false) override;
	void ApplySortMode(int SortMode);
	void SetCustomSortMode(int Mode, int Order, bool InvertByDefault);
	void ChangeSortOrder(int NewOrder) override;
	void ChangeNumericSort(int Mode) override;
	void ChangeCaseSensitiveSort(int Mode) override;
	void ChangeDirectoriesFirst(int Mode) override;
	bool SetCurDir(const wchar_t *NewDir, bool ClosePlugin, bool ShowMessage = true) override;
	int GetPrevSortMode() override;
	int GetPrevSortOrder() override;
	int GetPrevViewMode() override;
	int GetPrevNumericSort() override;
	int GetPrevCaseSensitiveSort() override;
	int GetPrevDirectoriesFirst() override;

	PHPTR OpenFilePlugin(const wchar_t *FileName, bool PushPrev, OPENFILEPLUGINTYPE Type);
	bool GetFileName(FARString &strName, int Pos, DWORD &FileAttr) const override;
	int GetCurrentPos() const override;
	bool FindPartName(const wchar_t *Name, bool Next, int Direct, bool ExcludeSets,
			bool UseXlat = false) override;

	bool GoToFile(long idxItem) override;
	bool GoToFile(const wchar_t *Name, bool OnlyPartName = false) override;
	long FindFile(const wchar_t *Name, bool OnlyPartName = false) override;

	bool IsSelected(const wchar_t *Name) override;
	bool IsSelected(long idxItem) override;

	long FindFirst(const wchar_t *Name) override;
	long FindNext(int StartPos, const wchar_t *Name) override;

	void ProcessHostFile();
	bool GetPluginInfo(PluginInfo *PInfo);
	void UpdateViewPanel() override;
	void CompareDir() override;
	void ClearSelection() override;
	void SaveSelection() override;
	void RestoreSelection() override;
	void EditFilter() override;
	bool FileInFilter(long idxItem) override;
	void ReadDiz(PluginPanelItem *ItemList = nullptr, int ItemLength = 0, DWORD dwFlags = 0) override;
	void DeleteDiz(const wchar_t *Name) override;
	void FlushDiz() override;
	void GetDizName(FARString &strDizName) override;
	void CopyDiz(const wchar_t *Name, const wchar_t *DestName, DizList *DestDiz) override;
	bool IsFullScreen() const override;
	bool IsDizDisplayed() const override;
	bool IsColumnDisplayed(int Type) const override;
	int GetColumnsCount() const override { return Columns; }
	void GetOpenPluginInfo(OpenPluginInfo *Info) override;
	void SetPluginMode(PHPTR PanHandle, const wchar_t *PluginFile, bool SendOnFocus = false) override;

	void PluginGetPanelInfo(PanelInfo &Info);
	size_t PluginGetPanelItem(int ItemNumber, PluginPanelItem *Item);
	size_t PluginGetSelectedPanelItem(int ItemNumber, PluginPanelItem *Item);
	void PluginGetColumnTypesAndWidths(FARString &strColumnTypes, FARString &strColumnWidths);

	void PluginBeginSelection();
	void PluginSetSelection(int ItemNumber, bool Selection);
	void PluginClearSelection(int SelectedItemNumber);
	void PluginEndSelection();

	void SetPluginModified() override;
	int ProcessPluginEvent(int Event, void *Param = nullptr) override;
	void SetTitle() override;
	// FARString &GetTitle(FARString &Title,int SubLen=-1,int TruncSize=0) override;
	int PluginPanelHelp(PHPTR ph);
	long GetFileCount() const override { return FileCount; }

	FARString &CreateFullPathName(const wchar_t *Name, FARString &strDest, bool RealName);
	FARString &PluginGetURL(const wchar_t *Name, FARString &strDest);

	const FileListItem *GetItem(int Index) const;
	bool UpdateKeyBar() override;

	void ResetLastUpdateTime() { LastUpdateTime = 0; }
	PHPTR GetPluginHandle() const override;
	int GetRealSelCount() const override;
	static void SetFilePanelModes();
	static void SavePanelModes(ConfigWriter &cfg_writer);
	static void ReadPanelModes(ConfigReader &cfg_reader);
	static bool FileNameToPluginItem(const wchar_t *Name, PluginPanelItem *pi);
	static void FileListToPluginItem(const FileListItem *fi, PluginPanelItem *pi);
	static void FreePluginPanelItem(PluginPanelItem *pi);
	static size_t FileListToPluginItem2(const FileListItem *fi, PluginPanelItem *pi);
	static void PluginToFileListItem(const PluginPanelItem *pi, FileListItem *fi);
	static bool IsModeFullScreen(int Mode);
};
