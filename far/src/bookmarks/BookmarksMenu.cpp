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
			//TruncStr(Data.ShortcutFolder,60);

			if (Data.ShortcutFolder.IsEmpty())
			{
				Data.ShortcutFolder = Data.PluginModule.IsEmpty()
					? Msg::ShortcutNone : Msg::ShortcutPlugin;
			}

//wxWidgets doesn't distinguish right/left modifiers
//			ListItem.strName.Format(L"%ls+&%d   %ls", Msg::RightCtrl.CPtr(), I ,strFolderName.CPtr());
			if (I < 10)
			{
				ListItem.strName.Format(L"[%ls | Ctrl+Alt] + &%d   %ls",
						Msg::RightCtrl.CPtr(), I, Data.ShortcutFolder.CPtr());
			}
			else
			{
				ListItem.strName.Format(L"%ls", Data.ShortcutFolder.CPtr());
			}
			ListItem.SetSelect(I == Pos);
			FolderList.AddItem(&ListItem);

			if (I >= 10 && Data.ShortcutFolder == Msg::ShortcutNone)
			{
				break;
			}
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
					BookmarkData NewData;
					NewData.ShortcutFolder = CtrlObject->CmdLine->GetCurDir();

					if (ActivePanel->GetMode() == PLUGIN_PANEL)
					{
						OpenPluginInfo Info;
						ActivePanel->GetOpenPluginInfo(&Info);
						PanelHandle *ph = ActivePanel->GetPluginHandle();
						NewData.PluginModule = ph->pPlugin->GetModuleName();
						NewData.PluginFile = Info.HostFile;
						NewData.PluginData = Info.ShortcutData;
					}

					b.Set(SelPos, NewData);
					return SelPos;
				}
				case KEY_F4:
				{
					BookmarkData Data;
					b.Get(SelPos, Data);
					FARString strNewDir = Data.ShortcutFolder;
					FARString strTemp = strNewDir;

					DialogBuilder Builder(Msg::BookmarksTitle, HelpBookmarks);
					Builder.SetId(FolderShortcutsDlgId);
					Builder.AddText(Msg::FSShortcut);
					Builder.AddEditField(&strNewDir, 50, L"FS_Path", DIF_EDITPATH);
					//...
					Builder.AddOKCancel();

					if (Builder.ShowDialog())
					{
						Unquote(strNewDir);

						if (!IsLocalRootPath(strNewDir))
							DeleteEndSlash(strNewDir);

						bool Saved = true;
						apiExpandEnvironmentStrings(strNewDir,strTemp);

						if (apiGetFileAttributes(strTemp) == INVALID_FILE_ATTRIBUTES)
						{
							WINPORT(SetLastError)(ERROR_PATH_NOT_FOUND);
							Saved = !Message(MSG_WARNING | MSG_ERRORTYPE, 2, Msg::Error, strNewDir,
									Msg::SaveThisShortcut, Msg::Yes, Msg::No);
						}

						if (Saved)
						{
							BookmarkData NewData { strNewDir };
							b.Set(SelPos, NewData);
							return SelPos;
						}
					}

					break;
				}
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

