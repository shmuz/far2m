/*
ffolders.cpp

Folder shortcuts
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


#include "Bookmarks.hpp"
#include "keys.hpp"
#include "lang.hpp"
#include "vmenu.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "filelist.hpp"
#include "KeyFileHelper.h"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "dialog.hpp"
#include "DialogBuilder.hpp"
#include <farplug-wide.h>
#include "plugins.hpp"
#include "DlgGuid.hpp"

static const wchar_t HelpBookmarks[] = L"Bookmarks";

bool Bookmarks::EditItem(int SelPos)
{
	BookmarkData Data;
	Get(SelPos, Data);

	DialogBuilder Builder(Msg::BookmarksTitle, HelpBookmarks);
	Builder.SetId(FolderShortcutsDlgId);
	Builder.AddText(Msg::FSShortcutName);
	Builder.AddEditField(&Data.Name, 50, L"FS_Name", 0);
	Builder.AddText(Msg::FSShortcutPath);
	Builder.AddEditField(&Data.Folder, 50, L"FS_Path", DIF_EDITPATH);
	if (Data.PluginId != SYSID_FAR)
	{
		Plugin *pPlugin = CtrlObject->Plugins.FindPlugin(Data.PluginId);
		FARString Text;
		if (pPlugin) {
			Text.Format(L" %ls ", pPlugin->GetTitle());
		}	else {
			Text.Format(L" Plugin: 0x%08X ", Data.PluginId);
		}
		Builder.AddSeparator(Text);
		Builder.AddText(Msg::FSShortcutPluginFile);
		Builder.AddEditField(&Data.PluginFile, 50, L"FS_PluginFile", DIF_EDITPATH);
		Builder.AddText(Msg::FSShortcutPluginData);
		Builder.AddEditField(&Data.PluginData, 50, L"FS_PluginData", 0);
	}
	//...
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		bool Saved = true;

		if (Data.PluginId == SYSID_FAR)
		{
			auto strNewFolder = Data.Folder;
			Unquote(strNewFolder);

			if (!IsLocalRootPath(strNewFolder))
				DeleteEndSlash(strNewFolder);

			auto strTemp = Data.Folder;
			apiExpandEnvironmentStrings(strNewFolder,strTemp);

			if (apiGetFileAttributes(strTemp) == INVALID_FILE_ATTRIBUTES)
			{
				WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);
				Saved = !Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::Error, strNewFolder,
						Msg::SaveThisShortcut, Msg::Yes, Msg::No);
			}
		}

		if (Saved)
		{
			Set(SelPos, Data);
			return true;
		}
	}

	return false;
}

static FARString MakeName(const BookmarkData &Item)
{
	if (!Item.Name.IsEmpty())
		return Item.Name;

	if (Item.PluginId == SYSID_FAR)
		return Item.Folder;

	FormatString Text;
	const auto plugin = CtrlObject->Plugins.FindPlugin(Item.PluginId);

	if (plugin)
		Text << plugin->GetTitle();
	else
		Text << FARString().Format(L"0x%08X", Item.PluginId);

	Text << L" : " << Item.PluginFile
	     << L" : " << Item.Folder
	     << L" : " << Item.PluginData;

	return Text.strValue();
}

static int ShowBookmarksMenuIteration(int Pos)
{
	int ExitCode=-1;
	Bookmarks b;
	{
		MenuItemEx ListItem;
		VMenu FolderList(Msg::BookmarksTitle,nullptr,0,ScrY-4);
		FolderList.SetFlags(VMENU_WRAPMODE); // VMENU_SHOWAMPERSAND|
		FolderList.SetHelp(HelpBookmarks);
		FolderList.SetPosition(-1,-1,0,0);
		FolderList.SetId(FolderShortcutsId);
		FolderList.SetBottomTitle(Msg::BookmarkBottom);

		for (int I=0; ; I++)
		{
			BookmarkData Data;
			ListItem.Clear();
			b.Get(I, Data);

			bool EmptyShortcut = false;
			FARString Text = MakeName(Data);
			if (Text.IsEmpty())
			{
				EmptyShortcut = true;
				Text = Msg::ShortcutNone;
			}

			if (I < 10)
			{
				ListItem.strName.Format(L"[%ls | Ctrl+Alt] + &%d   %ls",
						Msg::RightCtrl.CPtr(), I, Text.CPtr());
			}
			else
			{
				ListItem.strName.Format(L"%ls", Text.CPtr());
			}
			ListItem.SetSelect(I == Pos);
			FolderList.AddItem(&ListItem);

			if (I >= 10 && EmptyShortcut)
				break;
		}

		FolderList.Show();

		while (!FolderList.Done())
		{
			FarKey Key=FolderList.ReadInput();
			int SelPos=FolderList.GetSelectPos();

			switch (Key)
			{
				case KEY_SHIFTUP:
					if (SelPos == 0) {
						return SelPos;
					}
					[[fallthrough]];

				case KEY_SHIFTDOWN:
				{
					BookmarkData Data;
					if (!b.Get(SelPos, Data) )
						return SelPos;

					const int OtherPos = (Key == KEY_SHIFTUP) ? SelPos - 1 : SelPos + 1;
					BookmarkData OtherData;
					b.Get(OtherPos, OtherData);
					b.Set(OtherPos, Data);
					b.Set(SelPos, OtherData);

					return(OtherPos);
				}


				case KEY_NUMDEL:
				case KEY_DEL:
					b.Clear(SelPos);
					return(SelPos);

				case KEY_NUMPAD0:
				case KEY_INS:
				{
					Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
					BookmarkData Data;
					Data.Folder = CtrlObject->CmdLine->GetCurDir();

					if (ActivePanel->GetMode() == PLUGIN_PANEL)
					{
						OpenPluginInfo Info;
						ActivePanel->GetOpenPluginInfo(&Info);
						PanelHandle *ph = ActivePanel->GetPluginHandle();
						Data.PluginId = ph->pPlugin->GetSysID();
						Data.PluginFile = Info.HostFile;
						Data.PluginData = Info.ShortcutData;
					}

					b.Set(SelPos, Data);
					return SelPos;
				}

				case KEY_F4:
					if (b.EditItem(SelPos))
						return SelPos;
					else
						break;

				default:
					FolderList.ProcessInput();
					break;
			}
		}

		ExitCode=FolderList.Modal::GetExitCode();
		FolderList.Hide();
	}

	if (ExitCode>=0)
	{
		CtrlObject->Cp()->ActivePanel->ExecShortcutFolder(ExitCode);
	}

	return -1;
}

void ShowBookmarksMenu(int Pos)
{
	while (Pos != -1)
	{
		Pos = ShowBookmarksMenuIteration(Pos);
	}
}

