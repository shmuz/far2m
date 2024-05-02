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

struct FileTypeStrings
{
	const char *Help, *HelpModify,
	*MaskGroups, *TypeFmt, *Type0,
	*Desc, *Mask;
};

const FileTypeStrings FTS=
{
	"FileAssoc", "FileAssocModify",
	"MaskGroups", "MaskGroups/Type%d", "MaskGroups/Type",
	"Description", "Mask",
};

static int GetDescriptionWidth(ConfigReader &cfg_reader, const wchar_t *Name=nullptr)
{
	int Width = 0;
//	RenumKeyRecord(FTS.MaskGroups,FTS.TypeFmt,FTS.Type0);
	for (int NumLine=0;; NumLine++)
	{
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FTS.Mask, L""))
			break;

		CFileMask FMask;

		if (!FMask.Set(strMask, FMF_SILENT))
			continue;

		FARString strDescription = cfg_reader.GetString(FTS.Desc, L"");
		int CurWidth;

		if (!Name)
		{
			CurWidth = HiStrCellsCount(strDescription);
		}
		else
		{
			if (!FMask.Compare(Name))
				continue;

			FARString strExpandedDesc = strDescription;
			SubstFileName(strExpandedDesc,Name,nullptr,nullptr,TRUE);
			CurWidth = HiStrCellsCount(strExpandedDesc);
		}

		if (CurWidth>Width)
			Width=CurWidth;
	}

	if (Width>ScrX/2)
		Width=ScrX/2;

	return(Width);
}

static int FillFileTypesMenu(VMenu *TypesMenu,int MenuPos)
{
	ConfigReader cfg_reader;
	int DizWidth=GetDescriptionWidth(cfg_reader);
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	int NumLine=0;

	for (;; NumLine++)
	{
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, NumLine);
		FARString strMask;
		if (!cfg_reader.GetString(strMask, FTS.Mask, L""))
			break;

		TypesMenuItem.Clear();

		FARString strMenuText;

		if (DizWidth)
		{
			FARString strDescription = cfg_reader.GetString(FTS.Desc, L"");
			FARString strTitle = strDescription;
			size_t Pos=0;
			bool Ampersand=strTitle.Pos(Pos,L'&');

			if (DizWidth+Ampersand > ScrX/2 && Ampersand && static_cast<int>(Pos) > DizWidth)
				Ampersand=false;

			strMenuText.Format(L"%-*.*ls %lc ",DizWidth+Ampersand,DizWidth+Ampersand,strTitle.CPtr(),BoxSymbols[BS_V1]);
		}

		//TruncStr(strMask,ScrX-DizWidth-14);
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
		strMasks = cfg_reader.GetString(FTS.Mask, L"");
		strName = cfg_reader.GetString(FTS.Desc, L"");
	}

	DialogBuilder Builder(Msg::MaskGroupTitle, L"MaskGroupsSettings");
	Builder.AddText(Msg::MaskGroupName);
	Builder.AddEditField(&strName, 47);
	Builder.AddText(Msg::MaskGroupMasks);
	Builder.AddEditField(&strMasks, 47);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, EditPos);

		if (NewRec)
		{
			cfg_writer.ReserveIndexedSection(FTS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FTS.Mask, strMasks);
		cfg_writer.SetString(FTS.Desc, strName);
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
		strItemName = cfg_reader.GetString(FTS.Desc, L"");
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
	TypesMenu.SetId(FileAssocMenuId);
	TypesMenu.SetBottomTitle(Msg::AssocBottom);
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
