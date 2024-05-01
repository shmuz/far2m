/*
MaskGroups.cpp

Groups of file masks
*/

#include "headers.hpp"

#include "filetype.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "CFileMask.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "fnparce.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "ConfigRW.hpp"
#include "DlgGuid.hpp"
#include "DialogBuilder.hpp"

struct FileTypeStrings
{
	const char *Help, *HelpModify, *State,
	*MaskGroups, *TypeFmt, *Type0,
	*Execute, *Desc, *Mask, *View, *Edit,
	*AltExec, *AltView, *AltEdit;
};

const FileTypeStrings FTS=
{
	"FileAssoc", "FileAssocModify", "State",
	"MaskGroups", "MaskGroups/Type%d", "MaskGroups/Type",
	"Execute", "Description", "Mask", "View", "Edit",
	"AltExec", "AltView", "AltEdit"
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

enum EDITTYPERECORD
{
	ETR_DOUBLEBOX,
	ETR_TEXT_MASKS,
	ETR_EDIT_MASKS,
	ETR_TEXT_DESCR,
	ETR_EDIT_DESCR,
	ETR_SEPARATOR1,
	ETR_COMBO_EXEC,
	ETR_EDIT_EXEC,
	ETR_COMBO_ALTEXEC,
	ETR_EDIT_ALTEXEC,
	ETR_COMBO_VIEW,
	ETR_EDIT_VIEW,
	ETR_COMBO_ALTVIEW,
	ETR_EDIT_ALTVIEW,
	ETR_COMBO_EDIT,
	ETR_EDIT_EDIT,
	ETR_COMBO_ALTEDIT,
	ETR_EDIT_ALTEDIT,
	ETR_SEPARATOR2,
	ETR_BUTTON_OK,
	ETR_BUTTON_CANCEL,
};

enum EDITMASKRECORD
{
	EMR_DOUBLEBOX,
	EMR_TEXT_NAME,
	EMR_EDIT_NAME,
	EMR_TEXT_MASKS,
	EMR_EDIT_MASKS,
	EMR_SEPARATOR,
	EMR_BUTTON_OK,
	EMR_BUTTON_CANCEL,
};

static bool EditTypeRecord (int EditPos, int TotalRecords, bool NewRec)
{
	FARString strName, strMasks;

	DialogBuilder Builder(Msg::MaskGroupTitle, L"MaskGroupsSettings");
	Builder.AddText(Msg::MaskGroupName);
	Builder.AddEditField(&strName, 47);
	Builder.AddText(Msg::MaskGroupMasks);
	Builder.AddEditField(&strMasks, 47);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
	}
	return false;
}

static bool EditTypeRecord2(int EditPos, int TotalRecords, bool NewRec)
{
	bool Result = false;
	const int DlgX = 76, DlgY = 23;
	DialogDataEx EditDlgData[]=
	{
		{DI_DOUBLEBOX,3, 1,DlgX-4,DlgY-2,{},0,Msg::FileAssocTitle},
		{DI_TEXT,     5, 2, 0, 2,{},0,Msg::FileAssocMasks},
		{DI_EDIT,     5, 3,DlgX-6, 3,{(DWORD_PTR)L"Masks"},DIF_FOCUS|DIF_HISTORY,L""},
		{DI_TEXT,     5, 4, 0, 4,{},0,Msg::FileAssocDescr},
		{DI_EDIT,     5, 5,DlgX-6, 5,{},0,L""},
		{DI_TEXT,     3, 6, 0, 6,{},DIF_SEPARATOR,L""},
		{DI_CHECKBOX, 5, 7, 0, 7,{1},0,Msg::FileAssocExec},
		{DI_EDIT,     9, 8,DlgX-6, 8,{},DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5, 9, 0, 9,{1},0,Msg::FileAssocAltExec},
		{DI_EDIT,     9,10,DlgX-6,10,{},DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5,11, 0,11,{1},0,Msg::FileAssocView},
		{DI_EDIT,     9,12,DlgX-6,12,{},DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5,13, 0,13,{1},0,Msg::FileAssocAltView},
		{DI_EDIT,     9,14,DlgX-6,14,{},DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5,15, 0,15,{1},0,Msg::FileAssocEdit},
		{DI_EDIT,     9,16,DlgX-6,16,{},DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5,17, 0,17,{1},0,Msg::FileAssocAltEdit},
		{DI_EDIT,     9,18,DlgX-6,18,{},DIF_EDITPATH,L""},
		{DI_TEXT,     3,DlgY-4, 0,DlgY-4,{},DIF_SEPARATOR,L""},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,{},DIF_DEFAULT|DIF_CENTERGROUP,Msg::Ok},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,{},DIF_CENTERGROUP,Msg::Cancel}
	};
	MakeDialogItemsEx(EditDlgData,EditDlg);

	if (!NewRec)
	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, EditPos);
		EditDlg[ETR_EDIT_MASKS].strData = cfg_reader.GetString(FTS.Mask, L"");
		EditDlg[ETR_EDIT_DESCR].strData = cfg_reader.GetString(FTS.Desc, L"");
		EditDlg[ETR_EDIT_EXEC].strData = cfg_reader.GetString(FTS.Execute, L"");
		EditDlg[ETR_EDIT_ALTEXEC].strData = cfg_reader.GetString(FTS.AltExec, L"");
		EditDlg[ETR_EDIT_VIEW].strData = cfg_reader.GetString(FTS.View, L"");
		EditDlg[ETR_EDIT_ALTVIEW].strData = cfg_reader.GetString(FTS.AltView, L"");
		EditDlg[ETR_EDIT_EDIT].strData = cfg_reader.GetString(FTS.Edit, L"");
		EditDlg[ETR_EDIT_ALTEDIT].strData = cfg_reader.GetString(FTS.AltEdit, L"");
		DWORD State = cfg_reader.GetUInt(FTS.State, 0xffffffff);

		for (int i = FILETYPE_EXEC, Item = ETR_COMBO_EXEC; i <= FILETYPE_ALTEDIT; i++, Item+= 2)
		{
			if (!(State&(1<<i)))
			{
				EditDlg[Item].Selected = BSTATE_UNCHECKED;
				EditDlg[Item+1].Flags|= DIF_DISABLE;
			}
		}
	}

	Dialog Dlg(EditDlg, ARRAYSIZE(EditDlg), nullptr); //EditTypeRecordDlgProc);
	Dlg.SetHelp(FARString(FTS.HelpModify));
	Dlg.SetPosition(-1, -1, DlgX, DlgY);
	Dlg.SetId(FileAssocModifyId);
	Dlg.Process();

	if (Dlg.GetExitCode() == ETR_BUTTON_OK)
	{
		ConfigWriter cfg_writer;
		cfg_writer.SelectSectionFmt(FTS.TypeFmt, EditPos);

		if (NewRec)
		{
			cfg_writer.ReserveIndexedSection(FTS.Type0, (unsigned int)EditPos);
		}

		cfg_writer.SetString(FTS.Mask, EditDlg[ETR_EDIT_MASKS].strData);
		cfg_writer.SetString(FTS.Desc, EditDlg[ETR_EDIT_DESCR].strData);
		cfg_writer.SetString(FTS.Execute, EditDlg[ETR_EDIT_EXEC].strData);
		cfg_writer.SetString(FTS.AltExec, EditDlg[ETR_EDIT_ALTEXEC].strData);
		cfg_writer.SetString(FTS.View, EditDlg[ETR_EDIT_VIEW].strData);
		cfg_writer.SetString(FTS.AltView, EditDlg[ETR_EDIT_ALTVIEW].strData);
		cfg_writer.SetString(FTS.Edit, EditDlg[ETR_EDIT_EDIT].strData);
		cfg_writer.SetString(FTS.AltEdit, EditDlg[ETR_EDIT_ALTEDIT].strData);
		DWORD State = 0;

		for (int i = FILETYPE_EXEC,Item=ETR_COMBO_EXEC; i <= FILETYPE_ALTEDIT; i++, Item+= 2)
		{
			if (EditDlg[Item].Selected == BSTATE_CHECKED)
			{
				State|= (1<<i);
			}
		}

		cfg_writer.SetUInt(FTS.State, State);
		Result = true;
	}

	return Result;
}

static bool DeleteTypeRecord(int DeletePos)
{
	bool Result=false;
	FARString strItemName;

	{
		ConfigReader cfg_reader;
		cfg_reader.SelectSectionFmt(FTS.TypeFmt, DeletePos);
		strItemName = cfg_reader.GetString(FTS.Mask, L"");
		InsertQuote(strItemName);
	}

	if (!Message(MSG_WARNING,2,Msg::AssocTitle,Msg::AskDelAssoc,strItemName,Msg::Delete,Msg::Cancel))
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
	VMenu TypesMenu(Msg::AssocTitle,nullptr,0,ScrY-4);
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
