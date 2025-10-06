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
static const char NSecColors[]="Colors";
static const char NSecScreen[]="Screen";
static const char NSecCmdline[]="Cmdline";
static const char NSecInterface[]="Interface";
static const char NSecInterfaceCompletion[]="Interface/Completion";
static const char NSecViewer[]="Viewer";
static const char NSecDialog[]="Dialog";
static const char NSecEditor[]="Editor";
static const char NSecNotifications[]="Notifications";
static const char NSecXLat[]="XLat";
static const char NSecSystem[]="System";
static const char NSecHelp[]="Help";
static const char NSecLanguage[]="Language";
static const char NSecConfirmations[]="Confirmations";
static const char NSecPluginConfirmations[]="PluginConfirmations";
static const char NSecPanel[]="Panel";
static const char NSecPanelLeft[]="Panel/Left";
static const char NSecPanelRight[]="Panel/Right";
static const char NSecPanelLayout[]="Panel/Layout";
static const char NSecPanelTree[]="Panel/Tree";
static const char NSecLayout[]="Layout";
static const char NSecDescriptions[]="Descriptions";
static const char NSecMacros[]="Macros";
static const char NSecSavedHistory[]="SavedHistory";
static const char NSecSavedViewHistory[]="SavedViewHistory";
static const char NSecSavedFolderHistory[]="SavedFolderHistory";
static const char NSecSavedDialogHistory[]="SavedDialogHistory";
static const char NSecCodePages[]="CodePages";
static const char NParamHistoryCount[]="HistoryCount";
static const char NSecVMenu[]="VMenu";

// Структура, описывающая всю конфигурацию(!)
static struct FARConfig
{
	bool  IsSave;   // будет записываться в ConfigOptSave()
	OPT_TYPE ValType;
	const char *KeyName;
	const char *ValName;
	union {
		void      *ValPtr;   // адрес переменной, куда помещаем данные
		FARString *StrPtr;
	};
	DWORD DefDWord; // он же размер данных для OPT_BINARY
	union {
	  const wchar_t *DefStr;   // строка по умолчанию
	  const BYTE    *DefArr;   // данные по умолчанию
	};

	constexpr FARConfig(bool save, const char *key, const char *val, DWORD size, BYTE *trg, const BYTE *dflt) :
		IsSave(save),ValType(OPT_BINARY),KeyName(key),ValName(val),ValPtr(trg),DefDWord(size),DefArr(dflt) {}

	constexpr FARConfig(bool save, const char *key, const char *val, void *trg, DWORD dflt, OPT_TYPE Type=OPT_DWORD) :
		IsSave(save),ValType(Type),KeyName(key),ValName(val),ValPtr(trg),DefDWord(dflt),DefStr(nullptr) {}

	constexpr FARConfig(bool save, const char *key, const char *val, FARString *trg, const wchar_t *dflt) :
		IsSave(save),ValType(OPT_SZ),KeyName(key),ValName(val),StrPtr(trg),DefDWord(0),DefStr(dflt) {}

} CFG[]=
{
//	{false, NSecColors, "CurrentPalette", SIZE_ARRAY_PALETTE, (BYTE *)Palette8bit, nullptr},
//	{true,  NSecColors, "CurrentPaletteRGB", SIZE_ARRAY_PALETTE * sizeof(uint64_t), (BYTE *)Palette, nullptr},
	{true,  NSecColors, "TempColors256", TEMP_COLORS256_SIZE, g_tempcolors256, nullptr},
	{true,  NSecColors, "TempColorsRGB", TEMP_COLORSRGB_SIZE, (BYTE *)g_tempcolorsRGB, nullptr},

	{true,  NSecScreen, "Clock",                        &Opt.Clock, 1, OPT_BOOLEAN},
	{true,  NSecScreen, "ViewerEditorClock",            &Opt.ViewerEditorClock, 0, OPT_BOOLEAN},
	{true,  NSecScreen, "KeyBar",                       &Opt.ShowKeyBar, 1, OPT_BOOLEAN},
	{true,  NSecScreen, "ScreenSaver",                  &Opt.ScreenSaver, 0, OPT_BOOLEAN},
	{true,  NSecScreen, "ScreenSaverTime",              &Opt.ScreenSaverTime, 5},
	{true,  NSecScreen, "CursorBlinkInterval",          &Opt.CursorBlinkTime, 500},

	{true,  NSecCmdline, "UsePromptFormat",             &Opt.CmdLine.UsePromptFormat, 0, OPT_BOOLEAN},
	{true,  NSecCmdline, "PromptFormat",                &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{true,  NSecCmdline, "UseShell",                    &Opt.CmdLine.UseShell, 0, OPT_BOOLEAN},
	{true,  NSecCmdline, "ShellCmd",                    &Opt.CmdLine.strShell, L"bash -i"},
	{true,  NSecCmdline, "DelRemovesBlocks",            &Opt.CmdLine.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{true,  NSecCmdline, "EditBlock",                   &Opt.CmdLine.EditBlock, 0, OPT_BOOLEAN},
	{true,  NSecCmdline, "AutoComplete",                &Opt.CmdLine.AutoComplete, 1, OPT_BOOLEAN},
	{true,  NSecCmdline, "Splitter",                    &Opt.CmdLine.Splitter, 1, OPT_BOOLEAN},
	{true,  NSecCmdline, "WaitKeypress",                &Opt.CmdLine.WaitKeypress, 1},
	{true,  NSecCmdline, "VTLogLimit",                  &Opt.CmdLine.VTLogLimit, 5000},
	{true,  NSecCmdline, "ImitateNumpadKeys",           &Opt.CmdLine.ImitateNumpadKeys, 1, OPT_BOOLEAN},
	{false, NSecCmdline, "AskOnMultilinePaste",         &Opt.CmdLine.AskOnMultilinePaste, 1, OPT_BOOLEAN},

	{true,  NSecInterface, "Mouse",                     &Opt.Mouse, 1, OPT_BOOLEAN},
	{false, NSecInterface, "UseVk_oem_x",               &Opt.UseVk_oem_x, 1, OPT_BOOLEAN},
	{true,  NSecInterface, "ShowMenuBar",               &Opt.ShowMenuBar, 0, OPT_BOOLEAN},
	{false, NSecInterface, "CursorSize1",               &Opt.CursorSize[0], 15},
	{false, NSecInterface, "CursorSize2",               &Opt.CursorSize[1], 10},
	{false, NSecInterface, "CursorSize3",               &Opt.CursorSize[2], 99},
	{false, NSecInterface, "CursorSize4",               &Opt.CursorSize[3], 99},
	{false, NSecInterface, "ShiftsKeyRules",            &Opt.ShiftsKeyRules, 1, OPT_BOOLEAN},
	{true,  NSecInterface, "CtrlPgUp",                  &Opt.PgUpChangeDisk, 1, OPT_BOOLEAN},

	{true,  NSecInterface, "ConsolePaintSharp",         &Opt.ConsolePaintSharp, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveCtrlLeft",         &Opt.ExclusiveCtrlLeft, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveCtrlRight",        &Opt.ExclusiveCtrlRight, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveAltLeft",          &Opt.ExclusiveAltLeft, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveAltRight",         &Opt.ExclusiveAltRight, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveWinLeft",          &Opt.ExclusiveWinLeft, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "ExclusiveWinRight",         &Opt.ExclusiveWinRight, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "UseStickyKeyEvent",         &Opt.UseStickyKeyEvent, 0, OPT_BOOLEAN},

	{true,  NSecInterface, "DateFormat",                &Opt.DateFormat, (DWORD) GetDateFormatDefault()},
	{true,  NSecInterface, "DateSeparator",             &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{true,  NSecInterface, "TimeSeparator",             &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{true,  NSecInterface, "DecimalSeparator",          &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

#if defined(__ANDROID__)
	{true,  NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 1, OPT_BOOLEAN},
#else
	{true,  NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 0, OPT_BOOLEAN},
#endif

	{true,  NSecInterface, "TTYPaletteOverride",        &Opt.TTYPaletteOverride, 1, OPT_BOOLEAN},
	{false, NSecInterface, "ShowTimeoutDelFiles",       &Opt.ShowTimeoutDelFiles, 50},
	{false, NSecInterface, "ShowTimeoutDACLFiles",      &Opt.ShowTimeoutDACLFiles, 50},
	{false, NSecInterface, "FormatNumberSeparators",    &Opt.FormatNumberSeparators, 0},
	{true,  NSecInterface, "CopyShowTotal",             &Opt.CMOpt.CopyShowTotal, 1, OPT_BOOLEAN},
	{true,  NSecInterface, "DelShowTotal",              &Opt.DelOpt.DelShowTotal, 0, OPT_BOOLEAN},
	{true,  NSecInterface, "WindowTitle",               &Opt.strWindowTitle, L"%State - FAR2M %Ver %Backend %User@%Host"}, // %Platform
	{true,  NSecInterfaceCompletion, "Exceptions",      &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
	{true,  NSecInterfaceCompletion, "ShowList",        &Opt.AutoComplete.ShowList, 1, OPT_BOOLEAN},
	{true,  NSecInterfaceCompletion, "ModalList",       &Opt.AutoComplete.ModalList, 0, OPT_BOOLEAN},
	{true,  NSecInterfaceCompletion, "Append",          &Opt.AutoComplete.AppendCompletion, 0, OPT_BOOLEAN},

	{true,  NSecViewer, "ExternalViewerName",           &Opt.strExternalViewer, L""},
	{true,  NSecViewer, "UseExternalViewer",            &Opt.ViOpt.UseExternalViewer, 0, OPT_BOOLEAN},
	{true,  NSecViewer, "SaveViewerPos",                &Opt.ViOpt.SavePos, 1, OPT_BOOLEAN},
	{true,  NSecViewer, "SaveViewerShortPos",           &Opt.ViOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{true,  NSecViewer, "AutoDetectCodePage",           &Opt.ViOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{true,  NSecViewer, "SearchRegexp",                 &Opt.ViOpt.SearchRegexp, 0, OPT_BOOLEAN},

	{true,  NSecViewer, "TabSize",                      &Opt.ViOpt.TabSize, 8},
	{true,  NSecViewer, "ShowKeyBar",                   &Opt.ViOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{true,  NSecViewer, "ShowTitleBar",                 &Opt.ViOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{true,  NSecViewer, "ShowArrows",                   &Opt.ViOpt.ShowArrows, 1, OPT_BOOLEAN},
	{true,  NSecViewer, "ShowScrollbar",                &Opt.ViOpt.ShowScrollbar, 0, OPT_BOOLEAN},
	{true,  NSecViewer, "IsWrap",                       &Opt.ViOpt.ViewerIsWrap, 1},
	{true,  NSecViewer, "Wrap",                         &Opt.ViOpt.ViewerWrap, 0},
	{true,  NSecViewer, "PersistentBlocks",             &Opt.ViOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{true,  NSecViewer, "DefaultCodePage",              &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{true,  NSecDialog, "EditHistory",                  &Opt.Dialogs.EditHistory, 1, OPT_BOOLEAN},
	{true,  NSecDialog, "EditBlock",                    &Opt.Dialogs.EditBlock, 0, OPT_BOOLEAN},
	{true,  NSecDialog, "AutoComplete",                 &Opt.Dialogs.AutoComplete, 1, OPT_BOOLEAN},
	{true,  NSecDialog, "EULBsClear",                   &Opt.Dialogs.EULBsClear, 0, OPT_BOOLEAN},
	{false, NSecDialog, "EditLine",                     &Opt.Dialogs.EditLine, 0},
	{true,  NSecDialog, "MouseButton",                  &Opt.Dialogs.MouseButton, 0xFFFF},
	{true,  NSecDialog, "DelRemovesBlocks",             &Opt.Dialogs.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{false, NSecDialog, "CBoxMaxHeight",                &Opt.Dialogs.CBoxMaxHeight, 8},

	{true,  NSecEditor, "ExternalEditorName",           &Opt.strExternalEditor, L""},
	{true,  NSecEditor, "UseExternalEditor",            &Opt.EdOpt.UseExternalEditor, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "ExpandTabs",                   &Opt.EdOpt.ExpandTabs, EXPAND_NOTABS},
	{true,  NSecEditor, "TabSize",                      &Opt.EdOpt.TabSize, 8},
	{true,  NSecEditor, "PersistentBlocks",             &Opt.EdOpt.PersistentBlocks, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "DelRemovesBlocks",             &Opt.EdOpt.DelRemovesBlocks, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "AutoIndent",                   &Opt.EdOpt.AutoIndent, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "SaveEditorPos",                &Opt.EdOpt.SavePos, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "SaveEditorShortPos",           &Opt.EdOpt.SaveShortPos, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "AutoDetectCodePage",           &Opt.EdOpt.AutoDetectCodePage, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "EditorCursorBeyondEOL",        &Opt.EdOpt.CursorBeyondEOL, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "ReadOnlyLock",                 &Opt.EdOpt.ReadOnlyLock, 0},
	{false, NSecEditor, "EditorUndoSize",               &Opt.EdOpt.UndoSize, 0},
	{false, NSecEditor, "WordDiv",                      &Opt.strWordDiv, WordDiv0},
	{false, NSecEditor, "BSLikeDel",                    &Opt.EdOpt.BSLikeDel, 1, OPT_BOOLEAN},
	{false, NSecEditor, "FileSizeLimit",                &Opt.EdOpt.FileSizeLimitLo, 0},
	{false, NSecEditor, "FileSizeLimitHi",              &Opt.EdOpt.FileSizeLimitHi, 0},
	{false, NSecEditor, "CharCodeBase",                 &Opt.EdOpt.CharCodeBase, 1},
	{false, NSecEditor, "AllowEmptySpaceAfterEof",      &Opt.EdOpt.AllowEmptySpaceAfterEof, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "DefaultCodePage",              &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{true,  NSecEditor, "ShowKeyBar",                   &Opt.EdOpt.ShowKeyBar, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "ShowTitleBar",                 &Opt.EdOpt.ShowTitleBar, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "ShowScrollBar",                &Opt.EdOpt.ShowScrollBar, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "EditOpenedForWrite",           &Opt.EdOpt.EditOpenedForWrite, 1, OPT_BOOLEAN},
	{true,  NSecEditor, "SearchSelFound",               &Opt.EdOpt.SearchSelFound, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "SearchRegexp",                 &Opt.EdOpt.SearchRegexp, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "SearchPickUpWord",             &Opt.EdOpt.SearchPickUpWord, 0, OPT_BOOLEAN},
	{true,  NSecEditor, "ShowWhiteSpace",               &Opt.EdOpt.ShowWhiteSpace, 0, OPT_BOOLEAN},

	{true,  NSecNotifications, "OnFileOperation",       &Opt.NotifOpt.OnFileOperation, 1, OPT_BOOLEAN},
	{true,  NSecNotifications, "OnConsole",             &Opt.NotifOpt.OnConsole, 1, OPT_BOOLEAN},
	{true,  NSecNotifications, "OnlyIfBackground",      &Opt.NotifOpt.OnlyIfBackground, 1, OPT_BOOLEAN},

	{false, NSecXLat, "Flags",                          &Opt.XLat.Flags, XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{true,  NSecXLat, "EnableForFastFileFind",          &Opt.XLat.EnableForFastFileFind, 1, OPT_BOOLEAN},
	{true,  NSecXLat, "EnableForDialogs",               &Opt.XLat.EnableForDialogs, 1, OPT_BOOLEAN},
	{true,  NSecXLat, "WordDivForXlat",                 &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{true,  NSecXLat, "XLat",                           &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{true,  NSecSavedHistory, NParamHistoryCount,       &Opt.HistoryCount, 512},
	{true,  NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{true,  NSecSavedViewHistory, NParamHistoryCount,   &Opt.ViewHistoryCount, 512},
	{true,  NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{true,  NSecSystem, "PersonalPluginsPath",          &Opt.LoadPlug.strPersonalPluginsPath, L""},
	{true,  NSecSystem, "HistoryShowDates",             &Opt.HistoryShowDates, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "SaveHistory",                  &Opt.SaveHistory, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "SaveFoldersHistory",           &Opt.SaveFoldersHistory, 1, OPT_BOOLEAN},
	{false, NSecSystem, "SavePluginFoldersHistory",     &Opt.SavePluginFoldersHistory, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "SaveViewHistory",              &Opt.SaveViewHistory, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "AutoSaveSetup",                &Opt.AutoSaveSetup, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "DeleteToRecycleBin",           &Opt.DeleteToRecycleBin, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "DeleteToRecycleBinKillLink",   &Opt.DeleteToRecycleBinKillLink, 1, OPT_BOOLEAN},
	{false, NSecSystem, "WipeSymbol",                   &Opt.WipeSymbol, 0},
	{true,  NSecSystem, "SudoEnabled",                  &Opt.SudoEnabled, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "SudoConfirmModify",            &Opt.SudoConfirmModify, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "SudoPasswordExpiration",       &Opt.SudoPasswordExpiration, 15*60},

	{true,  NSecSystem, "UseCOW",                       &Opt.CMOpt.UseCOW, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "SparseFiles",                  &Opt.CMOpt.SparseFiles, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "HowCopySymlink",               &Opt.CMOpt.HowCopySymlink, 1},
	{true,  NSecSystem, "WriteThrough",                 &Opt.CMOpt.WriteThrough, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "CopyXAttr",                    &Opt.CMOpt.CopyXAttr, 0, OPT_BOOLEAN},
	{false, NSecSystem, "CopyAccessMode",               &Opt.CMOpt.CopyAccessMode, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "MultiCopy",                    &Opt.CMOpt.MultiCopy, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "CopyTimeRule",                 &Opt.CMOpt.CopyTimeRule, 3},

	{true,  NSecSystem, "MakeLinkSuggestSymlinkAlways", &Opt.MakeLinkSuggestSymlinkAlways, 1, OPT_BOOLEAN},

	{true,  NSecSystem, "InactivityExit",               &Opt.InactivityExit, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "InactivityExitTime",           &Opt.InactivityExitTime, 15},
	{true,  NSecSystem, "DriveMenuMode2",               &Opt.ChangeDriveMode, (DWORD)-1},
	{true,  NSecSystem, "DriveDisconnectMode",          &Opt.ChangeDriveDisconnectMode, 1},

	{true,  NSecSystem, "DriveExceptions",              &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;"
		"/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron"},
	{true,  NSecSystem, "DriveColumn2",                 &Opt.ChangeDriveColumn2, L"$U/$T"},
	{true,  NSecSystem, "DriveColumn3",                 &Opt.ChangeDriveColumn3, L"$S$D"},

	{true,  NSecSystem, "AutoUpdateRemoteDrive",        &Opt.AutoUpdateRemoteDrive, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "FileSearchMode",               &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{false, NSecSystem, "CollectFiles",                 &Opt.FindOpt.CollectFiles, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "SearchInFirstSize",            &Opt.FindOpt.strSearchInFirstSize, L""},
	{true,  NSecSystem, "FindAlternateStreams",         &Opt.FindOpt.FindAlternateStreams, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "SearchOutFormat",              &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{true,  NSecSystem, "SearchOutFormatWidth",         &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{true,  NSecSystem, "FindFolders",                  &Opt.FindOpt.FindFolders, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "FindSymLinks",                 &Opt.FindOpt.FindSymLinks, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "FindCaseSensitiveFileMask",    &Opt.FindOpt.FindCaseSensitiveFileMask, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "UseFilterInSearch",            &Opt.FindOpt.UseFilter, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "FindCodePage",                 &Opt.FindCodePage, CP_AUTODETECT},
	{false, NSecSystem, "CmdHistoryRule",               &Opt.CmdHistoryRule, 0, OPT_BOOLEAN},
	{false, NSecSystem, "SetAttrFolderRules",           &Opt.SetAttrFolderRules, 1, OPT_BOOLEAN},
	{false, NSecSystem, "MaxPositionCache",             &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{false, NSecSystem, "ConsoleDetachKey",             &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{false, NSecSystem, "SilentLoadPlugin",             &Opt.LoadPlug.SilentLoadPlugin, 0, OPT_BOOLEAN},
	{true,  NSecSystem, "ScanSymlinks",                 &Opt.LoadPlug.ScanSymlinks, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "MultiMakeDir",                 &Opt.MultiMakeDir, 0, OPT_BOOLEAN},
	{false, NSecSystem, "MsWheelDelta",                 &Opt.MsWheelDelta, 1},
	{false, NSecSystem, "MsWheelDeltaView",             &Opt.MsWheelDeltaView, 1},
	{false, NSecSystem, "MsWheelDeltaEdit",             &Opt.MsWheelDeltaEdit, 1},
	{false, NSecSystem, "MsWheelDeltaHelp",             &Opt.MsWheelDeltaHelp, 1},
	{false, NSecSystem, "MsHWheelDelta",                &Opt.MsHWheelDelta, 1},
	{false, NSecSystem, "MsHWheelDeltaView",            &Opt.MsHWheelDeltaView, 1},
	{false, NSecSystem, "MsHWheelDeltaEdit",            &Opt.MsHWheelDeltaEdit, 1},
	{false, NSecSystem, "SubstNameRule",                &Opt.SubstNameRule, 2},
	{false, NSecSystem, "ShowCheckingFile",             &Opt.ShowCheckingFile, 0, OPT_BOOLEAN},
	{false, NSecSystem, "QuotedSymbols",                &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{false, NSecSystem, "QuotedName",                   &Opt.QuotedName, QUOTEDNAME_INSERT},
	{false, NSecSystem, "PluginMaxReadData",            &Opt.PluginMaxReadData, 0x40000},
	{false, NSecSystem, "CASRule",                      &Opt.CASRule, 0xFFFFFFFFU},
	{false, NSecSystem, "AllCtrlAltShiftRule",          &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{true,  NSecSystem, "ScanJunction",                 &Opt.ScanJunction, 1, OPT_BOOLEAN},
	{true,  NSecSystem, "OnlyFilesSize",                &Opt.OnlyFilesSize, 0, OPT_BOOLEAN},
	{false, NSecSystem, "UsePrintManager",              &Opt.UsePrintManager, 1, OPT_BOOLEAN},
	{false, NSecSystem, "WindowMode",                   &Opt.WindowMode, 0, OPT_BOOLEAN},

	{false, NSecPanelTree, "MinTreeCount",              &Opt.Tree.MinTreeCount, 4},
	{false, NSecPanelTree, "TreeFileAttr",              &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{false, NSecPanelTree, "LocalDisk",                 &Opt.Tree.LocalDisk, 2},
	{false, NSecPanelTree, "NetDisk",                   &Opt.Tree.NetDisk, 2},
	{false, NSecPanelTree, "RemovableDisk",             &Opt.Tree.RemovableDisk, 2},
	{false, NSecPanelTree, "NetPath",                   &Opt.Tree.NetPath, 2},
	{true,  NSecPanelTree, "AutoChangeFolder",          &Opt.Tree.AutoChangeFolder, 0, OPT_BOOLEAN}, // ???

	{false, NSecHelp, "ActivateURL",                    &Opt.HelpURLRules, 1},

	{true,  NSecLanguage, "Help",                       &Opt.strHelpLanguage, L"English"},
	{true,  NSecLanguage, "Main",                       &Opt.strLanguage, L"English"},

	{true,  NSecConfirmations, "Copy",                  &Opt.Confirm.Copy, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "Move",                  &Opt.Confirm.Move, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "RO",                    &Opt.Confirm.RO, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "Drag",                  &Opt.Confirm.Drag, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "Delete",                &Opt.Confirm.Delete, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "DeleteFolder",          &Opt.Confirm.DeleteFolder, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "Esc",                   &Opt.Confirm.Esc, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "RemoveConnection",      &Opt.Confirm.RemoveConnection, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "RemoveHotPlug",         &Opt.Confirm.RemoveHotPlug, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "AllowReedit",           &Opt.Confirm.AllowReedit, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "HistoryClear",          &Opt.Confirm.HistoryClear, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "Exit",                  &Opt.Confirm.Exit, 1, OPT_BOOLEAN},
	{true,  NSecConfirmations, "ExitOrBknd",            &Opt.Confirm.ExitOrBknd, 1, OPT_BOOLEAN},
	{false, NSecConfirmations, "EscTwiceToInterrupt",   &Opt.Confirm.EscTwiceToInterrupt, 0, OPT_BOOLEAN},

	{true,  NSecPluginConfirmations, "OpenFilePlugin",  &Opt.PluginConfirm.OpenFilePlugin, 0, OPT_3STATE},
	{true,  NSecPluginConfirmations, "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, OPT_BOOLEAN},
	{true,  NSecPluginConfirmations, "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, OPT_BOOLEAN},
	{true,  NSecPluginConfirmations, "SetFindList",     &Opt.PluginConfirm.SetFindList, 0, OPT_BOOLEAN},
	{true,  NSecPluginConfirmations, "Prefix",          &Opt.PluginConfirm.Prefix, 0, OPT_BOOLEAN},

	{false, NSecPanel, "ShellRightLeftArrowsRule",      &Opt.ShellRightLeftArrowsRule, 0, OPT_BOOLEAN},
	{true,  NSecPanel, "ShowHidden",                    &Opt.ShowHidden, 1, OPT_BOOLEAN},
	{true,  NSecPanel, "Highlight",                     &Opt.Highlight, 1, OPT_BOOLEAN},
	{true,  NSecPanel, "SortFolderExt",                 &Opt.SortFolderExt, 0, OPT_BOOLEAN},
	{true,  NSecPanel, "SelectFolders",                 &Opt.SelectFolders, 0, OPT_BOOLEAN},
	{true,  NSecPanel, "AttrStrStyle",                  &Opt.AttrStrStyle, 1},
	{true,  NSecPanel, "CaseSensitiveCompareSelect",    &Opt.PanelCaseSensitiveCompareSelect, 0, OPT_BOOLEAN},
	{true,  NSecPanel, "ReverseSort",                   &Opt.ReverseSort, 1, OPT_BOOLEAN},
	{false, NSecPanel, "RightClickRule",                &Opt.PanelRightClickRule, 2, OPT_3STATE},
	{false, NSecPanel, "CtrlFRule",                     &Opt.PanelCtrlFRule, 1, OPT_BOOLEAN},
	{false, NSecPanel, "CtrlAltShiftRule",              &Opt.PanelCtrlAltShiftRule, 0, OPT_3STATE},
	{false, NSecPanel, "RememberLogicalDrives",         &Opt.RememberLogicalDrives, 0, OPT_BOOLEAN},
	{true,  NSecPanel, "AutoUpdateLimit",               &Opt.AutoUpdateLimit, 0},
	{true,  NSecPanel, "ShowFilenameMarks",             &Opt.ShowFilenameMarks, 1, OPT_BOOLEAN},
	{true,  NSecPanel, "FilenameMarksAlign",            &Opt.FilenameMarksAlign, 1, OPT_BOOLEAN},
	{true,  NSecPanel, "MinFilenameIndentation",        &Opt.MinFilenameIndentation, 0},
	{true,  NSecPanel, "MaxFilenameIndentation",        &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},
	{true,  NSecPanel, "ClassicHotkeyLinkResolving",    &Opt.ClassicHotkeyLinkResolving, 1, OPT_BOOLEAN},

	{true,  NSecPanelLeft, "Type",                      &Opt.LeftPanel.Type, FILE_PANEL},
	{true,  NSecPanelLeft, "Visible",                   &Opt.LeftPanel.Visible, 1, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "Focus",                     &Opt.LeftPanel.Focus, 1, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "ViewMode",                  &Opt.LeftPanel.ViewMode, VIEW_2},
	{true,  NSecPanelLeft, "SortMode",                  &Opt.LeftPanel.SortMode, PanelSortMode::BY_NAME},
	{true,  NSecPanelLeft, "SortOrder",                 &Opt.LeftPanel.SortOrder, 1},
	{true,  NSecPanelLeft, "SortGroups",                &Opt.LeftPanel.SortGroups, 0, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "NumericSort",               &Opt.LeftPanel.NumericSort, 0, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "CaseSensitiveSort",         &Opt.LeftPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "Folder",                    &Opt.strLeftFolder, L""},
	{true,  NSecPanelLeft, "CurFile",                   &Opt.strLeftCurFile, L""},
	{true,  NSecPanelLeft, "SelectedFirst",             &Opt.LeftSelectedFirst, 0, OPT_BOOLEAN},
	{true,  NSecPanelLeft, "DirectoriesFirst",          &Opt.LeftPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{true,  NSecPanelRight, "Type",                     &Opt.RightPanel.Type, FILE_PANEL},
	{true,  NSecPanelRight, "Visible",                  &Opt.RightPanel.Visible, 1, OPT_BOOLEAN},
	{true,  NSecPanelRight, "Focus",                    &Opt.RightPanel.Focus, 0, OPT_BOOLEAN},
	{true,  NSecPanelRight, "ViewMode",                 &Opt.RightPanel.ViewMode, VIEW_2},
	{true,  NSecPanelRight, "SortMode",                 &Opt.RightPanel.SortMode, PanelSortMode::BY_NAME},
	{true,  NSecPanelRight, "SortOrder",                &Opt.RightPanel.SortOrder, 1},
	{true,  NSecPanelRight, "SortGroups",               &Opt.RightPanel.SortGroups, 0, OPT_BOOLEAN},
	{true,  NSecPanelRight, "NumericSort",              &Opt.RightPanel.NumericSort, 0, OPT_BOOLEAN},
	{true,  NSecPanelRight, "CaseSensitiveSort",        &Opt.RightPanel.CaseSensitiveSort, 0, OPT_BOOLEAN},
	{true,  NSecPanelRight, "Folder",                   &Opt.strRightFolder, L""},
	{true,  NSecPanelRight, "CurFile",                  &Opt.strRightCurFile, L""},
	{true,  NSecPanelRight, "SelectedFirst",            &Opt.RightSelectedFirst, 0, OPT_BOOLEAN},
	{true,  NSecPanelRight, "DirectoriesFirst",         &Opt.RightPanel.DirectoriesFirst, 1, OPT_BOOLEAN},

	{true,  NSecPanelLayout, "ColumnTitles",            &Opt.ShowColumnTitles, 1, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "StatusLine",              &Opt.ShowPanelStatus, 1, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "TotalInfo",               &Opt.ShowPanelTotals, 1, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "FreeInfo",                &Opt.ShowPanelFree, 0, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "Scrollbar",               &Opt.ShowPanelScrollbar, 0, OPT_BOOLEAN},
	{false, NSecPanelLayout, "ScrollbarMenu",           &Opt.ShowMenuScrollbar, 1, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "ScreensNumber",           &Opt.ShowScreensNumber, 1, OPT_BOOLEAN},
	{true,  NSecPanelLayout, "SortMode",                &Opt.ShowSortMode, 1, OPT_BOOLEAN},

	{true,  NSecLayout, "LeftHeightDecrement",          &Opt.LeftHeightDecrement, 0},
	{true,  NSecLayout, "RightHeightDecrement",         &Opt.RightHeightDecrement, 0},
	{true,  NSecLayout, "WidthDecrement",               &Opt.WidthDecrement, 0},
	{true,  NSecLayout, "FullscreenHelp",               &Opt.FullScreenHelp, 0, OPT_BOOLEAN},

	{true,  NSecDescriptions, "ListNames",              &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{true,  NSecDescriptions, "UpdateMode",             &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{true,  NSecDescriptions, "ROUpdate",               &Opt.Diz.ROUpdate, 0, OPT_BOOLEAN},
	{true,  NSecDescriptions, "SetHidden",              &Opt.Diz.SetHidden, 1, OPT_BOOLEAN},
	{true,  NSecDescriptions, "StartPos",               &Opt.Diz.StartPos, 0},
	{true,  NSecDescriptions, "AnsiByDefault",          &Opt.Diz.AnsiByDefault, 0, OPT_BOOLEAN},
	{true,  NSecDescriptions, "SaveInUTF",              &Opt.Diz.SaveInUTF, 0, OPT_BOOLEAN},

	{false, NSecMacros, "DateFormat",                   &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{true,  NSecMacros, "ShowPlayIndicator",            &Opt.Macro.ShowPlayIndicator, 1, OPT_BOOLEAN},
	{true,  NSecMacros, "KeyRecordCtrlDot",             &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot},
	{true,  NSecMacros, "KeyRecordCtrlShiftDot",        &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot},

	{true,  NSecSystem, "ExcludeCmdHistory",            &Opt.ExcludeCmdHistory, 0},

	{true,  NSecCodePages, "CPMenuMode",                &Opt.CPMenuMode, 0, OPT_BOOLEAN},

	{true,  NSecSystem, "FolderInfo",                   &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{true,  NSecVMenu, "MenuLoopScroll",                &Opt.VMenu.MenuLoopScroll, 0, OPT_BOOLEAN},

	{true,  NSecVMenu, "LBtnClick",                     &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "RBtnClick",                     &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{true,  NSecVMenu, "MBtnClick",                     &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
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

bool ConfigOptGetValue(int I, GetConfig& Data)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG))
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
				Data.dwValue = *(DWORD*)CFG[I].ValPtr;
				break;
			case OPT_SZ:
				Data.strDefault = CFG[I].DefStr;
				Data.strValue = *CFG[I].StrPtr;
				break;
			case OPT_BINARY:
				Data.binDefault = CFG[I].DefArr;
				Data.binData = CFG[I].ValPtr;
				Data.binSize = CFG[I].DefDWord;
				break;
		}
		return true;
	}
	return false;
}

bool ConfigOptSetInteger(int I, DWORD Value)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG))
	{
		switch(CFG[I].ValType)
		{
			case OPT_DWORD:
				*(DWORD*)CFG[I].ValPtr = Value;
				break;
			case OPT_BOOLEAN:
				*(DWORD*)CFG[I].ValPtr = Value ? 1 : 0;
				break;
			case OPT_3STATE:
				*(DWORD*)CFG[I].ValPtr = Value % 3;
				break;
			default:
				return false;
		}
		return true;
	}
	return false;
}

bool ConfigOptSetString(int I, const wchar_t *Value)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG) && CFG[I].ValType == OPT_SZ && Value)
	{
		*CFG[I].StrPtr = Value;
		return true;
	}
	return false;
}

bool ConfigOptSetBinary(int I, const void *Data, DWORD Size)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG) && CFG[I].ValType == OPT_BINARY && Data)
	{
		Size = std::min(Size, CFG[I].DefDWord);
		memcpy(CFG[I].ValPtr, Data, Size);
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
			GetConfig cfg;
			int Index = ConfigOptGetIndex(strName.CPtr());
			if (ConfigOptGetValue(Index, cfg))
			{
				switch (cfg.ValType)
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
				*(DWORD*)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord);
				break;
			case OPT_BOOLEAN:
				*(DWORD*)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord) ? 1 : 0;
				break;
			case OPT_3STATE:
				*(DWORD*)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, CFG[I].DefDWord) % 3;
				break;
			case OPT_SZ:
				*CFG[I].StrPtr = cfg_reader.GetString(CFG[I].ValName, CFG[I].DefStr);
				break;
			case OPT_BINARY:
				int Size = cfg_reader.GetBytes((BYTE*)CFG[I].ValPtr, CFG[I].DefDWord, CFG[I].ValName, CFG[I].DefArr);
				if (Size > 0 && Size < (int)CFG[I].DefDWord)
					memset(((BYTE*)CFG[I].ValPtr)+Size,0,CFG[I].DefDWord-Size);

				break;
		}
	}

	/* Command line config modifiers */
	ConfigOptFromCmdLine();

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();
	SanitizeIndentationCounts();

	if (Opt.CursorBlinkTime < 100)
		Opt.CursorBlinkTime = 100;

	if (Opt.CursorBlinkTime > 500)
		Opt.CursorBlinkTime = 500;

	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar=1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData=0x1000;

	if(ExplicitWindowMode)
	{
		Opt.WindowMode=TRUE;
	}

	Opt.HelpTabSize=8; // пока жестко пропишем...
//	SanitizePalette();
//	MergePalette();

	Opt.ViOpt.ViewerIsWrap&=1;
	Opt.ViOpt.ViewerWrap&=1;

	// Исключаем случайное стирание разделителей ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// Исключаем случайное стирание разделителей
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	Opt.PanelRightClickRule%=3;
	Opt.PanelCtrlAltShiftRule%=3;
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

void ConfigOptSave(bool Ask)
{
	if (Ask && Message(0,2,Msg::SaveSetupTitle,Msg::SaveSetupAsk1,Msg::SaveSetupAsk2,Msg::SaveSetup,Msg::Cancel))
		return;

	WINPORT(SaveConsoleWindowState)();

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel=CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus=LeftPanel->GetFocus();
	Opt.LeftPanel.Visible=LeftPanel->IsVisible();
	Opt.RightPanel.Focus=RightPanel->GetFocus();
	Opt.RightPanel.Visible=RightPanel->IsVisible();

	if (LeftPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.LeftPanel.Type=LeftPanel->GetType();
		Opt.LeftPanel.ViewMode=LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode=LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder=LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups=LeftPanel->GetSortGroups();
		Opt.LeftPanel.NumericSort=LeftPanel->GetNumericSort();
		Opt.LeftPanel.CaseSensitiveSort=LeftPanel->GetCaseSensitiveSort();
		Opt.LeftSelectedFirst=LeftPanel->GetSelectedFirstMode();
		Opt.LeftPanel.DirectoriesFirst=LeftPanel->GetDirectoriesFirst();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile);

	if (RightPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.RightPanel.Type=RightPanel->GetType();
		Opt.RightPanel.ViewMode=RightPanel->GetViewMode();
		Opt.RightPanel.SortMode=RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder=RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups=RightPanel->GetSortGroups();
		Opt.RightPanel.NumericSort=RightPanel->GetNumericSort();
		Opt.RightPanel.CaseSensitiveSort=RightPanel->GetCaseSensitiveSort();
		Opt.RightSelectedFirst=RightPanel->GetSelectedFirstMode();
		Opt.RightPanel.DirectoriesFirst=RightPanel->GetDirectoriesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
	CtrlObject->HiFiles->SaveHiData();

	ConfigWriter cfg_writer;

	/* *************************************************** </ПРЕПРОЦЕССЫ> */
//	cfg_writer.SetString(NSecLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].IsSave)
		{
			cfg_writer.SelectSection(CFG[I].KeyName);
			switch (CFG[I].ValType)
			{
				case OPT_DWORD:
				case OPT_BOOLEAN:
				case OPT_3STATE:
					cfg_writer.SetUInt(CFG[I].ValName, *(DWORD*)CFG[I].ValPtr);
					break;
				case OPT_SZ:
					cfg_writer.SetString(CFG[I].ValName, CFG[I].StrPtr->CPtr());
					break;
				case OPT_BINARY:
					cfg_writer.SetBytes(CFG[I].ValName, (const BYTE*)CFG[I].ValPtr, CFG[I].DefDWord);
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
