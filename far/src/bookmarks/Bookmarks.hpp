#pragma once
#include <KeyFileHelper.h>
#include "FARString.hpp"

struct BookmarkData
{
	FARString ShortcutFolder;
	FARString PluginModule;
	FARString PluginFile;
	FARString PluginData;
};

class Bookmarks
{
	KeyFileHelper _kfh;

public:
	Bookmarks();

	bool Set(int index, const BookmarkData &Data);
	bool Get(int index, BookmarkData &Data);
	bool Clear(int index);
};

void ShowBookmarksMenu(int Pos = 0);
