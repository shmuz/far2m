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
#include "palette.hpp"
#include "panelmix.hpp"
#include "poscache.hpp"
#include "pick_color256.hpp"
#include "pick_colorRGB.hpp"
#include "strmix.hpp"

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
static const char NSecPolicies[]="Policies";
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
	int   IsSave;   // =1 - будет записываться в ConfigOptSave()
	DWORD ValType;  // REG_DWORD, REG_SZ, REG_BINARY
	const char *KeyName;
	const char *ValName;
	union {
		void      *ValPtr;   // адрес переменной, куда помещаем данные
		FARString *StrPtr;
	};
	DWORD DefDWord; // он же размер данных для REG_BINARY
	union {
	  const wchar_t *DefStr;   // строка по умолчанию
	  const BYTE    *DefArr;   // данные по умолчанию
	};

	constexpr FARConfig(int save, const char *key, const char *val, BYTE *trg, DWORD size, const BYTE *dflt) :
		IsSave(save),ValType(REG_BINARY),KeyName(key),ValName(val),ValPtr(trg),DefDWord(size),DefArr(dflt) {}

	constexpr FARConfig(int save, const char *key, const char *val, void *trg, DWORD dflt, DWORD Type=REG_DWORD) :
		IsSave(save),ValType(Type),KeyName(key),ValName(val),ValPtr(trg),DefDWord(dflt),DefStr(nullptr) {}

	constexpr FARConfig(int save, const char *key, const char *val, FARString *trg, const wchar_t *dflt) :
		IsSave(save),ValType(REG_SZ),KeyName(key),ValName(val),StrPtr(trg),DefDWord(0),DefStr(dflt) {}

} CFG[]=
{
	{1, NSecColors, "CurrentPalette", (BYTE *)Palette8bit, SIZE_ARRAY_PALETTE, (BYTE *)DefaultPalette8bit},
	{1, NSecColors, "CurrentPaletteRGB", (BYTE *)Palette, SIZE_ARRAY_PALETTE * 8, nullptr},
	{1, NSecColors, "TempColors256", g_tempcolors256,         TEMP_COLORS256_SIZE, g_tempcolors256},
	{1, NSecColors, "TempColorsRGB", (BYTE *)g_tempcolorsRGB, TEMP_COLORSRGB_SIZE, (BYTE *)g_tempcolorsRGB},

	{1, NSecScreen, "Clock",                        &Opt.Clock, 1, REG_BOOLEAN},
	{1, NSecScreen, "ViewerEditorClock",            &Opt.ViewerEditorClock, 0, REG_BOOLEAN},
	{1, NSecScreen, "KeyBar",                       &Opt.ShowKeyBar, 1, REG_BOOLEAN},
	{1, NSecScreen, "ScreenSaver",                  &Opt.ScreenSaver, 0, REG_BOOLEAN},
	{1, NSecScreen, "ScreenSaverTime",              &Opt.ScreenSaverTime, 5},
	{1, NSecScreen, "CursorBlinkInterval",          &Opt.CursorBlinkTime, 500},

	{1, NSecCmdline, "UsePromptFormat",             &Opt.CmdLine.UsePromptFormat, 0, REG_BOOLEAN},
	{1, NSecCmdline, "PromptFormat",                &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{1, NSecCmdline, "UseShell",                    &Opt.CmdLine.UseShell, 0, REG_BOOLEAN},
	{1, NSecCmdline, "Shell",                       &Opt.CmdLine.strShell, L"/bin/bash"},
	{1, NSecCmdline, "DelRemovesBlocks",            &Opt.CmdLine.DelRemovesBlocks, 1, REG_BOOLEAN},
	{1, NSecCmdline, "EditBlock",                   &Opt.CmdLine.EditBlock, 0, REG_BOOLEAN},
	{1, NSecCmdline, "AutoComplete",                &Opt.CmdLine.AutoComplete, 1, REG_BOOLEAN},
	{1, NSecCmdline, "Splitter",                    &Opt.CmdLine.Splitter, 1, REG_BOOLEAN},
	{1, NSecCmdline, "WaitKeypress",                &Opt.CmdLine.WaitKeypress, 1},
	{1, NSecCmdline, "VTLogLimit",                  &Opt.CmdLine.VTLogLimit, 5000},

	{1, NSecInterface, "Mouse",                     &Opt.Mouse, 1, REG_BOOLEAN},
	{0, NSecInterface, "UseVk_oem_x",               &Opt.UseVk_oem_x, 1, REG_BOOLEAN},
	{1, NSecInterface, "ShowMenuBar",               &Opt.ShowMenuBar, 0, REG_BOOLEAN},
	{0, NSecInterface, "CursorSize1",               &Opt.CursorSize[0], 15},
	{0, NSecInterface, "CursorSize2",               &Opt.CursorSize[1], 10},
	{0, NSecInterface, "CursorSize3",               &Opt.CursorSize[2], 99},
	{0, NSecInterface, "CursorSize4",               &Opt.CursorSize[3], 99},
	{0, NSecInterface, "ShiftsKeyRules",            &Opt.ShiftsKeyRules, 1, REG_BOOLEAN},
	{1, NSecInterface, "CtrlPgUp",                  &Opt.PgUpChangeDisk, 1, REG_BOOLEAN},

	{1, NSecInterface, "ConsolePaintSharp",         &Opt.ConsolePaintSharp, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveCtrlLeft",         &Opt.ExclusiveCtrlLeft, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveCtrlRight",        &Opt.ExclusiveCtrlRight, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveAltLeft",          &Opt.ExclusiveAltLeft, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveAltRight",         &Opt.ExclusiveAltRight, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveWinLeft",          &Opt.ExclusiveWinLeft, 0, REG_BOOLEAN},
	{1, NSecInterface, "ExclusiveWinRight",         &Opt.ExclusiveWinRight, 0, REG_BOOLEAN},
	{1, NSecInterface, "UseStickyKeyEvent",         &Opt.UseStickyKeyEvent, 0, REG_BOOLEAN},

	{1, NSecInterface, "DateFormat",                &Opt.DateFormat, (DWORD) GetDateFormatDefault()},
	{1, NSecInterface, "DateSeparator",             &Opt.strDateSeparator, GetDateSeparatorDefaultStr()},
	{1, NSecInterface, "TimeSeparator",             &Opt.strTimeSeparator, GetTimeSeparatorDefaultStr()},
	{1, NSecInterface, "DecimalSeparator",          &Opt.strDecimalSeparator, GetDecimalSeparatorDefaultStr()},

	{1, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 0, REG_BOOLEAN},
	{1, NSecInterface, "TTYPaletteOverride",        &Opt.TTYPaletteOverride, 1, REG_BOOLEAN},

	{0, NSecInterface, "ShowTimeoutDelFiles",       &Opt.ShowTimeoutDelFiles, 50},
	{0, NSecInterface, "ShowTimeoutDACLFiles",      &Opt.ShowTimeoutDACLFiles, 50},
	{0, NSecInterface, "FormatNumberSeparators",    &Opt.FormatNumberSeparators, 0},
	{1, NSecInterface, "CopyShowTotal",             &Opt.CMOpt.CopyShowTotal, 1, REG_BOOLEAN},
	{1, NSecInterface, "DelShowTotal",              &Opt.DelOpt.DelShowTotal, 0, REG_BOOLEAN},
	{1, NSecInterface, "WindowTitle",               &Opt.strWindowTitle, L"%State - FAR2M %Ver %Backend %User@%Host"}, // %Platform
	{1, NSecInterfaceCompletion, "Exceptions",      &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
	{1, NSecInterfaceCompletion, "ShowList",        &Opt.AutoComplete.ShowList, 1, REG_BOOLEAN},
	{1, NSecInterfaceCompletion, "ModalList",       &Opt.AutoComplete.ModalList, 0, REG_BOOLEAN},
	{1, NSecInterfaceCompletion, "Append",          &Opt.AutoComplete.AppendCompletion, 0, REG_BOOLEAN},

	{1, NSecViewer, "ExternalViewerName",           &Opt.strExternalViewer, L""},
	{1, NSecViewer, "UseExternalViewer",            &Opt.ViOpt.UseExternalViewer, 0, REG_BOOLEAN},
	{1, NSecViewer, "SaveViewerPos",                &Opt.ViOpt.SavePos, 1, REG_BOOLEAN},
	{1, NSecViewer, "SaveViewerShortPos",           &Opt.ViOpt.SaveShortPos, 1, REG_BOOLEAN},
	{1, NSecViewer, "AutoDetectCodePage",           &Opt.ViOpt.AutoDetectCodePage, 0, REG_BOOLEAN},
	{1, NSecViewer, "SearchRegexp",                 &Opt.ViOpt.SearchRegexp, 0, REG_BOOLEAN},

	{1, NSecViewer, "TabSize",                      &Opt.ViOpt.TabSize, 8},
	{1, NSecViewer, "ShowKeyBar",                   &Opt.ViOpt.ShowKeyBar, 1, REG_BOOLEAN},
	{1, NSecViewer, "ShowTitleBar",                 &Opt.ViOpt.ShowTitleBar, 1, REG_BOOLEAN},
	{1, NSecViewer, "ShowArrows",                   &Opt.ViOpt.ShowArrows, 1, REG_BOOLEAN},
	{1, NSecViewer, "ShowScrollbar",                &Opt.ViOpt.ShowScrollbar, 0, REG_BOOLEAN},
	{1, NSecViewer, "IsWrap",                       &Opt.ViOpt.ViewerIsWrap, 1},
	{1, NSecViewer, "Wrap",                         &Opt.ViOpt.ViewerWrap, 0},
	{1, NSecViewer, "PersistentBlocks",             &Opt.ViOpt.PersistentBlocks, 0, REG_BOOLEAN},
	{1, NSecViewer, "DefaultCodePage",              &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{1, NSecDialog, "EditHistory",                  &Opt.Dialogs.EditHistory, 1, REG_BOOLEAN},
	{1, NSecDialog, "EditBlock",                    &Opt.Dialogs.EditBlock, 0, REG_BOOLEAN},
	{1, NSecDialog, "AutoComplete",                 &Opt.Dialogs.AutoComplete, 1, REG_BOOLEAN},
	{1, NSecDialog, "EULBsClear",                   &Opt.Dialogs.EULBsClear, 0, REG_BOOLEAN},
	{0, NSecDialog, "EditLine",                     &Opt.Dialogs.EditLine, 0},
	{1, NSecDialog, "MouseButton",                  &Opt.Dialogs.MouseButton, 0xFFFF},
	{1, NSecDialog, "DelRemovesBlocks",             &Opt.Dialogs.DelRemovesBlocks, 1, REG_BOOLEAN},
	{0, NSecDialog, "CBoxMaxHeight",                &Opt.Dialogs.CBoxMaxHeight, 8},

	{1, NSecEditor, "ExternalEditorName",           &Opt.strExternalEditor, L""},
	{1, NSecEditor, "UseExternalEditor",            &Opt.EdOpt.UseExternalEditor, 0, REG_BOOLEAN},
	{1, NSecEditor, "ExpandTabs",                   &Opt.EdOpt.ExpandTabs, 0},
	{1, NSecEditor, "TabSize",                      &Opt.EdOpt.TabSize, 8},
	{1, NSecEditor, "PersistentBlocks",             &Opt.EdOpt.PersistentBlocks, 0, REG_BOOLEAN},
	{1, NSecEditor, "DelRemovesBlocks",             &Opt.EdOpt.DelRemovesBlocks, 1, REG_BOOLEAN},
	{1, NSecEditor, "AutoIndent",                   &Opt.EdOpt.AutoIndent, 0, REG_BOOLEAN},
	{1, NSecEditor, "SaveEditorPos",                &Opt.EdOpt.SavePos, 1, REG_BOOLEAN},
	{1, NSecEditor, "SaveEditorShortPos",           &Opt.EdOpt.SaveShortPos, 1, REG_BOOLEAN},
	{1, NSecEditor, "AutoDetectCodePage",           &Opt.EdOpt.AutoDetectCodePage, 0, REG_BOOLEAN},
	{1, NSecEditor, "EditorCursorBeyondEOL",        &Opt.EdOpt.CursorBeyondEOL, 1, REG_BOOLEAN},
	{1, NSecEditor, "ReadOnlyLock",                 &Opt.EdOpt.ReadOnlyLock, 0},
	{0, NSecEditor, "EditorUndoSize",               &Opt.EdOpt.UndoSize, 0},
	{0, NSecEditor, "WordDiv",                      &Opt.strWordDiv, WordDiv0},
	{0, NSecEditor, "BSLikeDel",                    &Opt.EdOpt.BSLikeDel, 1, REG_BOOLEAN},
	{0, NSecEditor, "EditorF7Rules",                &Opt.EdOpt.F7Rules, 1, REG_BOOLEAN},
	{0, NSecEditor, "FileSizeLimit",                &Opt.EdOpt.FileSizeLimitLo, 0},
	{0, NSecEditor, "FileSizeLimitHi",              &Opt.EdOpt.FileSizeLimitHi, 0},
	{0, NSecEditor, "CharCodeBase",                 &Opt.EdOpt.CharCodeBase, 1},
	{0, NSecEditor, "AllowEmptySpaceAfterEof",      &Opt.EdOpt.AllowEmptySpaceAfterEof, 0, REG_BOOLEAN},
	{1, NSecEditor, "DefaultCodePage",              &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{1, NSecEditor, "ShowKeyBar",                   &Opt.EdOpt.ShowKeyBar, 1, REG_BOOLEAN},
	{1, NSecEditor, "ShowTitleBar",                 &Opt.EdOpt.ShowTitleBar, 1, REG_BOOLEAN},
	{1, NSecEditor, "ShowScrollBar",                &Opt.EdOpt.ShowScrollBar, 0, REG_BOOLEAN},
	{1, NSecEditor, "EditOpenedForWrite",           &Opt.EdOpt.EditOpenedForWrite, 1, REG_BOOLEAN},
	{1, NSecEditor, "SearchSelFound",               &Opt.EdOpt.SearchSelFound, 0, REG_BOOLEAN},
	{1, NSecEditor, "SearchRegexp",                 &Opt.EdOpt.SearchRegexp, 0, REG_BOOLEAN},
	{1, NSecEditor, "SearchPickUpWord",             &Opt.EdOpt.SearchPickUpWord, 0, REG_BOOLEAN},
	{1, NSecEditor, "ShowWhiteSpace",               &Opt.EdOpt.ShowWhiteSpace, 0, REG_BOOLEAN},

	{1, NSecNotifications, "OnFileOperation",       &Opt.NotifOpt.OnFileOperation, 1, REG_BOOLEAN},
	{1, NSecNotifications, "OnConsole",             &Opt.NotifOpt.OnConsole, 1, REG_BOOLEAN},
	{1, NSecNotifications, "OnlyIfBackground",      &Opt.NotifOpt.OnlyIfBackground, 1, REG_BOOLEAN},

	{0, NSecXLat, "Flags",                          &Opt.XLat.Flags, XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{1, NSecXLat, "EnableForFastFileFind",          &Opt.XLat.EnableForFastFileFind, 1, REG_BOOLEAN},
	{1, NSecXLat, "EnableForDialogs",               &Opt.XLat.EnableForDialogs, 1, REG_BOOLEAN},
	{1, NSecXLat, "WordDivForXlat",                 &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{1, NSecXLat, "XLat",                           &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{1, NSecSavedHistory, NParamHistoryCount,       &Opt.HistoryCount, 512},
	{1, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{1, NSecSavedViewHistory, NParamHistoryCount,   &Opt.ViewHistoryCount, 512},
	{1, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{1, NSecSystem, "SaveHistory",                  &Opt.SaveHistory, 1, REG_BOOLEAN},
	{1, NSecSystem, "SaveFoldersHistory",           &Opt.SaveFoldersHistory, 1, REG_BOOLEAN},
	{0, NSecSystem, "SavePluginFoldersHistory",     &Opt.SavePluginFoldersHistory, 0, REG_BOOLEAN},
	{1, NSecSystem, "SaveViewHistory",              &Opt.SaveViewHistory, 1, REG_BOOLEAN},
	{1, NSecSystem, "AutoHighlightHistory",         &Opt.AutoHighlightHistory, 0, REG_BOOLEAN},
	{1, NSecSystem, "AutoSaveSetup",                &Opt.AutoSaveSetup, 0, REG_BOOLEAN},
	{1, NSecSystem, "DeleteToRecycleBin",           &Opt.DeleteToRecycleBin, 0, REG_BOOLEAN},
	{1, NSecSystem, "DeleteToRecycleBinKillLink",   &Opt.DeleteToRecycleBinKillLink, 1, REG_BOOLEAN},
	{0, NSecSystem, "WipeSymbol",                   &Opt.WipeSymbol, 0},
	{1, NSecSystem, "SudoEnabled",                  &Opt.SudoEnabled, 1, REG_BOOLEAN},
	{1, NSecSystem, "SudoConfirmModify",            &Opt.SudoConfirmModify, 1, REG_BOOLEAN},
	{1, NSecSystem, "SudoPasswordExpiration",       &Opt.SudoPasswordExpiration, 15*60},

	{1, NSecSystem, "UseCOW",                       &Opt.CMOpt.UseCOW, 0, REG_BOOLEAN},
	{1, NSecSystem, "SparseFiles",                  &Opt.CMOpt.SparseFiles, 0, REG_BOOLEAN},
	{1, NSecSystem, "HowCopySymlink",               &Opt.CMOpt.HowCopySymlink, 1},
	{1, NSecSystem, "WriteThrough",                 &Opt.CMOpt.WriteThrough, 0, REG_BOOLEAN},
	{1, NSecSystem, "CopyXAttr",                    &Opt.CMOpt.CopyXAttr, 0, REG_BOOLEAN},
	{0, NSecSystem, "CopyAccessMode",               &Opt.CMOpt.CopyAccessMode, 1, REG_BOOLEAN},
	{1, NSecSystem, "MultiCopy",                    &Opt.CMOpt.MultiCopy, 0, REG_BOOLEAN},
	{1, NSecSystem, "CopyTimeRule",                 &Opt.CMOpt.CopyTimeRule, 3},

	{1, NSecSystem, "InactivityExit",               &Opt.InactivityExit, 0, REG_BOOLEAN},
	{1, NSecSystem, "InactivityExitTime",           &Opt.InactivityExitTime, 15},
	{1, NSecSystem, "DriveMenuMode2",               &Opt.ChangeDriveMode, (DWORD)-1},
	{1, NSecSystem, "DriveDisconnectMode",          &Opt.ChangeDriveDisconnectMode, 1},

	{1, NSecSystem, "DriveExceptions",              &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;"
		"/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron"},
	{1, NSecSystem, "DriveColumn2",                 &Opt.ChangeDriveColumn2, L"$U/$T"},
	{1, NSecSystem, "DriveColumn3",                 &Opt.ChangeDriveColumn3, L"$S$D"},

	{1, NSecSystem, "AutoUpdateRemoteDrive",        &Opt.AutoUpdateRemoteDrive, 1, REG_BOOLEAN},
	{1, NSecSystem, "FileSearchMode",               &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{0, NSecSystem, "CollectFiles",                 &Opt.FindOpt.CollectFiles, 1, REG_BOOLEAN},
	{1, NSecSystem, "SearchInFirstSize",            &Opt.FindOpt.strSearchInFirstSize, L""},
	{1, NSecSystem, "FindAlternateStreams",         &Opt.FindOpt.FindAlternateStreams, 0, REG_BOOLEAN},
	{1, NSecSystem, "SearchOutFormat",              &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{1, NSecSystem, "SearchOutFormatWidth",         &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{1, NSecSystem, "FindFolders",                  &Opt.FindOpt.FindFolders, 1, REG_BOOLEAN},
	{1, NSecSystem, "FindSymLinks",                 &Opt.FindOpt.FindSymLinks, 1, REG_BOOLEAN},
	{1, NSecSystem, "UseFilterInSearch",            &Opt.FindOpt.UseFilter, 0, REG_BOOLEAN},
	{1, NSecSystem, "FindCodePage",                 &Opt.FindCodePage, CP_AUTODETECT},
	{0, NSecSystem, "CmdHistoryRule",               &Opt.CmdHistoryRule, 0, REG_BOOLEAN},
	{0, NSecSystem, "SetAttrFolderRules",           &Opt.SetAttrFolderRules, 1, REG_BOOLEAN},
	{0, NSecSystem, "MaxPositionCache",             &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{0, NSecSystem, "ConsoleDetachKey",             &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{0, NSecSystem, "SilentLoadPlugin",             &Opt.LoadPlug.SilentLoadPlugin, 0, REG_BOOLEAN},
	{1, NSecSystem, "ScanSymlinks",                 &Opt.LoadPlug.ScanSymlinks, 1, REG_BOOLEAN},
	{1, NSecSystem, "MultiMakeDir",                 &Opt.MultiMakeDir, 0, REG_BOOLEAN},
	{0, NSecSystem, "MsWheelDelta",                 &Opt.MsWheelDelta, 1},
	{0, NSecSystem, "MsWheelDeltaView",             &Opt.MsWheelDeltaView, 1},
	{0, NSecSystem, "MsWheelDeltaEdit",             &Opt.MsWheelDeltaEdit, 1},
	{0, NSecSystem, "MsWheelDeltaHelp",             &Opt.MsWheelDeltaHelp, 1},
	{0, NSecSystem, "MsHWheelDelta",                &Opt.MsHWheelDelta, 1},
	{0, NSecSystem, "MsHWheelDeltaView",            &Opt.MsHWheelDeltaView, 1},
	{0, NSecSystem, "MsHWheelDeltaEdit",            &Opt.MsHWheelDeltaEdit, 1},
	{0, NSecSystem, "SubstNameRule",                &Opt.SubstNameRule, 2},
	{0, NSecSystem, "ShowCheckingFile",             &Opt.ShowCheckingFile, 0, REG_BOOLEAN},
	{0, NSecSystem, "DelThreadPriority",            &Opt.DelThreadPriority, 0},
	{0, NSecSystem, "QuotedSymbols",                &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{0, NSecSystem, "QuotedName",                   &Opt.QuotedName, QUOTEDNAME_INSERT},
	{0, NSecSystem, "PluginMaxReadData",            &Opt.PluginMaxReadData, 0x40000},
	{0, NSecSystem, "CASRule",                      &Opt.CASRule, 0xFFFFFFFFU},
	{0, NSecSystem, "AllCtrlAltShiftRule",          &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{1, NSecSystem, "ScanJunction",                 &Opt.ScanJunction, 1, REG_BOOLEAN},
	{1, NSecSystem, "OnlyFilesSize",                &Opt.OnlyFilesSize, 0, REG_BOOLEAN},
	{0, NSecSystem, "UsePrintManager",              &Opt.UsePrintManager, 1, REG_BOOLEAN},
	{0, NSecSystem, "WindowMode",                   &Opt.WindowMode, 0, REG_BOOLEAN},
	{1, NSecSystem, "FastSynchroEvents",            &Opt.FastSynchroEvents, 1, REG_BOOLEAN},

	{0, NSecPanelTree, "MinTreeCount",              &Opt.Tree.MinTreeCount, 4},
	{0, NSecPanelTree, "TreeFileAttr",              &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{0, NSecPanelTree, "LocalDisk",                 &Opt.Tree.LocalDisk, 2},
	{0, NSecPanelTree, "NetDisk",                   &Opt.Tree.NetDisk, 2},
	{0, NSecPanelTree, "RemovableDisk",             &Opt.Tree.RemovableDisk, 2},
	{0, NSecPanelTree, "NetPath",                   &Opt.Tree.NetPath, 2},
	{1, NSecPanelTree, "AutoChangeFolder",          &Opt.Tree.AutoChangeFolder, 0, REG_BOOLEAN}, // ???

	{0, NSecHelp, "ActivateURL",                    &Opt.HelpURLRules, 1},

	{1, NSecLanguage, "Help",                       &Opt.strHelpLanguage, L"English"},
	{1, NSecLanguage, "Main",                       &Opt.strLanguage, L"English"},

	{1, NSecConfirmations, "Copy",                  &Opt.Confirm.Copy, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "Move",                  &Opt.Confirm.Move, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "RO",                    &Opt.Confirm.RO, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "Drag",                  &Opt.Confirm.Drag, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "Delete",                &Opt.Confirm.Delete, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "DeleteFolder",          &Opt.Confirm.DeleteFolder, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "Esc",                   &Opt.Confirm.Esc, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "RemoveConnection",      &Opt.Confirm.RemoveConnection, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "RemoveHotPlug",         &Opt.Confirm.RemoveHotPlug, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "AllowReedit",           &Opt.Confirm.AllowReedit, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "HistoryClear",          &Opt.Confirm.HistoryClear, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "Exit",                  &Opt.Confirm.Exit, 1, REG_BOOLEAN},
	{1, NSecConfirmations, "ExitOrBknd",            &Opt.Confirm.ExitOrBknd, 1, REG_BOOLEAN},
	{0, NSecConfirmations, "EscTwiceToInterrupt",   &Opt.Confirm.EscTwiceToInterrupt, 0, REG_BOOLEAN},

	{1, NSecPluginConfirmations, "OpenFilePlugin",  &Opt.PluginConfirm.OpenFilePlugin, 0, REG_3STATE},
	{1, NSecPluginConfirmations, "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, REG_BOOLEAN},
	{1, NSecPluginConfirmations, "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, REG_BOOLEAN},
	{1, NSecPluginConfirmations, "SetFindList",     &Opt.PluginConfirm.SetFindList, 0, REG_BOOLEAN},
	{1, NSecPluginConfirmations, "Prefix",          &Opt.PluginConfirm.Prefix, 0, REG_BOOLEAN},

	{0, NSecPanel, "ShellRightLeftArrowsRule",      &Opt.ShellRightLeftArrowsRule, 0, REG_BOOLEAN},
	{1, NSecPanel, "ShowHidden",                    &Opt.ShowHidden, 1, REG_BOOLEAN},
	{1, NSecPanel, "Highlight",                     &Opt.Highlight, 1, REG_BOOLEAN},
	{1, NSecPanel, "SortFolderExt",                 &Opt.SortFolderExt, 0, REG_BOOLEAN},
	{1, NSecPanel, "SelectFolders",                 &Opt.SelectFolders, 0, REG_BOOLEAN},
	{1, NSecPanel, "CaseSensitiveCompareSelect",    &Opt.PanelCaseSensitiveCompareSelect, 0, REG_BOOLEAN},
	{1, NSecPanel, "ReverseSort",                   &Opt.ReverseSort, 1, REG_BOOLEAN},
	{0, NSecPanel, "RightClickRule",                &Opt.PanelRightClickRule, 2, REG_3STATE},
	{0, NSecPanel, "CtrlFRule",                     &Opt.PanelCtrlFRule, 1, REG_BOOLEAN},
	{0, NSecPanel, "CtrlAltShiftRule",              &Opt.PanelCtrlAltShiftRule, 0, REG_3STATE},
	{0, NSecPanel, "RememberLogicalDrives",         &Opt.RememberLogicalDrives, 0},
	{1, NSecPanel, "AutoUpdateLimit",               &Opt.AutoUpdateLimit, 0},
	{1, NSecPanel, "ShowFilenameMarks",             &Opt.ShowFilenameMarks, 1, REG_BOOLEAN},
	{1, NSecPanel, "FilenameMarksAlign",            &Opt.FilenameMarksAlign, 1, REG_BOOLEAN},
	{1, NSecPanel, "MinFilenameIndentation",        &Opt.MinFilenameIndentation, 0},
	{1, NSecPanel, "MaxFilenameIndentation",        &Opt.MaxFilenameIndentation, HIGHLIGHT_MAX_MARK_LENGTH},

	{1, NSecPanelLeft, "Type",                      &Opt.LeftPanel.Type, 0},
	{1, NSecPanelLeft, "Visible",                   &Opt.LeftPanel.Visible, 1, REG_BOOLEAN},
	{1, NSecPanelLeft, "Focus",                     &Opt.LeftPanel.Focus, 1, REG_BOOLEAN},
	{1, NSecPanelLeft, "ViewMode",                  &Opt.LeftPanel.ViewMode, 2},
	{1, NSecPanelLeft, "SortMode",                  &Opt.LeftPanel.SortMode, 1},
	{1, NSecPanelLeft, "SortOrder",                 &Opt.LeftPanel.SortOrder, 1},
	{1, NSecPanelLeft, "SortGroups",                &Opt.LeftPanel.SortGroups, 0, REG_BOOLEAN},
	{1, NSecPanelLeft, "NumericSort",               &Opt.LeftPanel.NumericSort, 0, REG_BOOLEAN},
	{1, NSecPanelLeft, "CaseSensitiveSort",         &Opt.LeftPanel.CaseSensitiveSort, 0, REG_BOOLEAN},
	{1, NSecPanelLeft, "Folder",                    &Opt.strLeftFolder, L""},
	{1, NSecPanelLeft, "CurFile",                   &Opt.strLeftCurFile, L""},
	{1, NSecPanelLeft, "SelectedFirst",             &Opt.LeftSelectedFirst, 0, REG_BOOLEAN},
	{1, NSecPanelLeft, "DirectoriesFirst",          &Opt.LeftPanel.DirectoriesFirst, 1, REG_BOOLEAN},

	{1, NSecPanelRight, "Type",                     &Opt.RightPanel.Type, 0},
	{1, NSecPanelRight, "Visible",                  &Opt.RightPanel.Visible, 1, REG_BOOLEAN},
	{1, NSecPanelRight, "Focus",                    &Opt.RightPanel.Focus, 0, REG_BOOLEAN},
	{1, NSecPanelRight, "ViewMode",                 &Opt.RightPanel.ViewMode, 2},
	{1, NSecPanelRight, "SortMode",                 &Opt.RightPanel.SortMode, 1},
	{1, NSecPanelRight, "SortOrder",                &Opt.RightPanel.SortOrder, 1},
	{1, NSecPanelRight, "SortGroups",               &Opt.RightPanel.SortGroups, 0, REG_BOOLEAN},
	{1, NSecPanelRight, "NumericSort",              &Opt.RightPanel.NumericSort, 0, REG_BOOLEAN},
	{1, NSecPanelRight, "CaseSensitiveSort",        &Opt.RightPanel.CaseSensitiveSort, 0, REG_BOOLEAN},
	{1, NSecPanelRight, "Folder",                   &Opt.strRightFolder, L""},
	{1, NSecPanelRight, "CurFile",                  &Opt.strRightCurFile, L""},
	{1, NSecPanelRight, "SelectedFirst",            &Opt.RightSelectedFirst, 0, REG_BOOLEAN},
	{1, NSecPanelRight, "DirectoriesFirst",         &Opt.RightPanel.DirectoriesFirst, 1, REG_BOOLEAN},

	{1, NSecPanelLayout, "ColumnTitles",            &Opt.ShowColumnTitles, 1, REG_BOOLEAN},
	{1, NSecPanelLayout, "StatusLine",              &Opt.ShowPanelStatus, 1, REG_BOOLEAN},
	{1, NSecPanelLayout, "TotalInfo",               &Opt.ShowPanelTotals, 1, REG_BOOLEAN},
	{1, NSecPanelLayout, "FreeInfo",                &Opt.ShowPanelFree, 0, REG_BOOLEAN},
	{1, NSecPanelLayout, "Scrollbar",               &Opt.ShowPanelScrollbar, 0, REG_BOOLEAN},
	{0, NSecPanelLayout, "ScrollbarMenu",           &Opt.ShowMenuScrollbar, 1, REG_BOOLEAN},
	{1, NSecPanelLayout, "ScreensNumber",           &Opt.ShowScreensNumber, 1, REG_BOOLEAN},
	{1, NSecPanelLayout, "SortMode",                &Opt.ShowSortMode, 1, REG_BOOLEAN},

	{1, NSecLayout, "LeftHeightDecrement",          &Opt.LeftHeightDecrement, 0},
	{1, NSecLayout, "RightHeightDecrement",         &Opt.RightHeightDecrement, 0},
	{1, NSecLayout, "WidthDecrement",               &Opt.WidthDecrement, 0},
	{1, NSecLayout, "FullscreenHelp",               &Opt.FullScreenHelp, 0, REG_BOOLEAN},

	{1, NSecDescriptions, "ListNames",              &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{1, NSecDescriptions, "UpdateMode",             &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{1, NSecDescriptions, "ROUpdate",               &Opt.Diz.ROUpdate, 0, REG_BOOLEAN},
	{1, NSecDescriptions, "SetHidden",              &Opt.Diz.SetHidden, 1, REG_BOOLEAN},
	{1, NSecDescriptions, "StartPos",               &Opt.Diz.StartPos, 0},
	{1, NSecDescriptions, "AnsiByDefault",          &Opt.Diz.AnsiByDefault, 0, REG_BOOLEAN},
	{1, NSecDescriptions, "SaveInUTF",              &Opt.Diz.SaveInUTF, 0, REG_BOOLEAN},

	{0, NSecMacros, "DateFormat",                   &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{1, NSecMacros, "ShowPlayIndicator",            &Opt.Macro.ShowPlayIndicator, 1, REG_BOOLEAN},
	{1, NSecMacros, "KeyRecordCtrlDot",             &Opt.Macro.strKeyMacroCtrlDot, szCtrlDot},
	{1, NSecMacros, "KeyRecordCtrlShiftDot",        &Opt.Macro.strKeyMacroCtrlShiftDot, szCtrlShiftDot},

	{0, NSecPolicies, "ShowHiddenDrives",           &Opt.Policies.ShowHiddenDrives, 1, REG_BOOLEAN},
	{0, NSecPolicies, "DisabledOptions",            &Opt.Policies.DisabledOptions, 0},


	{0, NSecSystem, "ExcludeCmdHistory",            &Opt.ExcludeCmdHistory, 0}, //AN

	{1, NSecCodePages, "CPMenuMode",                &Opt.CPMenuMode, 0, REG_BOOLEAN},

	{1, NSecSystem, "FolderInfo",                   &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{1, NSecVMenu, "MenuLoopScroll",                &Opt.VMenu.MenuLoopScroll, 0, REG_BOOLEAN},

	{1, NSecVMenu, "LBtnClick",                     &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{1, NSecVMenu, "RBtnClick",                     &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{1, NSecVMenu, "MBtnClick",                     &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
};

int ConfigOptGetIndex(const wchar_t *KeyName)
{
	auto Dot = wcsrchr(KeyName, L'.');
	if (Dot)
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
		Data.Type = CFG[I].ValType;
		Data.Key = CFG[I].KeyName;
		Data.Name = CFG[I].ValName;
		switch (CFG[I].ValType)
		{
			default:
				Data.dwDefault = CFG[I].DefDWord;
				Data.dwValue = *(unsigned int *)CFG[I].ValPtr;
				break;
			case REG_SZ:
				Data.strDefault = CFG[I].DefStr;
				Data.strValue = *CFG[I].StrPtr;
				break;
			case REG_BINARY:
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
			case REG_DWORD:
			case REG_BOOLEAN:
			case REG_3STATE:
				*(unsigned int *)CFG[I].ValPtr = Value;
				return true;
		}
	}
	return false;
}

bool ConfigOptSetString(int I, const wchar_t *Value)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG) && CFG[I].ValType == REG_SZ && Value)
	{
		*CFG[I].StrPtr = Value;
		return true;
	}
	return false;
}

bool ConfigOptSetBinary(int I, const void *Data, DWORD Size)
{
	if (I >= 0 && I < (int)ARRAYSIZE(CFG) && CFG[I].ValType == REG_BINARY && Data)
	{
		Size = std::min(Size, CFG[I].DefDWord);
		memcpy(CFG[I].ValPtr, Data, Size);
		return true;
	}
	return false;
}

static void MergePalette()
{
	for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {

		Palette[i] &= 0xFFFFFFFFFFFFFF00;
		Palette[i] |= Palette8bit[i];
	}

//	uint32_t basepalette[32];
//	WINPORT(GetConsoleBasePalette)(NULL, basepalette);

/*
	for(size_t i = 0; i < SIZE_ARRAY_PALETTE; i++) {
		uint8_t color = Palette8bit[i];

		Palette[i] &= 0xFFFFFFFFFFFFFF00;

		if (!(Palette[i] & FOREGROUND_TRUECOLOR)) {
			Palette[i] &= 0xFFFFFF000000FFFF;
			Palette[i] += ((uint64_t)basepalette[16 + (color & 0xF)] << 16);
			Palette[i] += FOREGROUND_TRUECOLOR;
		}
		if (!(Palette[i] & BACKGROUND_TRUECOLOR)) {
			Palette[i] &= 0x000000FFFFFFFFFF;
			Palette[i] += ((uint64_t)basepalette[color >> 4] << 40);
			Palette[i] += BACKGROUND_TRUECOLOR;
		}

		Palette[i] += color;
	}
*/
}

static void ConfigOptFromCmdLine()
{
	for (auto Str: Opt.CmdLineStrings)
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
				switch (cfg.Type)
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

					case REG_SZ:
						ConfigOptSetString(Index, pVal);
						break;

					case REG_BINARY:
						break; // not supported
				}
			}
		}
	}
	Opt.CmdLineStrings.clear();
}

void ConfigOptLoad()
{
	FARString strKeyNameFromReg;
	FARString strPersonalPluginsPath;
	size_t I;

	ConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	cfg_reader.SelectSection(NSecSystem);
	Opt.LoadPlug.strPersonalPluginsPath = cfg_reader.GetString("PersonalPluginsPath", L"");
	bool ExplicitWindowMode=Opt.WindowMode!=FALSE;
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	for (I=0; I < ARRAYSIZE(CFG); ++I)
	{
		cfg_reader.SelectSection(CFG[I].KeyName);
		switch (CFG[I].ValType)
		{
			case REG_DWORD:
			case REG_BOOLEAN:
			case REG_3STATE:
				*(unsigned int *)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, (unsigned int)CFG[I].DefDWord);
				break;
			case REG_SZ:
				*CFG[I].StrPtr = cfg_reader.GetString(CFG[I].ValName, CFG[I].DefStr);
				break;
			case REG_BINARY:
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
		Opt.PluginMaxReadData=0x20000;

	if(ExplicitWindowMode)
	{
		Opt.WindowMode=TRUE;
	}

	Opt.HelpTabSize=8; // пока жестко пропишем...
//	SanitizePalette();
	MergePalette();

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
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

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
	cfg_writer.SelectSection(NSecSystem);
	cfg_writer.SetString("PersonalPluginsPath", Opt.LoadPlug.strPersonalPluginsPath);
//	cfg_writer.SetString(NSecLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].IsSave)
		{
			cfg_writer.SelectSection(CFG[I].KeyName);
			switch (CFG[I].ValType)
			{
				case REG_DWORD:
				case REG_BOOLEAN:
				case REG_3STATE:
					cfg_writer.SetUInt(CFG[I].ValName, *(unsigned int *)CFG[I].ValPtr);
					break;
				case REG_SZ:
					cfg_writer.SetString(CFG[I].ValName, CFG[I].StrPtr->CPtr());
					break;
				case REG_BINARY:
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

	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}
