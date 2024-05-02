/*
MaskGroups.cpp

Groups of file masks
*/

#include "headers.hpp"

#include "lang.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "CFileMask.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "fnparce.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "ConfigRW.hpp"
#include "DlgGuid.hpp"
#include "DialogBuilder.hpp"
#include "MaskGroups.hpp"

/*
MMaskGroupTitle
MMaskGroupName
MMaskGroupMasks
MMaskGroupAskDelete
MMaskGroupRestore
MMaskGroupFindMask
MMaskGroupTotal
========================================

MaskGroupTitle
"Группы масок файлов"
"Groups of file masks"

MaskGroupName
"&Имя:"
"&Name:"

MaskGroupMasks
"Одна или несколько &масок файлов:"
"A file &mask or several file masks:"

MaskGroupAskDelete
"Вы хотите удалить"
"Do you wish to delete"

MaskGroupRestore
"Вы хотите восстановить наборы масок по умолчанию?"
"Do you wish to restore default mask sets?"
*/

struct FileTypeStrings
{
		const char
	*Help,
	*MaskGroups,
	*TypeFmt,
	*Type0,
	*MaskName,
	*MaskValue;
};

static const FileTypeStrings FTS=
{
	"MaskGroupsSettings",
	"MaskGroups",
	"MaskGroups/Type%d",
	"MaskGroups/Type",
	"Name",
	"Value",
};

static int FillFileTypesMenu(VMenu *TypesMenu,int MenuPos)
{
	ConfigReader cfg_reader;
	int DizWidth=10;
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine=0;

	for (;; NumLine++)
	{
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FTS.MaskValue, L""))
			break;

		TypesMenuItem.Clear();

		FARString strMenuText;

		if (DizWidth)
		{
			FARString strName = cfg_reader.GetString(FTS.MaskName, L"");
			if (static_cast<int>(strName.GetLength()) > DizWidth)
			{
				strName.Truncate(DizWidth - (Opt.NoGraphics ? 3 : 1));
				strName += (Opt.NoGraphics ? L"..." : L"…");
			}
			strMenuText.Format(L"%-*.*ls %lc ", DizWidth, DizWidth, strName.CPtr(), BoxSymbols[BS_V1]);
		}

		strMenuText += strMask;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect(NumLine==MenuPos);
		TypesMenu->AddItem(&TypesMenuItem);
	}

	TypesMenuItem.strName.Clear();
	TypesMenuItem.SetSelect(NumLine==MenuPos);
	TypesMenu->AddItem(&TypesMenuItem);
	return NumLine;
}

static bool EditTypeRecord (int EditPos, int TotalRecords, bool NewRec)
{
	FARString strName, strMasks;

	if (!NewRec)
	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, EditPos);
		strMasks = cfg_reader.GetString(FTS.MaskValue, L"");
		strName = cfg_reader.GetString(FTS.MaskName, L"");
	}

	DialogBuilder Builder(Msg::MaskGroupTitle, L"MaskGroupsSettings");
	Builder.SetId(EditMaskGroupId);
	Builder.AddText(Msg::MaskGroupName);
	Builder.AddEditField(&strName, 60);
	Builder.AddText(Msg::MaskGroupMasks);
	Builder.AddEditField(&strMasks, 60);
	Builder.AddOKCancel();

	if (Builder.ShowDialog() && !strName.IsEmpty() && !strMasks.IsEmpty())
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, EditPos);

		if (NewRec)
		{
			cfg_writer.ReserveIndexedSection(FTS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FTS.MaskValue, strMasks);
		cfg_writer.SetString(FTS.MaskName, strName);
		return true;
	}

	return false;
}

static bool DeleteTypeRecord(int DeletePos)
{
	bool Result=false;
	FARString strItemName;

	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, DeletePos);
		strItemName = cfg_reader.GetString(FTS.MaskName, L"");
	}

	if (!Message(MSG_WARNING,2,Msg::MaskGroupTitle,Msg::MaskGroupAskDelete,strItemName,Msg::Delete,Msg::Cancel))
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, DeletePos);
		cfg_writer.RemoveSection();
		cfg_writer.DefragIndexedSections(FTS.Type0);
		Result=true;
	}

	return Result;
}

void EditMaskTypes()
{
	int NumLine=0;
	int MenuPos=0;
	//RenumKeyRecord(FTS.MaskGroups,FTS.TypeFmt,FTS.Type0);
	VMenu TypesMenu(Msg::MaskGroupTitle,nullptr,0,ScrY-4);
	TypesMenu.SetHelp(FARString(FTS.Help));
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetPosition(-1,-1,0,0);
	TypesMenu.SetId(MaskGroupsMenuId);
	TypesMenu.SetBottomTitle(L"Ins Del F4 F7 Ctrl+R");
	while (1)
	{
		bool MenuModified=true;

		while (!TypesMenu.Done())
		{
			if (MenuModified)
			{
				TypesMenu.Hide();
				NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
				TypesMenu.SetPosition(-1,-1,-1,-1);
				TypesMenu.Show();
				MenuModified=false;
			}

			FarKey Key=TypesMenu.ReadInput();
			MenuPos=TypesMenu.GetSelectPos();

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:
					if (MenuPos<NumLine)
						DeleteTypeRecord(MenuPos);

					MenuModified=true;
					break;

				case KEY_NUMPAD0:
				case KEY_INS:
					EditTypeRecord(MenuPos,NumLine,true);
					MenuModified=true;
					break;

				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:
					if (MenuPos<NumLine)
						EditTypeRecord(MenuPos,NumLine,false);

					MenuModified=true;
					break;

				default:
					TypesMenu.ProcessInput();
					break;
			}
		}

		int ExitCode=TypesMenu.Modal::GetExitCode();

		if (ExitCode!=-1)
		{
			MenuPos=ExitCode;
			TypesMenu.ClearDone();
			TypesMenu.WriteInput(KEY_F4);
			continue;
		}

		break;
	}
}
