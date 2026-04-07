/*
ConfigOpt.cpp

Конфигурация
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

#include "headers.hpp"

#include "AllXLats.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "ConfigOpt.hpp"
#include "ConfigRW.hpp"
#include "ctrlobj.hpp"
#include "datetime.hpp"
#include "dialog.hpp"
#include "filefilter.hpp"
#include "filelist.hpp"
#include "filepanels.hpp"
#include "findfile.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "farcolors.hpp"
#include "panelmix.hpp"
#include "poscache.hpp"
#include "pick_color256.hpp"
#include "pick_colorRGB.hpp"
#include "strmix.hpp"
#include "MaskGroups.hpp"

void SanitizeHistoryCounts();
void SanitizeIndentationCounts();

static bool g_config_ready = false;

// Стандартный набор разделителей
static const wchar_t *WordDiv0 = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";

// Стандартный набор разделителей для функции Xlat
static const wchar_t *WordDivForXlat0=L" \t!#$%^&*()+|=\\/@?";

static FARString strKeyNameConsoleDetachKey;
static const wchar_t szCtrlDot[]=L"Ctrl.";
static const wchar_t szCtrlShiftDot[]=L"CtrlShift.";

// KeyName
static const char NParamHistoryCount[] = "HistoryCount";
static const char NSecCmdline[] = "Cmdline";
static const char NSecCodePages[] = "CodePages";
static const char NSecColors[] = "Colors";
static const char NSecConfirmations[] = "Confirmations";
static const char NSecDescriptions[] = "Descriptions";
static const char NSecDialog[] = "Dialog";
static const char NSecEditor[] = "Editor";
static const char NSecHelp[] = "Help";
static const char NSecInterface[] = "Interface";
static const char NSecInterfaceCompletion[] = "Interface/Completion";
static const char NSecLanguage[] = "Language";
static const char NSecLayout[] = "Layout";
static const char NSecMacros[] = "Macros";
static const char NSecNotifications[] = "Notifications";
static const char NSecPanel[] = "Panel";
static const char NSecPanelLayout[] = "Panel/Layout";
static const char NSecPanelLeft[] = "Panel/Left";
static const char NSecPanelRight[] = "Panel/Right";
static const char NSecPanelTree[] = "Panel/Tree";
static const char NSecPluginConfirmations[] = "PluginConfirmations";
static const char NSecSavedDialogHistory[] = "SavedDialogHistory";
static const char NSecSavedFolderHistory[] = "SavedFolderHistory";
static const char NSecSavedHistory[] = "SavedHistory";
static const char NSecSavedViewHistory[] = "SavedViewHistory";
static const char NSecScreen[] = "Screen";
static const char NSecSystem[] = "System";
static const char NSecViewer[] = "Viewer";
static const char NSecVMenu[] = "VMenu";
static const char NSecXLat[] = "XLat";

// Структура, описывающая всю конфигурацию(!)
struct FARConfig
{
	int IsSave;   // enum OptSaveType; будет записываться в ConfigOptSave()
	OPT_TYPE ValType;
	const char *KeyName;
	const char *ValName;
	union {
		void      *VoidPtr;   // адрес переменной, куда помещаем данные
		int       *IntPtr;
		DWORD     *DWordPtr;
		bool      *BoolPtr;
		FARString *StrPtr;
	};
	union {
		int   DefInt;
		bool  DefBool;
		DWORD DefDWord;
		DWORD ArrSize;
	};
	union {
	  const wchar_t *DefStr;   // строка по умолчанию
	  const BYTE    *DefArr;   // данные по умолчанию
	};

	constexpr FARConfig(int save, const char *key, const char *val, DWORD size, void *trg, const BYTE *dflt) :
		IsSave(save),ValType(OPT_BINARY),KeyName(key),ValName(val),VoidPtr(trg),ArrSize(size),DefArr(dflt) {}

	constexpr FARConfig(int save, const char *key, const char *val, DWORD *trg, DWORD dflt, OPT_TYPE Type=OPT_DWORD) :
		IsSave(save),ValType(Type),KeyName(key),ValName(val),DWordPtr(trg),DefDWord(dflt),DefStr(nullptr) {}

	constexpr FARConfig(int save, const char *key, const char *val, int *trg, int dflt, OPT_TYPE Type=OPT_DWORD) :
		IsSave(save),ValType(Type),KeyName(key),ValName(val),IntPtr(trg),DefInt(dflt),DefStr(nullptr) {}

	constexpr FARConfig(int save, const char *key, const char *val, bool *trg, bool dflt) :
		IsSave(save),ValType(OPT_REALBOOLEAN),KeyName(key),ValName(val),BoolPtr(trg),DefBool(dflt),DefStr(nullptr) {}

	constexpr FARConfig(int save, const char *key, const char *val, FARString *trg, const wchar_t *dflt) :
		IsSave(save),ValType(OPT_SZ),KeyName(key),ValName(val),StrPtr(trg),DefDWord(0),DefStr(dflt) {}
};

static FARConfig CFG[]
{
	{0x1, NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, nullptr},
	{0x1, NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, g_tempcolorsRGB, nullptr},

	{0x1, NSecScreen, "Clock",                        &Opt.Clock, 1, OPT_BOOLEAN},
	{0x1, NSecScreen, "ViewerEditorClock",            &Opt.ViewerEditorClock, 0, OPT_BOOLEAN},
	{0x1, NSecScreen, "KeyBar",                       &Opt.ShowKeyBar, 1, OPT_BOOLEAN},
	{0x1, NSecScreen, "ScreenSaver",                  &Opt.ScreenSaver, 0, OPT_BOOLEAN},
	{0x1, NSecScreen, "ScreenSaverTime",              &Opt.ScreenSaverTime, 5},
	{0x1, NSecScreen, "CursorBlinkInterval",          &Opt.CursorBlinkTime, 500},

	{0x1, NSecCmdline, "UsePromptFormat",             &Opt.CmdLine.UsePromptFormat, 0, OPT_BOOLEAN},
	{0x1, NSecCmdline, "PromptFormat",                &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{0x1, NSecCmdline, "UseShell",                    &Opt.CmdLine.UseShell, 0, OPT_BOOLEAN},
	{0x1, NSecCmdline, "ShellCmd",                    &Opt.CmdLine.strShell, L"bash -i"},
	{0x1, NSecCmdline, "DelRemovesBlocks",            &Opt.CmdLine.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{0x1, NSecCmdline, "EditBlock",                   &Opt.CmdLine.EditBlock, 0, OPT_BOOLEAN},
	{0x1, NSecCmdline, "AutoComplete",                &Opt.CmdLine.AutoComplete, 1, OPT_BOOLEAN},
	{0x1, NSecCmdline, "Splitter",                    &Opt.CmdLine.Splitter, 1, OPT_BOOLEAN},
	{0x1, NSecCmdline, "WaitKeypress",                &Opt.CmdLine.WaitKeypress, 1},
	{0x1, NSecCmdline, "VTLogLimit",                  &Opt.CmdLine.VTLogLimit, 5000},
	{0x1, NSecCmdline, "ImitateNumpadKeys",           &Opt.CmdLine.ImitateNumpadKeys, 0, OPT_BOOLEAN},
	{0x0, NSecCmdline, "AskOnMultilinePaste",         &Opt.CmdLine.AskOnMultilinePaste, 1, OPT_BOOLEAN},

	{0x1, NSecInterface, "Mouse",                     &Opt.Mouse, 1, OPT_BOOLEAN},
	{0x0, NSecInterface, "UseVk_oem_x",               &Opt.UseVk_oem_x, 1, OPT_BOOLEAN},
	{0x1, NSecInterface, "ShowMenuBar",               &Opt.ShowMenuBar, 0, OPT_BOOLEAN},
	{0x0, NSecInterface, "CursorSize1",               &Opt.CursorSize[0], 15},
	{0x0, NSecInterface, "CursorSize2",               &Opt.CursorSize[1], 10},
	{0x0, NSecInterface, "CursorSize3",               &Opt.CursorSize[2], 99},
	{0x0, NSecInterface, "CursorSize4",               &Opt.CursorSize[3], 99},
	{0x0, NSecInterface, "ShiftsKeyRules",            &Opt.ShiftsKeyRules, 1, OPT_BOOLEAN},
	{0x1, NSecInterface, "CtrlPgUp",                  &Opt.PgUpChangeDisk, 1, OPT_BOOLEAN},

	{0x1, NSecInterface, "ConsolePaintSharp",         &Opt.ConsolePaintSharp, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveCtrlLeft",         &Opt.ExclusiveCtrlLeft, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveCtrlRight",        &Opt.ExclusiveCtrlRight, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveAltLeft",          &Opt.ExclusiveAltLeft, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveAltRight",         &Opt.ExclusiveAltRight, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveWinLeft",          &Opt.ExclusiveWinLeft, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "ExclusiveWinRight",         &Opt.ExclusiveWinRight, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "UseStickyKeyEvent",         &Opt.UseStickyKeyEvent, 0, OPT_BOOLEAN},

	{0x1, NSecInterface, "DateFormat",                &Opt.DateFormat, GetDateFormatDefault()},
	{0x1, NSecInterface, "DateSeparator",             &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{0x1, NSecInterface, "TimeSeparator",             &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{0x1, NSecInterface, "DecimalSeparator",          &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

#if defined(__ANDROID__)
	{0x1, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 1, OPT_BOOLEAN},
#else
	{0x1, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 0, OPT_BOOLEAN},
#endif

	{0x1, NSecInterface, "TTYPaletteOverride",        &Opt.TTYPaletteOverride, 1, OPT_BOOLEAN},
	{0x0, NSecInterface, "ShowTimeoutDelFiles",       &Opt.ShowTimeoutDelFiles, 50},
	{0x0, NSecInterface, "ShowTimeoutDACLFiles",      &Opt.ShowTimeoutDACLFiles, 50},
	{0x0, NSecInterface, "FormatNumberSeparators",    &Opt.FormatNumberSeparators, 0},
	{0x1, NSecInterface, "CopyShowTotal",             &Opt.CMOpt.CopyShowTotal, 1, OPT_BOOLEAN},
	{0x1, NSecInterface, "DelShowTotal",              &Opt.DelOpt.DelShowTotal, 0, OPT_BOOLEAN},
	{0x1, NSecInterface, "WindowTitle",               &Opt.strWindowTitle, L"%State - FAR2M %Ver %Backend %User@%Host"}, // %Platform
	{0x1, NSecInterfaceCompletion, "Exceptions",      &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
	{0x1, NSecInterfaceCompletion, "ShowList",        &Opt.AutoComplete.ShowList, 1, OPT_BOOLEAN},
	{0x1, NSecInterfaceCompletion, "ModalList",       &Opt.AutoComplete.ModalList, 0, OPT_BOOLEAN},
	{0x1, NSecInterfaceCompletion, "Append",          &Opt.AutoComplete.AppendCompletion, 0, OPT_BOOLEAN},

	{0x1, NSecViewer, "ExternalViewerName",           &Opt.strExternalViewer, L""},
	{0x1, NSecViewer, "UseExternalViewer",            &Opt.ViOpt.UseExternalViewer, 0, OPT_BOOLEAN},
	{0x1, NSecViewer, "SaveViewerPos",                &Opt.ViOpt.SavePos, 1, OPT_BOOLEAN},
	{0x1, NSecViewer, "SaveViewerShortPos",           &Opt.ViOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{0x1, NSecViewer, "AutoDetectCodePage",           &Opt.ViOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{0x1, NSecViewer, "SearchRegexp",                 &Opt.ViOpt.SearchRegexp, 0, OPT_BOOLEAN},

	{0x1, NSecViewer, "TabSize",                      &Opt.ViOpt.TabSize, 8},
	{0x1, NSecViewer, "ShowKeyBar",                   &Opt.ViOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{0x1, NSecViewer, "ShowTitleBar",                 &Opt.ViOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{0x1, NSecViewer, "ShowArrows",                   &Opt.ViOpt.ShowArrows, 1, OPT_BOOLEAN},
	{0x1, NSecViewer, "ShowScrollbar",                &Opt.ViOpt.ShowScrollbar, 0, OPT_BOOLEAN},
	{0x1, NSecViewer, "IsWrap",                       &Opt.ViOpt.ViewerIsWrap, 1},
	{0x1, NSecViewer, "Wrap",                         &Opt.ViOpt.ViewerWrap, 0},
	{0x1, NSecViewer, "PersistentBlocks",             &Opt.ViOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{0x1, NSecViewer, "DefaultCodePage",              &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{0x1, NSecDialog, "EditHistory",                  &Opt.Dialogs.EditHistory, 1, OPT_BOOLEAN},
	{0x1, NSecDialog, "EditBlock",                    &Opt.Dialogs.EditBlock, 0, OPT_BOOLEAN},
	{0x1, NSecDialog, "AutoComplete",                 &Opt.Dialogs.AutoComplete, 1, OPT_BOOLEAN},
	{0x1, NSecDialog, "EULBsClear",                   &Opt.Dialogs.EULBsClear, 0, OPT_BOOLEAN},
	{0x0, NSecDialog, "EditLine",                     &Opt.Dialogs.EditLine, 0},
	{0x1, NSecDialog, "MouseButton",                  &Opt.Dialogs.MouseButton, 0xFFFF},
	{0x1, NSecDialog, "DelRemovesBlocks",             &Opt.Dialogs.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{0x1, NSecDialog, "CBoxMaxHeight",                &Opt.Dialogs.CBoxMaxHeight, 8},

	{0x1, NSecEditor, "ExternalEditorName",           &Opt.strExternalEditor, L""},
	{0x1, NSecEditor, "UseExternalEditor",            &Opt.EdOpt.UseExternalEditor, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "ExpandTabs",                   &Opt.EdOpt.ExpandTabs, EXPAND_NOTABS},
	{0x1, NSecEditor, "TabSize",                      &Opt.EdOpt.TabSize, 8},
	{0x1, NSecEditor, "PersistentBlocks",             &Opt.EdOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "DelRemovesBlocks",             &Opt.EdOpt.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "AutoIndent",                   &Opt.EdOpt.AutoIndent, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "SaveEditorPos",                &Opt.EdOpt.SavePos, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "SaveEditorShortPos",           &Opt.EdOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "AutoDetectCodePage",           &Opt.EdOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "EditorCursorBeyondEOL",        &Opt.EdOpt.CursorBeyondEOL, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "ReadOnlyLock",                 &Opt.EdOpt.ReadOnlyLock, 0},
	{0x1, NSecEditor, "UndoDataSize",                 &Opt.EdOpt.UndoSize, 0x0800'0000}, // 2^27 chars (1 GiB)
	{0x0, NSecEditor, "WordDiv",                      &Opt.strWordDiv, WordDiv0},
	{0x0, NSecEditor, "BSLikeDel",                    &Opt.EdOpt.BSLikeDel, 1, OPT_BOOLEAN},
	{0x0, NSecEditor, "FileSizeLimit",                &Opt.EdOpt.FileSizeLimitLo, 0},
	{0x0, NSecEditor, "FileSizeLimitHi",              &Opt.EdOpt.FileSizeLimitHi, 0},
	{0x0, NSecEditor, "CharCodeBase",                 &Opt.EdOpt.CharCodeBase, 1},
	{0x0, NSecEditor, "AllowEmptySpaceAfterEof",      &Opt.EdOpt.AllowEmptySpaceAfterEof, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "DefaultCodePage",              &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{0x1, NSecEditor, "ShowKeyBar",                   &Opt.EdOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "ShowTitleBar",                 &Opt.EdOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "ShowScrollBar",                &Opt.EdOpt.ShowScrollBar, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "EditOpenedForWrite",           &Opt.EdOpt.EditOpenedForWrite, 1, OPT_BOOLEAN},
	{0x1, NSecEditor, "SearchSelFound",               &Opt.EdOpt.SearchSelFound, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "SearchRegexp",                 &Opt.EdOpt.SearchRegexp, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "SearchPickUpWord",             &Opt.EdOpt.SearchPickUpWord, 0, OPT_BOOLEAN},
	{0x1, NSecEditor, "ShowWhiteSpace",               &Opt.EdOpt.ShowWhiteSpace, 0, OPT_BOOLEAN},

	{0x1, NSecNotifications, "OnFileOperation",       &Opt.NotifOpt.OnFileOperation, 1, OPT_BOOLEAN},
	{0x1, NSecNotifications, "OnConsole",             &Opt.NotifOpt.OnConsole, 1, OPT_BOOLEAN},
	{0x1, NSecNotifications, "OnlyIfBackground",      &Opt.NotifOpt.OnlyIfBackground, 1, OPT_BOOLEAN},

	{0x0, NSecXLat, "Flags",                          &Opt.XLat.Flags, XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{0x1, NSecXLat, "EnableForFastFileFind",          &Opt.XLat.EnableForFastFileFind, 1, OPT_BOOLEAN},
	{0x1, NSecXLat, "EnableForDialogs",               &Opt.XLat.EnableForDialogs, 1, OPT_BOOLEAN},
	{0x1, NSecXLat, "WordDivForXlat",                 &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{0x1, NSecXLat, "XLat",                           &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{0x1, NSecSavedHistory, NParamHistoryCount,       &Opt.HistoryCount, 512},
	{0x1, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{0x1, NSecSavedViewHistory, NParamHistoryCount,   &Opt.ViewHistoryCount, 512},
	{0x1, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{0x1, NSecSystem, "PersonalPluginsPath",          &Opt.LoadPlug.strPersonalPluginsPath, L""},
	{0x1, NSecSystem, "HistoryShowDates",             &Opt.HistoryShowDates, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "SaveHistory",                  &Opt.SaveHistory, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "SaveFoldersHistory",           &Opt.SaveFoldersHistory, 1, OPT_BOOLEAN},
	{0x0, NSecSystem, "SavePluginFoldersHistory",     &Opt.SavePluginFoldersHistory, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "SaveViewHistory",              &Opt.SaveViewHistory, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "AutoSaveSetup",                &Opt.AutoSaveSetup, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "AutoSavePanels",               &Opt.AutoSavePanels, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "DeleteToRecycleBin",           &Opt.DeleteToRecycleBin, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "DeleteToRecycleBinKillLink",   &Opt.DeleteToRecycleBinKillLink, 1, OPT_BOOLEAN},
	{0x0, NSecSystem, "WipeSymbol",                   &Opt.WipeSymbol, 0},
	{0x1, NSecSystem, "SudoEnabled",                  &Opt.SudoEnabled, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "SudoConfirmModify",            &Opt.SudoConfirmModify, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "SudoPasswordExpiration",       &Opt.SudoPasswordExpiration, 15*60},

	{0x1, NSecSystem, "UseCOW",                       &Opt.CMOpt.UseCOW, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "SparseFiles",                  &Opt.CMOpt.SparseFiles, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "HowCopySymlink",               &Opt.CMOpt.HowCopySymlink, 1},
	{0x1, NSecSystem, "WriteThrough",                 &Opt.CMOpt.WriteThrough, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "CopyXAttr",                    &Opt.CMOpt.CopyXAttr, 0, OPT_BOOLEAN},
	{0x0, NSecSystem, "CopyAccessMode",               &Opt.CMOpt.CopyAccessMode, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "MultiCopy",                    &Opt.CMOpt.MultiCopy, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "CopyTimeRule",                 &Opt.CMOpt.CopyTimeRule, 3},

	{0x1, NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1, OPT_BOOLEAN},

	{0x1, NSecSystem, "InactivityExit",               &Opt.InactivityExit, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "InactivityExitTime",           &Opt.InactivityExitTime, 15},
	{0x1, NSecSystem, "DriveMenuMode2",               &Opt.ChangeDriveMode, -1},
	{0x1, NSecSystem, "DriveDisconnectMode",          &Opt.ChangeDriveDisconnectMode, 1},

	{0x1, NSecSystem, "DriveExceptions",              &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;"
		"/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron"},
	{0x1, NSecSystem, "DriveColumn2",                 &Opt.ChangeDriveColumn2, L"$U/$T"},
	{0x1, NSecSystem, "DriveColumn3",                 &Opt.ChangeDriveColumn3, L"$S$D"},

	{0x1, NSecSystem, "AutoUpdateRemoteDrive",        &Opt.AutoUpdateRemoteDrive, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "FileSearchMode",               &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{0x0, NSecSystem, "CollectFiles",                 &Opt.FindOpt.CollectFiles, true},
	{0x1, NSecSystem, "SearchInFirstSize",            &Opt.FindOpt.strSearchInFirstSize, L""},
	{0x1, NSecSystem, "FindAlternateStreams",         &Opt.FindOpt.FindAlternateStreams, false},
	{0x1, NSecSystem, "SearchOutFormat",              &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{0x1, NSecSystem, "SearchOutFormatWidth",         &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{0x1, NSecSystem, "FindFolders",                  &Opt.FindOpt.FindFolders, true},
	{0x1, NSecSystem, "FindSymLinks",                 &Opt.FindOpt.FindSymLinks, true},
	{0x1, NSecSystem, "FindCaseSensitiveFileMask",    &Opt.FindOpt.FindCaseSensitiveFileMask, false},
	{0x1, NSecSystem, "UseFilterInSearch",            &Opt.FindOpt.UseFilter, false},
	{0x1, NSecSystem, "FindCodePage",                 &Opt.FindCodePage, (int)CP_AUTODETECT},
	{0x0, NSecSystem, "CmdHistoryRule",               &Opt.CmdHistoryRule, 0, OPT_BOOLEAN},
	{0x0, NSecSystem, "SetAttrFolderRules",           &Opt.SetAttrFolderRules, 1, OPT_BOOLEAN},
	{0x0, NSecSystem, "MaxPositionCache",             &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{0x0, NSecSystem, "ConsoleDetachKey",             &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{0x0, NSecSystem, "SilentLoadPlugin",             &Opt.LoadPlug.SilentLoadPlugin, 0, OPT_BOOLEAN},
	{0x1, NSecSystem, "ScanSymlinks",                 &Opt.LoadPlug.ScanSymlinks, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "MultiMakeDir",                 &Opt.MultiMakeDir, 0, OPT_BOOLEAN},
	{0x0, NSecSystem, "MsWheelDelta",                 &Opt.MsWheelDelta, 1},
	{0x0, NSecSystem, "MsWheelDeltaView",             &Opt.MsWheelDeltaView, 1},
	{0x0, NSecSystem, "MsWheelDeltaEdit",             &Opt.MsWheelDeltaEdit, 1},
	{0x0, NSecSystem, "MsWheelDeltaHelp",             &Opt.MsWheelDeltaHelp, 1},
	{0x0, NSecSystem, "MsHWheelDelta",                &Opt.MsHWheelDelta, 1},
	{0x0, NSecSystem, "MsHWheelDeltaView",            &Opt.MsHWheelDeltaView, 1},
	{0x0, NSecSystem, "MsHWheelDeltaEdit",            &Opt.MsHWheelDeltaEdit, 1},
	{0x0, NSecSystem, "SubstNameRule",                &Opt.SubstNameRule, 2},
	{0x0, NSecSystem, "ShowCheckingFile",             &Opt.ShowCheckingFile, 0, OPT_BOOLEAN},
	{0x0, NSecSystem, "QuotedSymbols",                &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{0x0, NSecSystem, "QuotedName",                   &Opt.QuotedName, QUOTEDNAME_INSERT},
	{0x0, NSecSystem, "PluginMaxReadData",            &Opt.PluginMaxReadData, 0x40000},
	{0x0, NSecSystem, "CASRule",                      &Opt.CASRule, -1},
	{0x0, NSecSystem, "AllCtrlAltShiftRule",          &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{0x1, NSecSystem, "ScanJunction",                 &Opt.ScanJunction, 1, OPT_BOOLEAN},
	{0x1, NSecSystem, "OnlyFilesSize",                &Opt.OnlyFilesSize, 0, OPT_BOOLEAN},
	{0x0, NSecSystem, "UsePrintManager",              &Opt.UsePrintManager, 1, OPT_BOOLEAN},
	{0x0, NSecSystem, "WindowMode",                   &Opt.WindowMode, 0, OPT_BOOLEAN},

	{0x0, NSecPanelTree, "MinTreeCount",              &Opt.Tree.MinTreeCount, 4},
	{0x0, NSecPanelTree, "TreeFileAttr",              &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{0x0, NSecPanelTree, "LocalDisk",                 &Opt.Tree.LocalDisk, 2},
	{0x0, NSecPanelTree, "NetDisk",                   &Opt.Tree.NetDisk, 2},
	{0x0, NSecPanelTree, "RemovableDisk",             &Opt.Tree.RemovableDisk, 2},
	{0x0, NSecPanelTree, "NetPath",                   &Opt.Tree.NetPath, 2},
	{0x1, NSecPanelTree, "AutoChangeFolder",          &Opt.Tree.AutoChangeFolder, 0, OPT_BOOLEAN}, // ???
	{0x1, NSecPanelTree, "ExclSubTreeMask",           &Opt.Tree.ExclSubTreeMask, L".*"},

	{0x0, NSecHelp, "ActivateURL",                    &Opt.HelpURLRules, 1},

	{0x1, NSecLanguage, "Help",                       &Opt.strHelpLanguage, L"English"},
	{0x1, NSecLanguage, "Main",                       &Opt.strLanguage, L"English"},

	{0x1, NSecConfirmations, "Copy",                  &Opt.Confirm.Copy, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "Move",                  &Opt.Confirm.Move, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "RO",                    &Opt.Confirm.RO, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "Drag",                  &Opt.Confirm.Drag, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "Delete",                &Opt.Confirm.Delete, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "DeleteFolder",          &Opt.Confirm.DeleteFolder, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "Esc",                   &Opt.Confirm.Esc, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "RemoveConnection",      &Opt.Confirm.RemoveConnection, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "RemoveHotPlug",         &Opt.Confirm.RemoveHotPlug, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "AllowReedit",           &Opt.Confirm.AllowReedit, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "HistoryClear",          &Opt.Confirm.HistoryClear, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "Exit",                  &Opt.Confirm.Exit, 1, OPT_BOOLEAN},
	{0x1, NSecConfirmations, "ExitOrBknd",            &Opt.Confirm.ExitOrBknd, 1, OPT_BOOLEAN},
	{0x0, NSecConfirmations, "EscTwiceToInterrupt",   &Opt.Confirm.EscTwiceToInterrupt, 0, OPT_BOOLEAN},

	{0x1, NSecPluginConfirmations, "OpenFilePlugin",  &Opt.PluginConfirm.OpenFilePlugin, 0, OPT_3STATE},
	{0x1, NSecPluginConfirmations, "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, OPT_BOOLEAN},
	{0x1, NSecPluginConfirmations, "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, OPT_BOOLEAN},
	{0x1, NSecPluginConfirmations, "SetFindList",     &Opt.PluginConfirm.SetFindList, 0, OPT_BOOLEAN},
	{0x1, NSecPluginConfirmations, "Prefix",          &Opt.PluginConfirm.Prefix, 0, OPT_BOOLEAN},

	{0x0, NSecPanel, "ShellRightLeftArrowsRule",      &Opt.ShellRightLeftArrowsRule, 0, OPT_BOOLEAN},
	{0x1, NSecPanel, "ShowHidden",                    &Opt.ShowHidden, 1, OPT_BOOLEAN},
	{0x1, NSecPanel, "Highlight",                     &Opt.Highlight, 1, OPT_BOOLEAN},
	{0x1, NSecPanel, "SortFolderExt",                 &Opt.SortFolderExt, 0, OPT_BOOLEAN},
	{0x1, NSecPanel, "SelectFolders",                 &Opt.SelectFolders, 0, OPT_BOOLEAN},
	{0x1, NSecPanel, "AttrStrStyle",                  &Opt.AttrStrStyle, 1},
	{0x1, NSecPanel, "CaseSensitiveCompareSelect",    &Opt.PanelCaseSensitiveCompareSelect, 0, OPT_BOOLEAN},
	{0x1, NSecPanel, "ReverseSort",                   &Opt.ReverseSort, 1, OPT_BOOLEAN},
	{0x0, NSecPanel, "RightClickRule",                &Opt.PanelRightClickRule, 2, OPT_3STATE},
	{0x0, NSecPanel, "CtrlAltShiftRule",              &Opt.PanelCtrlAltShiftRule, 0, OPT_3STATE},
	{0x0, NSecPanel, "RememberLogicalDrives",         &Opt.RememberLogicalDrives, 0, OPT_BOOLEAN},
	{0x1, NSecPanel, "AutoUpdateLimit",               &Opt.AutoUpdateLimit, 0},
	{0x1, NSecPanel, "ShowFilenameMarks",             &Opt.ShowFilenameMarks, 1, OPT_BOOLEAN},
	{0x1, NSecPanel, "FilenameMarksAlign",            &Opt.FilenameMarksAlign, 1, OPT_BOOLEAN},
	{0x1, NSecPanel, "MinFilenameIndentation",        &Opt.MinFilenameIndentation, 0},
	{0x1, NSecPanel, "MaxFilenameIndentation",        &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},
	{0x1, NSecPanel, "ClassicHotkeyLinkResolving",    &Opt.ClassicHotkeyLinkResolving, 1, OPT_BOOLEAN},

	{0x2, NSecPanelLeft, "Type",                      &Opt.LeftPanel.Type, FILE_PANEL},
	{0x2, NSecPanelLeft, "Visible",                   &Opt.LeftPanel.Visible, 1, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "Focus",                     &Opt.LeftPanel.Focus, 1, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "ViewMode",                  &Opt.LeftPanel.ViewMode, VIEW_2},
	{0x2, NSecPanelLeft, "SortMode",                  &Opt.LeftPanel.SortMode, PanelSortMode::BY_NAME},
	{0x2, NSecPanelLeft, "SortOrder",                 &Opt.LeftPanel.SortOrder, 1},
	{0x2, NSecPanelLeft, "SortGroups",                &Opt.LeftPanel.SortGroups, 0, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "NumericSort",               &Opt.LeftPanel.NumericSort, 0, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "CaseSensitiveSort",         &Opt.LeftPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "Folder",                    &Opt.strLeftFolder, L""},
	{0x2, NSecPanelLeft, "CurFile",                   &Opt.strLeftCurFile, L""},
	{0x2, NSecPanelLeft, "SelectedFirst",             &Opt.LeftSelectedFirst, 0, OPT_BOOLEAN},
	{0x2, NSecPanelLeft, "DirectoriesFirst",          &Opt.LeftPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{0x2, NSecPanelRight, "Type",                     &Opt.RightPanel.Type, FILE_PANEL},
	{0x2, NSecPanelRight, "Visible",                  &Opt.RightPanel.Visible, 1, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "Focus",                    &Opt.RightPanel.Focus, 0, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "ViewMode",                 &Opt.RightPanel.ViewMode, VIEW_2},
	{0x2, NSecPanelRight, "SortMode",                 &Opt.RightPanel.SortMode, PanelSortMode::BY_NAME},
	{0x2, NSecPanelRight, "SortOrder",                &Opt.RightPanel.SortOrder, 1},
	{0x2, NSecPanelRight, "SortGroups",               &Opt.RightPanel.SortGroups, 0, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "NumericSort",              &Opt.RightPanel.NumericSort, 0, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "CaseSensitiveSort",        &Opt.RightPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "Folder",                   &Opt.strRightFolder, L""},
	{0x2, NSecPanelRight, "CurFile",                  &Opt.strRightCurFile, L""},
	{0x2, NSecPanelRight, "SelectedFirst",            &Opt.RightSelectedFirst, 0, OPT_BOOLEAN},
	{0x2, NSecPanelRight, "DirectoriesFirst",         &Opt.RightPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{0x1, NSecPanelLayout, "ColumnTitles",            &Opt.ShowColumnTitles, 1, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "StatusLine",              &Opt.ShowPanelStatus, 1, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "TotalInfo",               &Opt.ShowPanelTotals, 1, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "FreeInfo",                &Opt.ShowPanelFree, 0, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "Scrollbar",               &Opt.ShowPanelScrollbar, 0, OPT_BOOLEAN},
	{0x0, NSecPanelLayout, "ScrollbarMenu",           &Opt.ShowMenuScrollbar, 1, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "ScreensNumber",           &Opt.ShowScreensNumber, 1, OPT_BOOLEAN},
	{0x1, NSecPanelLayout, "SortMode",                &Opt.ShowSortMode, 1, OPT_BOOLEAN},

	{0x1, NSecLayout, "LeftHeightDecrement",          &Opt.LeftHeightDecrement, 0},
	{0x1, NSecLayout, "RightHeightDecrement",         &Opt.RightHeightDecrement, 0},
	{0x1, NSecLayout, "WidthDecrement",               &Opt.WidthDecrement, 0},
	{0x1, NSecLayout, "FullscreenHelp",               &Opt.FullScreenHelp, 0, OPT_BOOLEAN},

	{0x1, NSecDescriptions, "ListNames",              &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{0x1, NSecDescriptions, "UpdateMode",             &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{0x1, NSecDescriptions, "ROUpdate",               &Opt.Diz.ROUpdate, 0, OPT_BOOLEAN},
	{0x1, NSecDescriptions, "SetHidden",              &Opt.Diz.SetHidden, 1, OPT_BOOLEAN},
	{0x1, NSecDescriptions, "StartPos",               &Opt.Diz.StartPos, 0},
	{0x1, NSecDescriptions, "AnsiByDefault",          &Opt.Diz.AnsiByDefault, 0, OPT_BOOLEAN},
	{0x1, NSecDescriptions, "SaveInUTF",              &Opt.Diz.SaveInUTF, 0, OPT_BOOLEAN},

	{0x0, NSecMacros, "DateFormat",                   &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{0x1, NSecMacros, "ShowPlayIndicator",            &Opt.Macro.ShowPlayIndicator, 1, OPT_BOOLEAN},
	{0x1, NSecMacros, "KeyRecordCtrlDot",             &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot},
	{0x1, NSecMacros, "KeyRecordCtrlShiftDot",        &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot},

	{0x1, NSecSystem, "ExcludeCmdHistory",            &Opt.ExcludeCmdHistory, 0},

	{0x1, NSecCodePages, "CPMenuMode",                &Opt.CPMenuMode, 0, OPT_BOOLEAN},

	{0x1, NSecSystem, "FolderInfo",                   &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{0x1, NSecVMenu, "MenuLoopScroll",                &Opt.VMenu.MenuLoopScroll, 0, OPT_BOOLEAN},

	{0x1, NSecVMenu, "LBtnClick",                     &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{0x1, NSecVMenu, "RBtnClick",                     &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{0x1, NSecVMenu, "MBtnClick",                     &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
};

size_t ConfigOptGetSize()
{
	return ARRAYSIZE(CFG);
}

int ConfigOptGetIndex(const wchar_t *KeyName)
{
	if (const wchar_t *Dot = wcsrchr(KeyName, L'.'))
	{
		std::string sKey = FARString(KeyName, Dot-KeyName).GetMB();
		std::string sName = FARString(Dot+1).GetMB();
		const char *Key=sKey.c_str(), *Name=sName.c_str();

		for (int I=0; I < (int)ARRAYSIZE(CFG); ++I)
		{
			if (!strcasecmp(CFG[I].KeyName,Key) && !strcasecmp(CFG[I].ValName,Name))
				return I;
		}
	}
	return -1;
}

bool ConfigOptGetValue(size_t I, GetConfig& Data)
{
	if (I < ARRAYSIZE(CFG))
	{
		Data.IsSave = CFG[I].IsSave;
		Data.ValType = CFG[I].ValType;
		Data.KeyName = CFG[I].KeyName;
		Data.ValName = CFG[I].ValName;
		switch (CFG[I].ValType)
		{
			case OPT_DWORD:
			case OPT_BOOLEAN:
			case OPT_3STATE:
				Data.dwDefault = CFG[I].DefDWord;
				Data.dwValue = *CFG[I].DWordPtr;
				break;
			case OPT_REALBOOLEAN:
				Data.dwDefault = CFG[I].DefBool ? 1 : 0;
				Data.dwValue = *CFG[I].BoolPtr ? 1 : 0;
				break;
			case OPT_SZ:
				Data.strDefault = CFG[I].DefStr;
				Data.strValue = *CFG[I].StrPtr;
				break;
			case OPT_BINARY:
				Data.binDefault = CFG[I].DefArr;
				Data.binData = CFG[I].VoidPtr;
				Data.binSize = CFG[I].ArrSize;
				break;
		}
		return true;
	}
	return false;
}

bool ConfigOptSetInteger(size_t I, DWORD Value)
{
	if (I < ARRAYSIZE(CFG))
	{
		switch(CFG[I].ValType)
		{
			case OPT_DWORD:
				*CFG[I].DWordPtr = Value;
				break;
			case OPT_BOOLEAN:
				*CFG[I].DWordPtr = Value ? 1 : 0;
				break;
			case OPT_REALBOOLEAN:
				*CFG[I].BoolPtr = Value != 0;
				break;
			case OPT_3STATE:
				*CFG[I].DWordPtr = Value % 3;
				break;
			default:
				return false;
		}
		return true;
	}
	return false;
}

bool ConfigOptSetString(size_t I, const wchar_t *Value)
{
	if (I < ARRAYSIZE(CFG) && CFG[I].ValType == OPT_SZ && Value)
	{
		*CFG[I].StrPtr = Value;
		return true;
	}
	return false;
}

bool ConfigOptSetBinary(size_t I, const void *Data, DWORD Size)
{
	if (I < ARRAYSIZE(CFG) && CFG[I].ValType == OPT_BINARY && Data)
	{
		Size = std::min(Size, CFG[I].ArrSize);
		memcpy(CFG[I].VoidPtr, Data, Size);
		return true;
	}
	return false;
}

static void ConfigOptFromCmdLine()
{
	for (const auto &Str: Opt.CmdLineStrings)
	{
		auto pName = Str.c_str();
		auto pVal = wcschr(pName, L'=');
		if (pVal)
		{
			FARString strName(pName, pVal - pName);
			pVal++;
			if (int Index = ConfigOptGetIndex(strName.CPtr()); Index >= 0)
			{
				switch (CFG[Index].ValType)
				{
					default:
						if (!StrCmpI(pVal, L"false"))        ConfigOptSetInteger(Index, 0);
						else if (!StrCmpI(pVal, L"true"))    ConfigOptSetInteger(Index, 1);
						else if (!StrCmpI(pVal, L"other"))   ConfigOptSetInteger(Index, 2);
						else {
							static auto Formats = { L"%d%lc", L"0x%x%lc", L"0X%x%lc" };
							for (auto Fmt: Formats)
							{
								int Int; wchar_t wc;
								if (1 == swscanf(pVal, Fmt, &Int, &wc))
								{
									ConfigOptSetInteger(Index, Int);
									break;
								}
							}
						}
						break;

					case OPT_SZ:
						ConfigOptSetString(Index, pVal);
						break;

					case OPT_BINARY:
						break; // not supported
				}
			}
		}
	}
	Opt.CmdLineStrings.clear();
}

void ConfigOptLoad()
{
	ConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	bool ExplicitWindowMode=Opt.WindowMode!=FALSE;
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		cfg_reader.SelectSection(CFG[I].KeyName);
		switch (CFG[I].ValType)
		{
			case OPT_DWORD:
				*CFG[I].DWordPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord);
				break;
			case OPT_BOOLEAN:
				*CFG[I].DWordPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord) ? 1 : 0;
				break;
			case OPT_3STATE:
				*CFG[I].DWordPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord) % 3;
				break;
			case OPT_REALBOOLEAN:
				*CFG[I].BoolPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefBool ? 1 : 0) != 0;
				break;
			case OPT_SZ:
				*CFG[I].StrPtr = cfg_reader.GetString(CFG[I].ValName, CFG[I].DefStr);
				break;
			case OPT_BINARY:
				size_t Size = cfg_reader.GetBytes((BYTE*)CFG[I].VoidPtr, CFG[I].ArrSize, CFG[I].ValName, CFG[I].DefArr);
				if (Size > 0 && Size < CFG[I].ArrSize)
					memset((BYTE*)CFG[I].VoidPtr + Size, 0, CFG[I].ArrSize - Size);

				break;
		}
	}

	/* Command line config modifiers */
	ConfigOptFromCmdLine();

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();
	SanitizeIndentationCounts();

	Opt.CursorBlinkTime = std::clamp(Opt.CursorBlinkTime, 100, 500);

	Opt.PluginMaxReadData = std::max(Opt.PluginMaxReadData, 0x1000u);

	if(ExplicitWindowMode)
	{
		Opt.WindowMode=TRUE;
	}

	Opt.HelpTabSize=8; // пока жестко пропишем...

	Opt.ViOpt.ViewerIsWrap&=1;
	Opt.ViOpt.ViewerWrap&=1;

	// Исключаем случайное стирание разделителей ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// Исключаем случайное стирание разделителей
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	Opt.ConsoleDetachKey=KeyNameToKey(strKeyNameConsoleDetachKey);

	if (Opt.EdOpt.TabSize<1 || Opt.EdOpt.TabSize>512)
		Opt.EdOpt.TabSize=8;

	if (Opt.ViOpt.TabSize<1 || Opt.ViOpt.TabSize>512)
		Opt.ViOpt.TabSize=8;

	if (KeyNameToKey(Opt.Macro.strKeyMacroCtrlDot) == KEY_INVALID)
		Opt.Macro.strKeyMacroCtrlDot = szCtrlDot;

	if (KeyNameToKey(Opt.Macro.strKeyMacroCtrlShiftDot) == KEY_INVALID)
		Opt.Macro.strKeyMacroCtrlShiftDot = szCtrlShiftDot;

	Opt.EdOpt.strWordDiv = Opt.strWordDiv;
	FileList::ReadPanelModes(cfg_reader);

	{
		//cfg_reader.SelectSection(NSecXLat);
		AllXlats xlats;
		std::string SetXLat;
		for (const auto &xlat : xlats) {
			if (Opt.XLat.XLat == xlat) {
				SetXLat.clear();
				break;
			}
			if (SetXLat.empty()) {
				SetXLat = xlat;
			}
		}
		if (!SetXLat.empty()) {
			Opt.XLat.XLat = SetXLat;
		}
	}

	Opt.FindOpt.OutColumns.clear();

	if (!Opt.FindOpt.strSearchOutFormat.IsEmpty())
	{
		if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
			Opt.FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
		TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),Opt.FindOpt.strSearchOutFormatWidth.CPtr(),
		                   Opt.FindOpt.OutColumns);
	}

	CheckMaskGroups();
	FileFilter::InitFilter(cfg_reader);

	g_config_ready = true;
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void ConfigOptAssertLoaded()
{
	if (!g_config_ready)
	{
		fprintf(stderr, "%s: oops\n", __FUNCTION__);
		abort();
	}
}

static void SavePanelsToOpt()
{
	Panel *LeftPanel = CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel = CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus = LeftPanel->GetFocus();
	Opt.LeftPanel.Visible = LeftPanel->IsVisible();
	Opt.RightPanel.Focus = RightPanel->GetFocus();
	Opt.RightPanel.Visible = RightPanel->IsVisible();

	if (LeftPanel->GetMode() == NORMAL_PANEL)
	{
		Opt.LeftPanel.Type = LeftPanel->GetType();
		Opt.LeftPanel.ViewMode = LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode = LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder = LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups = LeftPanel->GetSortGroups();
		Opt.LeftPanel.NumericSort = LeftPanel->GetNumericSort();
		Opt.LeftPanel.CaseSensitiveSort = LeftPanel->GetCaseSensitiveSort();
		Opt.LeftSelectedFirst = LeftPanel->GetSelectedFirstMode();
		Opt.LeftPanel.DirectoriesFirst = LeftPanel->GetDirectoriesFirst();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile);

	if (RightPanel->GetMode() == NORMAL_PANEL)
	{
		Opt.RightPanel.Type = RightPanel->GetType();
		Opt.RightPanel.ViewMode = RightPanel->GetViewMode();
		Opt.RightPanel.SortMode = RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder = RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups = RightPanel->GetSortGroups();
		Opt.RightPanel.NumericSort = RightPanel->GetNumericSort();
		Opt.RightPanel.CaseSensitiveSort = RightPanel->GetCaseSensitiveSort();
		Opt.RightSelectedFirst = RightPanel->GetSelectedFirstMode();
		Opt.RightPanel.DirectoriesFirst = RightPanel->GetDirectoriesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
}

void ConfigOptSave(bool Ask, int SaveWhat)
{
	if (Ask && Message(0,2,Msg::SaveSetupTitle,Msg::SaveSetupAsk1,Msg::SaveSetupAsk2,Msg::SaveSetup,Msg::Cancel))
		return;

	WINPORT(SaveConsoleWindowState)();

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	if (SaveWhat & OST_PANELS) {
		SavePanelsToOpt();
	}
	CtrlObject->HiFiles->SaveHiData();

	ConfigWriter cfg_writer;

	/* *************************************************** </ПРЕПРОЦЕССЫ> */
//	cfg_writer.SetString(NSecLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].IsSave & SaveWhat)
		{
			cfg_writer.SelectSection(CFG[I].KeyName);
			switch (CFG[I].ValType)
			{
				case OPT_DWORD:
				case OPT_BOOLEAN:
				case OPT_3STATE:
					cfg_writer.SetUInt(CFG[I].ValName, *CFG[I].DWordPtr);
					break;
				case OPT_REALBOOLEAN:
					cfg_writer.SetUInt(CFG[I].ValName, *CFG[I].BoolPtr ? 1 : 0);
					break;
				case OPT_SZ:
					cfg_writer.SetString(CFG[I].ValName, CFG[I].StrPtr->CPtr());
					break;
				case OPT_BINARY:
					cfg_writer.SetBytes(CFG[I].ValName, (const BYTE*)CFG[I].VoidPtr, CFG[I].ArrSize);
					break;
			}
		}
	}

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	FileFilter::SaveFilters(cfg_writer);
	FileList::SavePanelModes(cfg_writer);

	if (Ask)
		CtrlObject->Macro.SaveMacros();

	FarColors::SaveFarColors();
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}
