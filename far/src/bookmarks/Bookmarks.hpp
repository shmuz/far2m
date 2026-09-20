#pragma once
#include <KeyFileHelper.h>
#include "FARString.hpp"

struct BookmarkData
{
	FARString Name;
	FARString Folder;
	FARString PluginFile;
	FARString PluginData;
	uint32_t  PluginId = 0;
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
