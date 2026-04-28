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

// ValName
static const char NParamAutoSavePanels[] = "AutoSavePanels";
static const char NParamAutoSaveSetup[] = "AutoSaveSetup";
static const char NParamHistoryCount[] = "HistoryCount";

enum OptSaveType {
	OST_NONE   = 0,
	OST_COMMON = 0x01,
	OST_PANELS = 0x02,
};

// Структура, описывающая всю конфигурацию(!)
struct FARConfig
{
	OptSaveType SaveType;
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

	constexpr FARConfig(OptSaveType save, const char *key, const char *val, DWORD size, void *trg, const BYTE *dflt) :
		SaveType(save),ValType(OPT_BINARY),KeyName(key),ValName(val),VoidPtr(trg),ArrSize(size),DefArr(dflt) {}

	constexpr FARConfig(OptSaveType save, const char *key, const char *val, DWORD *trg, DWORD dflt, OPT_TYPE Type=OPT_DWORD) :
		SaveType(save),ValType(Type),KeyName(key),ValName(val),DWordPtr(trg),DefDWord(dflt),DefStr(nullptr) {}

	constexpr FARConfig(OptSaveType save, const char *key, const char *val, int *trg, int dflt, OPT_TYPE Type=OPT_DWORD) :
		SaveType(save),ValType(Type),KeyName(key),ValName(val),IntPtr(trg),DefInt(dflt),DefStr(nullptr) {}

	constexpr FARConfig(OptSaveType save, const char *key, const char *val, bool *trg, bool dflt) :
		SaveType(save),ValType(OPT_REALBOOLEAN),KeyName(key),ValName(val),BoolPtr(trg),DefBool(dflt),DefStr(nullptr) {}

	constexpr FARConfig(OptSaveType save, const char *key, const char *val, FARString *trg, const wchar_t *dflt) :
		SaveType(save),ValType(OPT_SZ),KeyName(key),ValName(val),StrPtr(trg),DefDWord(0),DefStr(dflt) {}
};

static FARConfig CFG[]
{
	{OST_COMMON, NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, nullptr},
	{OST_COMMON, NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, g_tempcolorsRGB, nullptr},

	{OST_COMMON, NSecScreen, "Clock",                        &Opt.Clock, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecScreen, "ViewerEditorClock",            &Opt.ViewerEditorClock, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecScreen, "KeyBar",                       &Opt.ShowKeyBar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecScreen, "ScreenSaver",                  &Opt.ScreenSaver, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecScreen, "ScreenSaverTime",              &Opt.ScreenSaverTime, 5},
	{OST_COMMON, NSecScreen, "CursorBlinkInterval",          &Opt.CursorBlinkTime, 500},

	{OST_COMMON, NSecCmdline, "UsePromptFormat",             &Opt.CmdLine.UsePromptFormat, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "PromptFormat",                &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{OST_COMMON, NSecCmdline, "UseShell",                    &Opt.CmdLine.UseShell, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "ShellCmd",                    &Opt.CmdLine.strShell, L"bash -i"},
	{OST_COMMON, NSecCmdline, "DelRemovesBlocks",            &Opt.CmdLine.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "EditBlock",                   &Opt.CmdLine.EditBlock, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "AutoComplete",                &Opt.CmdLine.AutoComplete, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "Splitter",                    &Opt.CmdLine.Splitter, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecCmdline, "WaitKeypress",                &Opt.CmdLine.WaitKeypress, 1},
	{OST_COMMON, NSecCmdline, "VTLogLimit",                  &Opt.CmdLine.VTLogLimit, 5000},
	{OST_COMMON, NSecCmdline, "ImitateNumpadKeys",           &Opt.CmdLine.ImitateNumpadKeys, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecCmdline, "AskOnMultilinePaste",         &Opt.CmdLine.AskOnMultilinePaste, 1, OPT_BOOLEAN},

	{OST_COMMON, NSecInterface, "Mouse",                     &Opt.Mouse, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecInterface, "UseVk_oem_x",               &Opt.UseVk_oem_x, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ShowMenuBar",               &Opt.ShowMenuBar, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecInterface, "CursorSize1",               &Opt.CursorSize[0], 15},
	{OST_NONE,   NSecInterface, "CursorSize2",               &Opt.CursorSize[1], 10},
	{OST_NONE,   NSecInterface, "CursorSize3",               &Opt.CursorSize[2], 99},
	{OST_NONE,   NSecInterface, "CursorSize4",               &Opt.CursorSize[3], 99},
	{OST_NONE,   NSecInterface, "ShiftsKeyRules",            &Opt.ShiftsKeyRules, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "CtrlPgUp",                  &Opt.PgUpChangeDisk, 1, OPT_BOOLEAN},

	{OST_COMMON, NSecInterface, "ConsolePaintSharp",         &Opt.ConsolePaintSharp, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveCtrlLeft",         &Opt.ExclusiveCtrlLeft, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveCtrlRight",        &Opt.ExclusiveCtrlRight, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveAltLeft",          &Opt.ExclusiveAltLeft, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveAltRight",         &Opt.ExclusiveAltRight, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveWinLeft",          &Opt.ExclusiveWinLeft, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "ExclusiveWinRight",         &Opt.ExclusiveWinRight, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "UseStickyKeyEvent",         &Opt.UseStickyKeyEvent, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecInterface, "DateFormat",                &Opt.DateFormat, GetDateFormatDefault()},
	{OST_COMMON, NSecInterface, "DateSeparator",             &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{OST_COMMON, NSecInterface, "TimeSeparator",             &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{OST_COMMON, NSecInterface, "DecimalSeparator",          &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

#if defined(__ANDROID__)
	{OST_COMMON, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 1, OPT_BOOLEAN},
#else
	{OST_COMMON, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 0, OPT_BOOLEAN},
#endif

	{OST_COMMON, NSecInterface, "TTYPaletteOverride",        &Opt.TTYPaletteOverride, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecInterface, "ShowTimeoutDelFiles",       &Opt.ShowTimeoutDelFiles, 50},
	{OST_NONE,   NSecInterface, "ShowTimeoutDACLFiles",      &Opt.ShowTimeoutDACLFiles, 50},
	{OST_NONE,   NSecInterface, "FormatNumberSeparators",    &Opt.FormatNumberSeparators, 0},
	{OST_COMMON, NSecInterface, "CopyShowTotal",             &Opt.CMOpt.CopyShowTotal, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "DelShowTotal",              &Opt.DelOpt.DelShowTotal, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterface, "WindowTitle",               &Opt.strWindowTitle, L"%State - FAR2M %Ver %Backend %User@%Host"}, // %Platform
	{OST_COMMON, NSecInterfaceCompletion, "Exceptions",      &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
	{OST_COMMON, NSecInterfaceCompletion, "ShowList",        &Opt.AutoComplete.ShowList, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecInterfaceCompletion, "ModalList",       &Opt.AutoComplete.ModalList, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecInterfaceCompletion, "Append",          &Opt.AutoComplete.AppendCompletion, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecViewer, "ExternalViewerName",           &Opt.strExternalViewer, L""},
	{OST_COMMON, NSecViewer, "UseExternalViewer",            &Opt.ViOpt.UseExternalViewer, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "SaveViewerPos",                &Opt.ViOpt.SavePos, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "SaveViewerShortPos",           &Opt.ViOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "AutoDetectCodePage",           &Opt.ViOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "SearchRegexp",                 &Opt.ViOpt.SearchRegexp, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecViewer, "TabSize",                      &Opt.ViOpt.TabSize, 8},
	{OST_COMMON, NSecViewer, "ShowKeyBar",                   &Opt.ViOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "ShowTitleBar",                 &Opt.ViOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "ShowArrows",                   &Opt.ViOpt.ShowArrows, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "ShowScrollbar",                &Opt.ViOpt.ShowScrollbar, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "IsWrap",                       &Opt.ViOpt.ViewerIsWrap, 1},
	{OST_COMMON, NSecViewer, "Wrap",                         &Opt.ViOpt.ViewerWrap, 0},
	{OST_COMMON, NSecViewer, "PersistentBlocks",             &Opt.ViOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecViewer, "DefaultCodePage",              &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{OST_COMMON, NSecDialog, "EditHistory",                  &Opt.Dialogs.EditHistory, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecDialog, "EditBlock",                    &Opt.Dialogs.EditBlock, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecDialog, "AutoComplete",                 &Opt.Dialogs.AutoComplete, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecDialog, "EULBsClear",                   &Opt.Dialogs.EULBsClear, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecDialog, "EditLine",                     &Opt.Dialogs.EditLine, 0},
	{OST_COMMON, NSecDialog, "MouseButton",                  &Opt.Dialogs.MouseButton, 0xFFFF},
	{OST_COMMON, NSecDialog, "DelRemovesBlocks",             &Opt.Dialogs.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecDialog, "CBoxMaxHeight",                &Opt.Dialogs.CBoxMaxHeight, 8},

	{OST_COMMON, NSecEditor, "ExternalEditorName",           &Opt.strExternalEditor, L""},
	{OST_COMMON, NSecEditor, "UseExternalEditor",            &Opt.EdOpt.UseExternalEditor, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "ExpandTabs",                   &Opt.EdOpt.ExpandTabs, EXPAND_NOTABS},
	{OST_COMMON, NSecEditor, "TabSize",                      &Opt.EdOpt.TabSize, 8},
	{OST_COMMON, NSecEditor, "PersistentBlocks",             &Opt.EdOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "DelRemovesBlocks",             &Opt.EdOpt.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "AutoIndent",                   &Opt.EdOpt.AutoIndent, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "SaveEditorPos",                &Opt.EdOpt.SavePos, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "SaveEditorShortPos",           &Opt.EdOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "AutoDetectCodePage",           &Opt.EdOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "EditorCursorBeyondEOL",        &Opt.EdOpt.CursorBeyondEOL, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "ReadOnlyLock",                 &Opt.EdOpt.ReadOnlyLock, 0},
	{OST_COMMON, NSecEditor, "UndoDataSize",                 &Opt.EdOpt.UndoSize, 0x0800'0000}, // 2^27 chars (1 GiB)
	{OST_NONE,   NSecEditor, "WordDiv",                      &Opt.strWordDiv, WordDiv0},
	{OST_NONE,   NSecEditor, "BSLikeDel",                    &Opt.EdOpt.BSLikeDel, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecEditor, "FileSizeLimit",                &Opt.EdOpt.FileSizeLimitLo, 0},
	{OST_NONE,   NSecEditor, "FileSizeLimitHi",              &Opt.EdOpt.FileSizeLimitHi, 0},
	{OST_NONE,   NSecEditor, "CharCodeBase",                 &Opt.EdOpt.CharCodeBase, 1},
	{OST_NONE,   NSecEditor, "AllowEmptySpaceAfterEof",      &Opt.EdOpt.AllowEmptySpaceAfterEof, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "DefaultCodePage",              &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{OST_COMMON, NSecEditor, "ShowKeyBar",                   &Opt.EdOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "ShowTitleBar",                 &Opt.EdOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "ShowScrollBar",                &Opt.EdOpt.ShowScrollBar, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "EditOpenedForWrite",           &Opt.EdOpt.EditOpenedForWrite, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "SearchSelFound",               &Opt.EdOpt.SearchSelFound, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "SearchRegexp",                 &Opt.EdOpt.SearchRegexp, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "SearchPickUpWord",             &Opt.EdOpt.SearchPickUpWord, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecEditor, "ShowWhiteSpace",               &Opt.EdOpt.ShowWhiteSpace, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecNotifications, "OnFileOperation",       &Opt.NotifOpt.OnFileOperation, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecNotifications, "OnConsole",             &Opt.NotifOpt.OnConsole, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecNotifications, "OnlyIfBackground",      &Opt.NotifOpt.OnlyIfBackground, 1, OPT_BOOLEAN},

	{OST_NONE,   NSecXLat, "Flags",                          &Opt.XLat.Flags, XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{OST_COMMON, NSecXLat, "EnableForFastFileFind",          &Opt.XLat.EnableForFastFileFind, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecXLat, "EnableForDialogs",               &Opt.XLat.EnableForDialogs, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecXLat, "WordDivForXlat",                 &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{OST_COMMON, NSecXLat, "XLat",                           &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{OST_COMMON, NSecSavedHistory, NParamHistoryCount,       &Opt.HistoryCount, 512},
	{OST_COMMON, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{OST_COMMON, NSecSavedViewHistory, NParamHistoryCount,   &Opt.ViewHistoryCount, 512},
	{OST_COMMON, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{OST_COMMON, NSecSystem, "PersonalPluginsPath",          &Opt.LoadPlug.strPersonalPluginsPath, L""},
	{OST_COMMON, NSecSystem, "HistoryShowDates",             &Opt.HistoryShowDates, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SaveHistory",                  &Opt.SaveHistory, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SaveFoldersHistory",           &Opt.SaveFoldersHistory, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "SavePluginFoldersHistory",     &Opt.SavePluginFoldersHistory, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SaveViewHistory",              &Opt.SaveViewHistory, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, NParamAutoSaveSetup,            &Opt.AutoSaveSetup, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, NParamAutoSavePanels,           &Opt.AutoSavePanels, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "DeleteToRecycleBin",           &Opt.DeleteToRecycleBin, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "DeleteToRecycleBinKillLink",   &Opt.DeleteToRecycleBinKillLink, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "WipeSymbol",                   &Opt.WipeSymbol, 0},
	{OST_COMMON, NSecSystem, "SudoEnabled",                  &Opt.SudoEnabled, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SudoConfirmModify",            &Opt.SudoConfirmModify, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SudoPasswordExpiration",       &Opt.SudoPasswordExpiration, 15*60},

	{OST_COMMON, NSecSystem, "UseCOW",                       &Opt.CMOpt.UseCOW, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "SparseFiles",                  &Opt.CMOpt.SparseFiles, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "HowCopySymlink",               &Opt.CMOpt.HowCopySymlink, 1},
	{OST_COMMON, NSecSystem, "WriteThrough",                 &Opt.CMOpt.WriteThrough, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "CopyXAttr",                    &Opt.CMOpt.CopyXAttr, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "CopyAccessMode",               &Opt.CMOpt.CopyAccessMode, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "MultiCopy",                    &Opt.CMOpt.MultiCopy, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "CopyTimeRule",                 &Opt.CMOpt.CopyTimeRule, 3},

	{OST_COMMON, NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1, OPT_BOOLEAN},

	{OST_COMMON, NSecSystem, "InactivityExit",               &Opt.InactivityExit, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "InactivityExitTime",           &Opt.InactivityExitTime, 15},
	{OST_COMMON, NSecSystem, "DriveMenuMode2",               &Opt.ChangeDriveMode, -1},
	{OST_COMMON, NSecSystem, "DriveDisconnectMode",          &Opt.ChangeDriveDisconnectMode, 1},

	{OST_COMMON, NSecSystem, "DriveExceptions",              &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;"
		"/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron"},
	{OST_COMMON, NSecSystem, "DriveColumn2",                 &Opt.ChangeDriveColumn2, L"$U/$T"},
	{OST_COMMON, NSecSystem, "DriveColumn3",                 &Opt.ChangeDriveColumn3, L"$S$D"},

	{OST_COMMON, NSecSystem, "AutoUpdateRemoteDrive",        &Opt.AutoUpdateRemoteDrive, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "FileSearchMode",               &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{OST_NONE,   NSecSystem, "CollectFiles",                 &Opt.FindOpt.CollectFiles, true},
	{OST_COMMON, NSecSystem, "SearchInFirstSize",            &Opt.FindOpt.strSearchInFirstSize, L""},
	{OST_COMMON, NSecSystem, "FindAlternateStreams",         &Opt.FindOpt.FindAlternateStreams, false},
	{OST_COMMON, NSecSystem, "SearchOutFormat",              &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{OST_COMMON, NSecSystem, "SearchOutFormatWidth",         &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{OST_COMMON, NSecSystem, "FindFolders",                  &Opt.FindOpt.FindFolders, true},
	{OST_COMMON, NSecSystem, "FindSymLinks",                 &Opt.FindOpt.FindSymLinks, true},
	{OST_COMMON, NSecSystem, "FindCaseSensitiveFileMask",    &Opt.FindOpt.FindCaseSensitiveFileMask, false},
	{OST_COMMON, NSecSystem, "UseFilterInSearch",            &Opt.FindOpt.UseFilter, false},
	{OST_COMMON, NSecSystem, "FindCodePage",                 &Opt.FindCodePage, (int)CP_AUTODETECT},
	{OST_NONE,   NSecSystem, "CmdHistoryRule",               &Opt.CmdHistoryRule, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "SetAttrFolderRules",           &Opt.SetAttrFolderRules, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "MaxPositionCache",             &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{OST_NONE,   NSecSystem, "ConsoleDetachKey",             &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{OST_NONE,   NSecSystem, "SilentLoadPlugin",             &Opt.LoadPlug.SilentLoadPlugin, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "ScanSymlinks",                 &Opt.LoadPlug.ScanSymlinks, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "MultiMakeDir",                 &Opt.MultiMakeDir, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "MsWheelDelta",                 &Opt.MsWheelDelta, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaView",             &Opt.MsWheelDeltaView, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaEdit",             &Opt.MsWheelDeltaEdit, 1},
	{OST_NONE,   NSecSystem, "MsWheelDeltaHelp",             &Opt.MsWheelDeltaHelp, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDelta",                &Opt.MsHWheelDelta, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDeltaView",            &Opt.MsHWheelDeltaView, 1},
	{OST_NONE,   NSecSystem, "MsHWheelDeltaEdit",            &Opt.MsHWheelDeltaEdit, 1},
	{OST_NONE,   NSecSystem, "SubstNameRule",                &Opt.SubstNameRule, 2},
	{OST_NONE,   NSecSystem, "ShowCheckingFile",             &Opt.ShowCheckingFile, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "QuotedSymbols",                &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{OST_NONE,   NSecSystem, "QuotedName",                   &Opt.QuotedName, QUOTEDNAME_INSERT},
	{OST_NONE,   NSecSystem, "PluginMaxReadData",            &Opt.PluginMaxReadData, 0x40000},
	{OST_NONE,   NSecSystem, "CASRule",                      &Opt.CASRule, -1},
	{OST_NONE,   NSecSystem, "AllCtrlAltShiftRule",          &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{OST_COMMON, NSecSystem, "ScanJunction",                 &Opt.ScanJunction, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecSystem, "OnlyFilesSize",                &Opt.OnlyFilesSize, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "UsePrintManager",              &Opt.UsePrintManager, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecSystem, "WindowMode",                   &Opt.WindowMode, 0, OPT_BOOLEAN},

	{OST_NONE,   NSecPanelTree, "MinTreeCount",              &Opt.Tree.MinTreeCount, 4},
	{OST_NONE,   NSecPanelTree, "TreeFileAttr",              &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{OST_NONE,   NSecPanelTree, "LocalDisk",                 &Opt.Tree.LocalDisk, 2},
	{OST_NONE,   NSecPanelTree, "NetDisk",                   &Opt.Tree.NetDisk, 2},
	{OST_NONE,   NSecPanelTree, "RemovableDisk",             &Opt.Tree.RemovableDisk, 2},
	{OST_NONE,   NSecPanelTree, "NetPath",                   &Opt.Tree.NetPath, 2},
	{OST_COMMON, NSecPanelTree, "AutoChangeFolder",          &Opt.Tree.AutoChangeFolder, 0, OPT_BOOLEAN}, // ???
	{OST_COMMON, NSecPanelTree, "ExclSubTreeMask",           &Opt.Tree.ExclSubTreeMask, L".*"},

	{OST_NONE,   NSecHelp, "ActivateURL",                    &Opt.HelpURLRules, 1},

	{OST_COMMON, NSecLanguage, "Help",                       &Opt.strHelpLanguage, L"English"},
	{OST_COMMON, NSecLanguage, "Main",                       &Opt.strLanguage, L"English"},

	{OST_COMMON, NSecConfirmations, "Copy",                  &Opt.Confirm.Copy, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "Move",                  &Opt.Confirm.Move, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "RO",                    &Opt.Confirm.RO, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "Drag",                  &Opt.Confirm.Drag, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "Delete",                &Opt.Confirm.Delete, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "DeleteFolder",          &Opt.Confirm.DeleteFolder, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "Esc",                   &Opt.Confirm.Esc, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "RemoveConnection",      &Opt.Confirm.RemoveConnection, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "RemoveHotPlug",         &Opt.Confirm.RemoveHotPlug, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "AllowReedit",           &Opt.Confirm.AllowReedit, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "HistoryClear",          &Opt.Confirm.HistoryClear, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "Exit",                  &Opt.Confirm.Exit, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecConfirmations, "ExitOrBknd",            &Opt.Confirm.ExitOrBknd, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecConfirmations, "EscTwiceToInterrupt",   &Opt.Confirm.EscTwiceToInterrupt, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecPluginConfirmations, "OpenFilePlugin",  &Opt.PluginConfirm.OpenFilePlugin, 0, OPT_3STATE},
	{OST_COMMON, NSecPluginConfirmations, "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPluginConfirmations, "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPluginConfirmations, "SetFindList",     &Opt.PluginConfirm.SetFindList, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPluginConfirmations, "Prefix",          &Opt.PluginConfirm.Prefix, 0, OPT_BOOLEAN},

	{OST_NONE,   NSecPanel, "ShellRightLeftArrowsRule",      &Opt.ShellRightLeftArrowsRule, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "ShowHidden",                    &Opt.ShowHidden, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "Highlight",                     &Opt.Highlight, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "SortFolderExt",                 &Opt.SortFolderExt, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "SelectFolders",                 &Opt.SelectFolders, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "AttrStrStyle",                  &Opt.AttrStrStyle, 1},
	{OST_COMMON, NSecPanel, "CaseSensitiveCompareSelect",    &Opt.PanelCaseSensitiveCompareSelect, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "ReverseSort",                   &Opt.ReverseSort, 1, OPT_BOOLEAN},
	{OST_NONE,   NSecPanel, "RightClickRule",                &Opt.PanelRightClickRule, 2, OPT_3STATE},
	{OST_NONE,   NSecPanel, "CtrlAltShiftRule",              &Opt.PanelCtrlAltShiftRule, 0, OPT_3STATE},
	{OST_NONE,   NSecPanel, "RememberLogicalDrives",         &Opt.RememberLogicalDrives, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "AutoUpdateLimit",               &Opt.AutoUpdateLimit, 0},
	{OST_COMMON, NSecPanel, "ShowFilenameMarks",             &Opt.ShowFilenameMarks, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "FilenameMarksAlign",            &Opt.FilenameMarksAlign, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanel, "MinFilenameIndentation",        &Opt.MinFilenameIndentation, 0},
	{OST_COMMON, NSecPanel, "MaxFilenameIndentation",        &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},
	{OST_COMMON, NSecPanel, "ClassicHotkeyLinkResolving",    &Opt.ClassicHotkeyLinkResolving, 1, OPT_BOOLEAN},

	{OST_PANELS, NSecPanelLeft, "Type",                      &Opt.LeftPanel.Type, FILE_PANEL},
	{OST_COMMON, NSecPanelLeft, "Visible",                   &Opt.LeftPanel.Visible, 1, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "Focus",                     &Opt.LeftPanel.Focus, 1, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "ViewMode",                  &Opt.LeftPanel.ViewMode, VIEW_2},
	{OST_PANELS, NSecPanelLeft, "SortMode",                  &Opt.LeftPanel.SortMode, PanelSortMode::BY_NAME},
	{OST_PANELS, NSecPanelLeft, "SortOrder",                 &Opt.LeftPanel.SortOrder, 1},
	{OST_PANELS, NSecPanelLeft, "SortGroups",                &Opt.LeftPanel.SortGroups, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "NumericSort",               &Opt.LeftPanel.NumericSort, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "CaseSensitiveSort",         &Opt.LeftPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "Folder",                    &Opt.strLeftFolder, L""},
	{OST_PANELS, NSecPanelLeft, "CurFile",                   &Opt.strLeftCurFile, L""},
	{OST_COMMON, NSecPanelLeft, "SelectedFirst",             &Opt.LeftSelectedFirst, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelLeft, "DirectoriesFirst",          &Opt.LeftPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{OST_PANELS, NSecPanelRight, "Type",                     &Opt.RightPanel.Type, FILE_PANEL},
	{OST_COMMON, NSecPanelRight, "Visible",                  &Opt.RightPanel.Visible, 1, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "Focus",                    &Opt.RightPanel.Focus, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "ViewMode",                 &Opt.RightPanel.ViewMode, VIEW_2},
	{OST_PANELS, NSecPanelRight, "SortMode",                 &Opt.RightPanel.SortMode, PanelSortMode::BY_NAME},
	{OST_PANELS, NSecPanelRight, "SortOrder",                &Opt.RightPanel.SortOrder, 1},
	{OST_PANELS, NSecPanelRight, "SortGroups",               &Opt.RightPanel.SortGroups, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "NumericSort",              &Opt.RightPanel.NumericSort, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "CaseSensitiveSort",        &Opt.RightPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "Folder",                   &Opt.strRightFolder, L""},
	{OST_PANELS, NSecPanelRight, "CurFile",                  &Opt.strRightCurFile, L""},
	{OST_COMMON, NSecPanelRight, "SelectedFirst",            &Opt.RightSelectedFirst, 0, OPT_BOOLEAN},
	{OST_PANELS, NSecPanelRight, "DirectoriesFirst",         &Opt.RightPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{OST_COMMON, NSecPanelLayout, "ColumnTitles",            &Opt.ShowColumnTitles, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "StatusLine",              &Opt.ShowPanelStatus, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "TotalInfo",               &Opt.ShowPanelTotals, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "FreeInfo",                &Opt.ShowPanelFree, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "Scrollbar",               &Opt.ShowPanelScrollbar, 0, OPT_BOOLEAN},
	{OST_NONE,   NSecPanelLayout, "ScrollbarMenu",           &Opt.ShowMenuScrollbar, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "ScreensNumber",           &Opt.ShowScreensNumber, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecPanelLayout, "SortMode",                &Opt.ShowSortMode, 1, OPT_BOOLEAN},

	{OST_COMMON, NSecLayout, "LeftHeightDecrement",          &Opt.LeftHeightDecrement, 0},
	{OST_COMMON, NSecLayout, "RightHeightDecrement",         &Opt.RightHeightDecrement, 0},
	{OST_COMMON, NSecLayout, "WidthDecrement",               &Opt.WidthDecrement, 0},
	{OST_COMMON, NSecLayout, "FullscreenHelp",               &Opt.FullScreenHelp, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecDescriptions, "ListNames",              &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{OST_COMMON, NSecDescriptions, "UpdateMode",             &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{OST_COMMON, NSecDescriptions, "ROUpdate",               &Opt.Diz.ROUpdate, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecDescriptions, "SetHidden",              &Opt.Diz.SetHidden, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecDescriptions, "StartPos",               &Opt.Diz.StartPos, 0},
	{OST_COMMON, NSecDescriptions, "AnsiByDefault",          &Opt.Diz.AnsiByDefault, 0, OPT_BOOLEAN},
	{OST_COMMON, NSecDescriptions, "SaveInUTF",              &Opt.Diz.SaveInUTF, 0, OPT_BOOLEAN},

	{OST_NONE,   NSecMacros, "DateFormat",                   &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{OST_COMMON, NSecMacros, "ShowPlayIndicator",            &Opt.Macro.ShowPlayIndicator, 1, OPT_BOOLEAN},
	{OST_COMMON, NSecMacros, "KeyRecordCtrlDot",             &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot},
	{OST_COMMON, NSecMacros, "KeyRecordCtrlShiftDot",        &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot},

	{OST_COMMON, NSecSystem, "ExcludeCmdHistory",            &Opt.ExcludeCmdHistory, 0},

	{OST_COMMON, NSecCodePages, "CPMenuMode",                &Opt.CPMenuMode, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecSystem, "FolderInfo",                   &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{OST_COMMON, NSecVMenu, "MenuLoopScroll",                &Opt.VMenu.MenuLoopScroll, 0, OPT_BOOLEAN},

	{OST_COMMON, NSecVMenu, "LBtnClick",                     &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{OST_COMMON, NSecVMenu, "RBtnClick",                     &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{OST_COMMON, NSecVMenu, "MBtnClick",                     &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
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
		Data.SaveType = CFG[I].SaveType;
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

// Saved instantly when the "System Settings" dialog is accepted.
void ConfigOptSaveAutoOptions()
{
	ConfigWriter cfg_writer;
	cfg_writer.SelectSection(NSecSystem);
	cfg_writer.SetUInt(NParamAutoSaveSetup, Opt.AutoSaveSetup);
	cfg_writer.SetUInt(NParamAutoSavePanels, Opt.AutoSavePanels);
}

void ConfigOptSave(bool Ask)
{
	int SaveFlags = Ask ? (OST_COMMON | OST_PANELS)
			: (Opt.AutoSaveSetup ? OST_COMMON : 0) | (Opt.AutoSavePanels ? OST_PANELS : 0);

	if (SaveFlags == OST_NONE)
		return;

	if (Ask && Message(0,2,Msg::SaveSetupTitle,Msg::SaveSetupAsk1,Msg::SaveSetupAsk2,Msg::SaveSetup,Msg::Cancel))
		return;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	if (SaveFlags & OST_COMMON)
	{
		WINPORT(SaveConsoleWindowState)();
		CtrlObject->HiFiles->SaveHiData();
	}

	if (SaveFlags & OST_PANELS)
		SavePanelsToOpt();

	ConfigWriter cfg_writer;

	/* *************************************************** </ПРЕПРОЦЕССЫ> */
//	cfg_writer.SetString(NSecLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].SaveType & SaveFlags)
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
	if (SaveFlags & OST_COMMON)
	{
		FileFilter::SaveFilters(cfg_writer);
		FileList::SavePanelModes(cfg_writer);

		if (Ask)
			CtrlObject->Macro.SaveMacros();

		FarColors::SaveFarColors();
	}
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}
