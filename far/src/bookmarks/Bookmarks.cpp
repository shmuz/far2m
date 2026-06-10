/*
Bookrmarks.cpp

Folder shortcuts
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

Bookmarks::Bookmarks()
	: _kfh(InMyConfig("settings/bookmarks.ini").c_str(), true)
{
}

bool Bookmarks::Set(int index, const BookmarkData &Data)
{
	if (Data.ShortcutFolder.IsEmpty() && Data.PluginModule.IsEmpty())
	{
		return Clear(index);
	}

	const auto &sec = ToDec(index);
	_kfh.RemoveSection(sec);

	_kfh.SetString(sec, "Path",       Data.ShortcutFolder.GetMB().c_str());
	_kfh.SetString(sec, "Plugin",     Data.PluginModule.GetMB().c_str());
	_kfh.SetString(sec, "PluginFile", Data.PluginFile.GetMB().c_str());
	_kfh.SetString(sec, "PluginData", Data.PluginData.GetMB().c_str());

	return _kfh.Save();
}

bool Bookmarks::Get(int index, BookmarkData &Data)
{
	const auto &sec = ToDec(index);
	FARString strFolder(_kfh.GetString(sec, "Path"));

	if (!strFolder.IsEmpty())
		apiExpandEnvironmentStrings(strFolder, Data.ShortcutFolder);
	else
		Data.ShortcutFolder.Clear();

	Data.PluginModule = _kfh.GetString(sec, "Plugin");
	Data.PluginFile   = _kfh.GetString(sec, "PluginFile");
	Data.PluginData   = _kfh.GetString(sec, "PluginData");

	return !Data.ShortcutFolder.IsEmpty() || !Data.PluginModule.IsEmpty();
}

bool Bookmarks::Clear(int index)
{
	_kfh.RemoveSection(ToDec(index));
	if (index < 10)
		return _kfh.Save();

	for (int dst_index = index, miss_counter = 0; ; ++index)
	{
		BookmarkData Data;
		if (Get(index, Data))
		{
			if (dst_index != index)
			{
				Set(dst_index, Data);
			}
			++dst_index;
			miss_counter = 0;
		}
		else if (++miss_counter >= 10)
		{
			for (; dst_index <= index; ++dst_index)
			{
				_kfh.RemoveSection(ToDec(dst_index));
			}
			break;
		}
	}

	return _kfh.Save();
}
