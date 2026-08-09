#include "fardialog.h"
#include "plugincfg.h"
#include "netcfglng.h"
#include "netcfgplugin.h"

#include <common/log.h>
#include <common/errname.h>

#include <utils.h>
#include <memory>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "fardialog.cpp"


#if INTPTR_MAX == INT32_MAX
#define LLFMT L"%lld"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT L"%ld"
#else
    #error "Environment not 32 or 64-bit."
#endif

void FarDlgConstructor::SetText(unsigned int itemNum, const wchar_t * str, bool dublicate)
{
	static const wchar_t * empty_string = L"";
	if( free_items.find(itemNum) != free_items.end() ) {
		free((void *)dlg[itemNum].PtrData);
		free_items.erase(itemNum);
	}

	if( str ) {
		if( dublicate ) {
			free_items.insert(itemNum);
			dlg[itemNum].PtrData = wcsdup(str);
		} else {
			dlg[itemNum].PtrData = str;
		}
	} else
		dlg[itemNum].PtrData = empty_string;

}

void FarDlgConstructor::SetCountText(unsigned int itemNum, int64_t value)
{
	wchar_t max64[sizeof("18446744073709551615")] = {0};
	if( NetCfgPlugin::FSF.snprintf(max64, ARRAYSIZE(max64), LLFMT, value) > 0 )
		SetText(itemNum, max64, true);
	return;
}

void FarDlgConstructor::SetCountHex32Text(unsigned int itemNum, uint32_t value, bool prefix)
{
	wchar_t maxhex32[sizeof("0xFFFFFFFF")] = {0};
	if( NetCfgPlugin::FSF.snprintf(maxhex32, ARRAYSIZE(maxhex32), prefix ? L"0x%08X":L"%08X", value) > 0 )
		SetText(itemNum, maxhex32, true);
	return;
}

void FarDlgConstructor::SetFileSizeText(unsigned int itemNum, uint64_t value)
{
	if( value > 100 * 1024 ) {
		std::wstring _tmp = FileSizeString(value);
		SetText(itemNum, _tmp.c_str(), true);
		return;
	}
	return SetCountText(itemNum, (int64_t)value);
}

void FarDlgConstructor::SetX(unsigned int itemNum, int x)
{
	dlg[itemNum].X1 = x;
}

void FarDlgConstructor::SetSelected(unsigned int itemNum, bool selected)
{
	dlg[itemNum].Selected = selected;
}

void FarDlgConstructor::SetFocus(unsigned int itemNum)
{
	dlg[itemNum].Focus = 1;
}

void FarDlgConstructor::SetDefaultButton(unsigned int itemNum)
{
	dlg[itemNum].DefaultButton = 1;
}

DWORD FarDlgConstructor::GetFlags(unsigned int itemNum)
{
	return dlg[itemNum].Flags;
}

void FarDlgConstructor::SetFlags(unsigned int itemNum, DWORD flags)
{
	dlg[itemNum].Flags = flags;
}

void FarDlgConstructor::OrFlags(unsigned int itemNum, DWORD flags)
{
	dlg[itemNum].Flags |= flags;
}

void FarDlgConstructor::AndFlags(unsigned int itemNum, DWORD flags)
{
	dlg[itemNum].Flags &= flags;
}

void FarDlgConstructor::HideItems(unsigned int itemFirst, unsigned int itemLast)
{
	assert( itemFirst <= itemLast );
	for( unsigned int i = itemFirst; i <= itemLast; i++ )
		dlg[i].Flags |= DIF_HIDDEN;
}

void FarDlgConstructor::UnhideItems(unsigned int itemFirst, unsigned int itemLast)
{
	assert( itemFirst <= itemLast );
	for( unsigned int i = itemFirst; i <= itemLast; i++ )
		dlg[i].Flags &= ~DIF_HIDDEN;
}

void FarDlgConstructor::SetMaxTextLen(unsigned int itemNum, size_t maxLen)
{
	dlg[itemNum].MaxLen = maxLen;
}

// return last appended item index
unsigned int FarDlgConstructor::Append(const DlgConstructorItem * item)
{
	const wchar_t * empty_string = L"";

	if( item->type == DI_ENDDIALOG )
		return (dlg.size() - 1);

/*
struct FarDialogItem
{
	int Type;
	int X1,Y1,X2,Y2;
	int Focus;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		const wchar_t *History;
		const wchar_t *Mask;
		struct FarList *ListItems;
		int  ListPos;
		CHAR_INFO *VBuf;
	}
#ifdef _FAR_NO_NAMELESS_UNIONS
	Param
#endif
	;
	DWORD Flags;
	int DefaultButton;

	const wchar_t *PtrData;
	size_t MaxLen; // terminate 0 not included (if == 0 string size is unlimited)
};
*/
        struct FarDialogItem fdi = {
		item->type,
		item->X1,
		item->incY ? (++yEnd):yEnd,
		item->X2,
		yEnd,
		0,
		{0},
		item->flags,
		0,
		(item->data.ptrData && item->data.ptrData < (const wchar_t *)MMaxString) ? \
			(NetCfgPlugin::psi.GetMsg(NetCfgPlugin::psi.ModuleNumber, item->data.lngIdstring)):\
			(item->data.ptrData ? item->data.ptrData:empty_string),
		0};
	dlg.push_back(fdi);

	//LOG_INFO("itemNum: %u text: %S item.Type %d total: %d\n", dlg.size() - 1, fdi.PtrData ? fdi.PtrData:L"NONE", item->type, GetNumberOfItems());

	dlg[0].Y2 = (yEnd + 1);
	return (dlg.size() - 1);
}

// return next appended item index
unsigned int FarDlgConstructor::AppendItems(const DlgConstructorItem * items)
{
	while( items->type != DI_ENDDIALOG ) {
		Append(items);
		items++;
	}
	return dlg.size();
}

// return index of first element in appended suffix
unsigned int FarDlgConstructor::AppendOkCancel(void) {
	static const DlgConstructorItem suffix[] = {
		//  Type       NewLine       X1        X2                  Flags                 PtrData
		{DI_TEXT,      true,   5,              0,            DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_BUTTON,    true,   0,              0,              DIF_CENTERGROUP,          {.lngIdstring = MOk}},
		{DI_BUTTON,    false,  0,              0,              DIF_CENTERGROUP,          {.lngIdstring = MCancel}},
		{DI_ENDDIALOG, 0}
	};
	unsigned int suffixOff = dlg.size();
	AppendItems(&suffix[0]);
	return suffixOff;
}

FarDlgConstructor::FarDlgConstructor(const DlgConstructorItem * items, int y)
{
	xStart = items->X1;
	yStart = y;
	xEnd = items->X2 + xStart + 1;
	yEnd = y;
	AppendItems(items);
}

FarDlgConstructor::FarDlgConstructor(const wchar_t * title, int width, int x, int y)
{
	xStart = x;
	yStart = y;
	xEnd = width;
	yEnd = y;

	DlgConstructorItem item = {DI_DOUBLEBOX, false, xStart, xEnd-(xStart+1), 0, {.ptrData = title}};
	Append(&item);
}

FarDlgConstructor::~FarDlgConstructor()
{
	for( auto itemNum : free_items ) {
		LOG_INFO("free: %d\n", itemNum);
		free((void *)dlg[itemNum].PtrData);
	}
}

unsigned int FarDlgConstructor::GetNumberOfItems(void)
{
	return dlg.size();
}

FarDialogItem * FarDlgConstructor::GetDialogItem(unsigned int itemNum)
{
	if( dlg.size() <= itemNum )
		return nullptr;
	return &dlg[itemNum];
}

const wchar_t * FarDlgConstructor::GetConstText(unsigned int itemNum) const
{
	return dlg[itemNum].PtrData;
}

const wchar_t * FarDlgConstructor::GetHelpTopic(void)
{
	return helpTopic.c_str();
}

void FarDlgConstructor::SetHelpTopic(const wchar_t * help)
{
	helpTopic = help;
}

FarDialog::FarDialog(FarDlgConstructor * data, FARWINDOWPROC dlgProc, LONG_PTR param):
		fdc(data)
{
	FarDialogItem * item = fdc->GetDialogItem(0);
	hDlg = NetCfgPlugin::psi.DialogInit(
		NetCfgPlugin::psi.ModuleNumber,		// INT_PTR               PluginNumber
		-1,                                     // int                   X1,         
		-1,                                     // int                   Y1,         
		fdc->xEnd,                             	// int                   X2,         
		fdc->yEnd+fdc->yStart+2,     		// int                   Y2,
		fdc->GetHelpTopic(),                    // const wchar_t        *HelpTopic,  
		item,                                   // struct FarDialogItem *Item,       
		fdc->GetNumberOfItems(),                // unsigned int          ItemsNumber,
		0,                                      // DWORD                 Reserved,   
		0,                                      // DWORD                 Flags,      
		dlgProc,                                // FARWINDOWPROC         DlgProc,    
		(LONG_PTR)param);                       // LONG_PTR              Param       

	if (hDlg == INVALID_HANDLE_VALUE) {
		LOG_ERROR("DialogInit()\n");
	}
	LOG_INFO("\n");
}

FarDialog::~FarDialog()
{
	LOG_INFO("\n");
	if( hDlg != INVALID_HANDLE_VALUE )
		NetCfgPlugin::psi.DialogFree(hDlg);
}

int FarDialog::Run(void)
{
	LOG_INFO("\n");
	return NetCfgPlugin::psi.DialogRun(hDlg);
}


ItemChange::ItemChange(unsigned int itemNum_, int type_): itemNum(itemNum_), type(type_)
{
	const wchar_t * const empty_string = L"";
	oldVal.ptrData = nullptr;
	ptrData = empty_string;
	empty = true;
	if( type == DI_BUTTON )
		newVal.ptrData = ptrData.c_str();
	else
		newVal.ptrData = nullptr;
}

ItemChange::ItemChange(ItemChange && old)
{
	itemNum = old.itemNum;
	type = old.type;
	empty = old.empty;
	oldVal.ptrData = old.oldVal.ptrData;
	ptrData = std::move(old.ptrData);
	if( type == DI_BUTTON )
		newVal.ptrData = ptrData.c_str();
	else
		newVal.ptrData = old.newVal.ptrData;
}

bool FarDialog::CreateChangeList(std::vector<ItemChange> & chlst)
{
	bool change = false;
	unsigned int itemNum = 0;
	for( auto & item : *fdc ) {
		ItemChange ich(itemNum, item.Type);
		//LOG_INFO("itemNum: %u text: %S item.Type %d total: %d\n", itemNum, item.PtrData ? item.PtrData:L"NONE", item.Type, fdc->GetNumberOfItems());
		switch(item.Type) {
		case DI_BUTTON:
			ich.oldVal.ptrData = item.PtrData;
			ich.ptrData = GetText(itemNum);
			ich.newVal.ptrData = ich.ptrData.c_str();
			ich.empty = ich.ptrData.empty();
			if( NetCfgPlugin::FSF.LStricmp(ich.newVal.ptrData, ich.oldVal.ptrData) ) {
				LOG_INFO("%u. DI_BUTTON: change from: %S to %S\n", itemNum, ich.oldVal.ptrData, ich.newVal.ptrData);
				change = true;
				chlst.push_back(std::move(ich));
			}
			break;
		case DI_CHECKBOX:
		case DI_RADIOBUTTON:
			ich.oldVal.Selected = item.Selected;
			ich.newVal.Selected = GetCheck(itemNum);
			if( ich.newVal.Selected != ich.oldVal.Selected ) {
				change = true;
				chlst.push_back(std::move(ich));
			}
			break;
		case DI_EDIT:
			ich.oldVal.ptrData = item.PtrData;
			ich.newVal.ptrData = GetConstText(itemNum);
			ich.empty = (wcslen(ich.newVal.ptrData) == 0);
			if( NetCfgPlugin::FSF.LStricmp(ich.newVal.ptrData, ich.oldVal.ptrData) ) {
				LOG_INFO("%u. DI_EDIT: change from: %S to %S\n", itemNum, ich.oldVal.ptrData, ich.newVal.ptrData);
				change = true;
				chlst.push_back(std::move(ich));
			}
			break;
		}
		itemNum++;
	}
	return change;
}

bool FarDialog::GetCheck(unsigned int itemNum)
{
	return bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, itemNum, 0));
}

std::wstring FarDialog::GetText(unsigned int itemNum)
{
	std::wstring text;
	size_t len = NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, itemNum, 0);
	if( len ) {
		auto buf = std::make_unique<wchar_t[]>(len+1);
		NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTPTR, itemNum, (ULONG_PTR)buf.get());
		text = buf.get();
	}
	return text;
}

const wchar_t * FarDialog::GetConstText(unsigned int itemNum)
{
	return (const wchar_t *)NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, itemNum, 0);
}

int32_t FarDialog::GetInt32(unsigned int itemNum)
{
	return NetCfgPlugin::FSF.atoi(GetConstText(itemNum));
}

int64_t FarDialog::GetInt64(unsigned int itemNum)
{
	return NetCfgPlugin::FSF.atoi64(GetConstText(itemNum));
}

int64_t FarDialog::GetInt64FromHex(unsigned int itemNum, bool prefix)
{
	unsigned long long ofs = 0;
	NetCfgPlugin::FSF.sscanf(GetConstText(itemNum), prefix ? L"0x%llx":L"%llx", &ofs);
	return ofs;
}

int32_t FarDialog::GetInt32FromHex(unsigned int itemNum, bool prefix)
{
	return (int32_t)GetInt64FromHex(itemNum, prefix);
}

HANDLE FarDialog::GetDlg(void)
{
	return hDlg;
}

void ChangeDialogItemsView(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast, bool hide, bool disable)
{
	assert( itemFirst <= itemLast );
	for(unsigned int i = itemFirst; i <= itemLast; i++ ) {
		struct FarDialogItem *dlg = (FarDialogItem *)malloc(NetCfgPlugin::psi.SendDlgMessage(hDlg,DM_GETDLGITEM,i,0));
		if( dlg ) {
			NetCfgPlugin::psi.SendDlgMessage(hDlg,DM_GETDLGITEM,i,(LONG_PTR)dlg);
			if( disable )
				dlg->Flags |= DIF_DISABLE;
			else
				dlg->Flags &= ~DIF_DISABLE;

			if( hide )
				dlg->Flags |= DIF_HIDDEN;
			else
				dlg->Flags &= ~DIF_HIDDEN;

			NetCfgPlugin::psi.SendDlgMessage(hDlg,DM_SETDLGITEM,i,(LONG_PTR)dlg);
			free(dlg);
		}
	}
	NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
}

void HideDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast)
{
	ChangeDialogItemsView(hDlg, itemFirst, itemLast, true, false);
}

void UnhideDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast)
{
	ChangeDialogItemsView(hDlg, itemFirst, itemLast, false, false);
}

void DisableDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast)
{
	ChangeDialogItemsView(hDlg, itemFirst, itemLast, false, true);
}

void EnableDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast)
{
	ChangeDialogItemsView(hDlg, itemFirst, itemLast, false, false);
}

uint64_t GetNumberItem(HANDLE hDlg, unsigned int itemNum)
{
	size_t len = NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, itemNum, 0);
	if( len ) {
		auto buf = std::make_unique<wchar_t[]>(len+1);
		NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTPTR, itemNum, (ULONG_PTR)buf.get());
		return NetCfgPlugin::FSF.atoi64(buf.get());
	}
	return 0;
}

std::wstring GetText(HANDLE hDlg, unsigned int itemNum)
{
	std::wstring text;
	size_t len = NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTLENGTH, itemNum, 0);
	if( len ) {
		auto buf = std::make_unique<wchar_t[]>(len+1);
		NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETTEXTPTR, itemNum, (ULONG_PTR)buf.get());
		text = buf.get();
	}
	return text;
}

bool ShowHideElements(HANDLE hDlg, uint32_t chk, uint32_t chkStore, uint32_t begin, uint32_t end)
{
	bool prev = bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, chkStore, 0));
	bool enabled = bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, chk, 0));
	if( prev != enabled ) {
		NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETCHECK, chkStore, enabled);
		ChangeDialogItemsView(hDlg, begin, end, false, !enabled);
	}
	return enabled;
}
