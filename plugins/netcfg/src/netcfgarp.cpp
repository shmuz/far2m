#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "netcfgplugin.h"
#include "fardialog.h"

#include <utils.h>

#include <assert.h>

#include <common/log.h>
#include <common/errname.h>
#include <common/netutils.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <memory>
#include <map>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "netcfgarp.cpp"

NetcfgArpRoute::NetcfgArpRoute(uint32_t index_, std::deque<ArpRouteInfo> & arp_, std::map<uint32_t, std::wstring> & ifs_):
	NetFarPanel(index_, ifs_),
	arp(arp_)
{
	LOG_INFO("index_ %u this %p\n", index_, this);
}

NetcfgArpRoute::~NetcfgArpRoute()
{
	LOG_INFO("\n");
}

const int DIALOG_WIDTH = 80;

bool NetcfgArpRoute::DeleteArp(void)
{
	LOG_INFO("\n");

	bool change = false;

	PanelInfo pi = {0};
	GetPanelInfo(pi);

	do {
		if( !pi.SelectedItemsNumber )
			break;

		FarDlgConstructor fdc(L"Delete arp(s):", DIALOG_WIDTH);
		fdc.SetHelpTopic(L"DeleteArp");

		std::vector<ArpRouteInfo *> delarps;

		static const DlgConstructorItem arp_dlg = {DI_TEXT, true, 5, 0, 0, {0}};

		for( int i = 0; i < pi.SelectedItemsNumber; i++ ) {
			auto ppi = GetSelectedPanelItem(i);
			if( ppi && ppi->Flags & PPIF_USERDATA ) {

        			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));

				auto r = ((PluginUserData *)ppi->UserData)->data.arp;
				delarps.push_back(r);

				auto off = fdc.Append(&arp_dlg);
				std::wstring _a(std::to_wstring(i+1)+L". ");

				_a += r->ip;
				_a += L" ";
				_a += r->mac.empty() ? L"00:00:00:00:00:00":r->mac;
				_a += L" ";
				_a += r->iface;

				if( _a.size() > (DIALOG_WIDTH - 10) ) {
					_a.resize(DIALOG_WIDTH - 13);
					_a += L"...";
				}
				fdc.SetText(off, _a.c_str(), true);
			}
			FreePanelItem(ppi);
		}

		if( delarps.size() == 0 )
			break;

		auto offSufix = fdc.AppendOkCancel();

		fdc.SetDefaultButton(offSufix + WinSuffixOkIndex);
		fdc.SetFocus(offSufix + WinSuffixOkIndex);

		FarDialog dlg(&fdc);

		if( dlg.Run() != static_cast<int>(offSufix) + WinSuffixOkIndex )
			break;

		for( auto & r : delarps )
			change |= r->Delete();

	} while(0);

	return change;
}

const int EDIT_ARP_DIALOG_WIDTH = 85;
enum {
	WinEditArpCaptionIndex,
	WinEditArpFamilyButtonIndex,
	WinEditArpIpTextIndex,
	WinEditArpIpEditIndex,
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	WinEditArpProxyCheckboxIndex,
	WinEditArpProxyCheckboxStoreIndex,
#endif
	WinEditArpMacTextIndex,
	WinEditArpMacEditIndex,

	WinEditArpSeparator1TextIndex,

	WinEditArpIfaceTextIndex,
	WinEditArpIfaceButtonIndex,

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	WinEditArpNoRadiobuttonIndex,
	WinEditArpUseRadiobuttonIndex,
	WinEditArpManagedRadiobuttonIndex,
	WinEditArpProtocolTextIndex,
	WinEditArpProtocolButtonIndex,

	WinEditArpSeparator2TextIndex,

	WinEditArpStateTextIndex,
	WinEditArpStateButtonIndex,

	WinEditArpRouterCheckboxIndex,
	WinEditArpExternLearnCheckboxIndex,
#else

	WinEditArpPublicCheckboxIndex,
	WinEditArpProxyCheckboxIndex,

	WinEditArpIfscopeTextIndex,
	WinEditArpIfscopeButtonIndex,

	WinEditArpNormalRadiobuttonIndex,
	WinEditArpRejectRadiobuttonIndex,
	WinEditArpBlackholeRadiobuttonIndex,
#endif

	WinEditArpMaxIndex
};



LONG_PTR WINAPI EditArpDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgArpRoute * rtCfg = 0;

	switch( msg ) {

	case DN_INITDIALOG:
		rtCfg = (NetcfgArpRoute *)param2;
		break;
	case DM_CLOSE:
		rtCfg = 0;
		break;
	case DN_DRAWDLGITEM:
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		if( param1 == WinEditArpProxyCheckboxIndex ) {
			bool prev = bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, WinEditArpProxyCheckboxStoreIndex, 0));
			bool enabled = bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, WinEditArpProxyCheckboxIndex, 0));
			if( prev != enabled ) {
				NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETCHECK, WinEditArpProxyCheckboxStoreIndex, enabled);
				ChangeDialogItemsView(hDlg, WinEditArpMacTextIndex, WinEditArpMacEditIndex, false, enabled);
				ChangeDialogItemsView(hDlg, WinEditArpStateTextIndex, WinEditArpStateButtonIndex, false, enabled);
			}
		}
#endif
		break;
	case DN_BTNCLICK:
		assert( rtCfg != 0 );
		switch( param1 ) {
		case WinEditArpFamilyButtonIndex:
			rtCfg->new_a.sa_family = rtCfg->SelectFamily(hDlg, param1);
			#if defined(__APPLE__) || defined(__FreeBSD__)
			ChangeDialogItemsView(hDlg, WinEditArpIfscopeTextIndex, WinEditArpBlackholeRadiobuttonIndex, rtCfg->new_a.sa_family == AF_INET6, false);
			ChangeDialogItemsView(hDlg, WinEditArpIfaceTextIndex, WinEditArpIfaceButtonIndex, rtCfg->new_a.sa_family == AF_INET, false);
			ChangeDialogItemsView(hDlg, WinEditArpProxyCheckboxIndex, WinEditArpProxyCheckboxIndex, rtCfg->new_a.sa_family == AF_INET, false);
			ChangeDialogItemsView(hDlg, WinEditArpPublicCheckboxIndex, WinEditArpPublicCheckboxIndex, rtCfg->new_a.sa_family == AF_INET6, false);
			#endif
			return true;
		case WinEditArpIfaceButtonIndex:
			rtCfg->new_a.valid.iface = rtCfg->SelectInterface(hDlg, param1, nullptr);
			rtCfg->new_a.iface = GetText(hDlg, param1);
			return true;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		case WinEditArpProtocolButtonIndex:
			rtCfg->new_a.protocol = rtCfg->SelectProto(hDlg, param1, rtCfg->new_a.protocol);
			rtCfg->new_a.valid.protocol = rtCfg->new_a.protocol != 0;
			return true;
		case WinEditArpStateButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"permanent",
				L"reachable",
				L"noarp",
				L"stale",
				L"incomplete",
				L"delay",
				L"probe",
				L"failed",
				L"none"
				};
			auto state = rtCfg->Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), param1);
			static const uint16_t def_state[] {
				NUD_PERMANENT,
				NUD_REACHABLE,
	                        NUD_NOARP,
				NUD_STALE,
				NUD_INCOMPLETE,
				NUD_DELAY,
				NUD_PROBE,
				NUD_FAILED,
				NUD_NONE
			};
			if( state < 0 || state >= static_cast<int>(ARRAYSIZE(def_state)) )
				return true;
			rtCfg->new_a.state = def_state[state];
			rtCfg->new_a.valid.state = 1;
			return true;
		}
#else
		case WinEditArpIfscopeButtonIndex:
			{
			uint32_t scope_index;
			rtCfg->SelectInterface(hDlg, param1, &scope_index);
			return true;
			}
#endif
		};
		break;
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

bool NetcfgArpRoute::FillNewArp(void)
{
	LOG_INFO("\n");

	bool change = false;

	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_ARP_DIALOG_WIDTH-4,  0,     {.ptrData = L"Edit arp record:"}},
		{DI_BUTTON,    true,   5,              0,           0,                  {0}},
		{DI_TEXT,      false,  15,             0,           0,                  {.ptrData = L"ip:"}},
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{DI_EDIT,      false,  19,            44,           DIF_LEFTTEXT,       {0}},
		{DI_CHECKBOX,  false,  46,             0,           0,                  {.ptrData = L"proxy"}},
	/*Store*/ {DI_CHECKBOX,  false,  80,            0,           DIF_HIDDEN,         {0}},
#else
		{DI_EDIT,      false,  19,            54,           DIF_LEFTTEXT,       {0}},
#endif
		{DI_TEXT,      false,  56,             0,           0,                  {.ptrData = L"mac:"}},
		{DI_EDIT,      false,  61,            79,           DIF_LEFTTEXT,       {0}},

		{DI_TEXT,      true,   5,              0,           0,                  {0}},
		{DI_TEXT,      true,   5,              0,           0,                  {.ptrData = L"dev:"}},
		{DI_BUTTON,    false,  10,             0,           0,                  {0}},

#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{DI_RADIOBUTTON, false, 30,             0,           DIF_GROUP,         {.ptrData = L""}},
		{DI_RADIOBUTTON, false, 34,             0,           0,                 {.ptrData = L"use"}},
		{DI_RADIOBUTTON, false, 42,             0,           0,                 {.ptrData = L"managed"}},

		{DI_TEXT,      false,  56,             0,           0,                  {.ptrData = L"proto:"}},
		{DI_BUTTON,    false,  63,             0,           0,                  {0}},

		{DI_TEXT,      true,   5,              0,           0,                  {0}},

		{DI_TEXT,      true,   5,              0,           0,                  {.ptrData = L"state:"}},
		{DI_BUTTON,    false,  12,             0,           0,                  {0}},

		{DI_CHECKBOX,  false,  27,             0,           0,                  {.ptrData = L"router"}},
		{DI_CHECKBOX,  false,  38,             0,           0,                  {.ptrData = L"extern_learn"}},
#else

		{DI_CHECKBOX,  false,   5,              0,           DIF_HIDDEN,        {.ptrData = L"public"}},
		{DI_CHECKBOX,  false,  22,              0,           DIF_HIDDEN,        {.ptrData = L"proxy"}},

		{DI_TEXT,      false,  22,              0,           0,                 {.ptrData = L"ifscope:"}},
		{DI_BUTTON,    false,  32,              0,           0,                 {0}},

		{DI_RADIOBUTTON, false,44,              0,           DIF_GROUP,         {.ptrData = L"normal"}},
		{DI_RADIOBUTTON, false,55,              0,           0,                 {.ptrData = L"reject"}},
		{DI_RADIOBUTTON, false,66,              0,           0,                 {.ptrData = L"blackhole"}},
#endif

		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	fdc.SetText(WinEditArpFamilyButtonIndex, towstr(ipfamilyname(new_a.sa_family)).c_str(), true);

	if( new_a.valid.ip )
		fdc.SetText(WinEditArpIpEditIndex, new_a.ip.c_str());

	if( new_a.valid.mac )
		fdc.SetText(WinEditArpMacEditIndex, new_a.mac.c_str());

	if( new_a.valid.iface )
		fdc.SetText(WinEditArpIfaceButtonIndex, new_a.iface.c_str());

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( new_a.flags & NTF_PROXY ) {
		fdc.SetSelected(WinEditArpProxyCheckboxIndex, true);
		fdc.OrFlags(WinEditArpMacTextIndex, DIF_DISABLE);
		fdc.OrFlags(WinEditArpMacEditIndex, DIF_DISABLE);
		fdc.OrFlags(WinEditArpStateTextIndex, DIF_DISABLE);
		fdc.OrFlags(WinEditArpStateButtonIndex, DIF_DISABLE);
	}

	if( new_a.flags & NTF_ROUTER )
		fdc.SetSelected(WinEditArpRouterCheckboxIndex, true);

	if( new_a.flags & NTF_EXT_LEARNED )
		fdc.SetSelected(WinEditArpExternLearnCheckboxIndex, true);

	if( new_a.valid.flags_ext && new_a.flags_ext & NTF_EXT_MANAGED )
		fdc.SetSelected(WinEditArpManagedRadiobuttonIndex, true);
	else if( new_a.flags & NTF_USE )
		fdc.SetSelected(WinEditArpUseRadiobuttonIndex, true);
	else
		fdc.SetSelected(WinEditArpNoRadiobuttonIndex, true);

	if( new_a.valid.protocol )
		fdc.SetText(WinEditArpProtocolButtonIndex, towstr(rtprotocoltype(new_a.protocol)).c_str(), true);

	if( new_a.valid.state )
		fdc.SetText(WinEditArpStateButtonIndex, towstr(ndmsgstate(new_a.state)).c_str(), true);

#else
	if( new_a.valid.flags ) {
		if( new_a.flags & RTF_BLACKHOLE )
			fdc.SetSelected(WinEditArpBlackholeRadiobuttonIndex, true);
		else if( new_a.flags & RTF_REJECT )
			fdc.SetSelected(WinEditArpRejectRadiobuttonIndex, true);
		else
			fdc.SetSelected(WinEditArpNormalRadiobuttonIndex, true);
		if( new_a.flags & RTF_IFSCOPE && new_a.valid.iface )
			fdc.SetText(WinEditArpIfscopeButtonIndex, new_a.iface.c_str());

		if( new_a.flags & RTF_ANNOUNCE ) {
			fdc.SetSelected(WinEditArpPublicCheckboxIndex, true);
			fdc.SetSelected(WinEditArpProxyCheckboxIndex, true);
		}
	}

	if( new_a.sa_family == AF_INET6) {
		fdc.HideItems(WinEditArpIfscopeTextIndex, WinEditArpBlackholeRadiobuttonIndex);
		fdc.AndFlags(WinEditArpProxyCheckboxIndex, ~DIF_HIDDEN);
	} else {
		fdc.HideItems(WinEditArpIfaceTextIndex, WinEditArpIfaceButtonIndex);
		fdc.AndFlags(WinEditArpPublicCheckboxIndex, ~DIF_HIDDEN);
	}
#endif

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	FarDialog dlg(&fdc, EditArpDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			switch( item.itemNum ) {
			case WinEditArpIpEditIndex:
				new_a.valid.ip = !item.empty;
				new_a.ip = item.newVal.ptrData;
				break;
			case WinEditArpMacEditIndex:
				new_a.valid.mac = !item.empty;
				new_a.mac = item.newVal.ptrData;
				break;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
			case WinEditArpProxyCheckboxIndex:
				if( item.newVal.Selected )
					new_a.flags |= NTF_PROXY;
				else
					new_a.flags &= ~NTF_PROXY;
				new_a.valid.flags = 1;
				break;
			case WinEditArpNoRadiobuttonIndex:
				if( item.newVal.Selected ) {
					new_a.flags &= ~NTF_USE;
					new_a.valid.flags = 1;
					new_a.flags_ext &= ~NTF_EXT_MANAGED;
					new_a.valid.flags_ext = 1;
				}
				break;
			case WinEditArpUseRadiobuttonIndex:
				if( item.newVal.Selected ) {
					new_a.flags |= NTF_USE;
					new_a.valid.flags = 1;
					new_a.flags_ext &= ~NTF_EXT_MANAGED;
					new_a.valid.flags_ext = 1;
				}
				break;
			case WinEditArpManagedRadiobuttonIndex:
				if( item.newVal.Selected ) {
					new_a.flags &= ~NTF_USE;
					new_a.valid.flags = 1;
					new_a.flags_ext |= NTF_EXT_MANAGED;
					new_a.valid.flags_ext = 1;
				}
				break;
			case WinEditArpRouterCheckboxIndex:
				if( item.newVal.Selected )
					new_a.flags |= NTF_ROUTER;
				else
					new_a.flags &= ~NTF_ROUTER;
				new_a.valid.flags = 1;
				break;
			case WinEditArpExternLearnCheckboxIndex:
				if( item.newVal.Selected )
					new_a.flags |= NTF_EXT_LEARNED;
				else
					new_a.flags &= ~NTF_EXT_LEARNED;
				new_a.valid.flags = 1;
				break;
#else
			case WinEditArpIfscopeButtonIndex:
				new_a.iface = item.newVal.ptrData;
				new_a.valid.iface = !item.empty;
				if( new_a.valid.iface )
					new_a.flags |= RTF_IFSCOPE;
				else
					new_a.flags &= ~RTF_IFSCOPE;
				new_a.valid.flags = 1;
				break;
			case WinEditArpBlackholeRadiobuttonIndex:
				if( item.newVal.Selected )
					new_a.flags |= RTF_BLACKHOLE;
				else
					new_a.flags &= ~RTF_BLACKHOLE;
				new_a.valid.flags = 1;
				break;
			case WinEditArpRejectRadiobuttonIndex:
				if( item.newVal.Selected )
					new_a.flags |= RTF_REJECT;
				else
					new_a.flags &= ~RTF_REJECT;
				new_a.valid.flags = 1;
				break;
			case WinEditArpNormalRadiobuttonIndex:
				if( item.newVal.Selected )
					new_a.flags &= ~(RTF_BLACKHOLE|RTF_REJECT);
				new_a.valid.flags = 1;
				break;
			case WinEditArpPublicCheckboxIndex:
			case WinEditArpProxyCheckboxIndex:
				if( item.newVal.Selected )
					new_a.flags |= RTF_ANNOUNCE;
				else
					new_a.flags &= ~RTF_ANNOUNCE;
				new_a.valid.flags = 1;
				break;
#endif
			};
		}
	}
	return change;
}

bool NetcfgArpRoute::EditArp(void)
{
	LOG_INFO("\n");

	bool change = false;

	auto ppi = GetCurrentPanelItem();
	if( ppi ) {
		if( ppi->Flags & PPIF_USERDATA ) {

			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));

			a = ((PluginUserData *)ppi->UserData)->data.arp;
			new_a = *a;
			if( FillNewArp() )
				change = a->Change(new_a);
			a = nullptr;
		}
		FreePanelItem(ppi);
	}
	return change;
}

bool NetcfgArpRoute::CreateArp(void)
{
	LOG_INFO("\n");
	bool change = false;

	a = nullptr;
	new_a = ArpRouteInfo();

#if defined(__APPLE__) || defined(__FreeBSD__)	
	new_a.valid.flags = 1;
	new_a.flags |= RTF_IFSCOPE;
#endif

	if( FillNewArp() )
		change = new_a.Create();

	return change;
}

int NetcfgArpRoute::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change)
{
	LOG_INFO("\n");

	if( controlState == 0 ) {
		switch( key ) {
		case VK_F8:
			change = DeleteArp();
			return TRUE;
		case VK_F4:
			change = EditArp();
			return TRUE;
		}
	}

	if( controlState == PKF_SHIFT && key == VK_F4 ) {
		change = CreateArp();
		return TRUE;
	}

	return GetPanelTitleKey(key, controlState) != 0;
}


int NetcfgArpRoute::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	const static wchar_t * empty_string = L"";

	LOG_INFO("\n");
	*pItemsNumber = arp.size();
	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));
	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;
	for( const auto & item : arp ) {
		pi->FindData.lpwszFileName = item.ip.c_str();

		#if !defined(__APPLE__) && !defined(__FreeBSD__)				
		pi->FindData.dwFileAttributes = (item.valid.type && item.type == RTN_UNICAST) ? FILE_ATTRIBUTE_EXECUTABLE:0;
		#else
		pi->FindData.dwFileAttributes = 0;
		#endif

		pi->FindData.nFileSize = 0;

		PluginUserData * user_data = (PluginUserData *)malloc(sizeof(PluginUserData));
		if( user_data ) {
			user_data->size = sizeof(PluginUserData);
			user_data->data.arp = const_cast<ArpRouteInfo*>(&item);
			pi->Flags = PPIF_USERDATA;
			pi->UserData = (DWORD_PTR)user_data;
		}

		const wchar_t ** CustomColumnData = (const wchar_t **)malloc(ArpRoutesColumnMaxIndex*sizeof(const wchar_t *));
		if( CustomColumnData ) {
			memset(CustomColumnData, 0, ArpRoutesColumnMaxIndex*sizeof(const wchar_t *));
			CustomColumnData[ArpRoutesColumnMacIndex] = wcsdup(item.valid.mac ? item.mac.c_str():empty_string);
			CustomColumnData[ArpRoutesColumnDevIndex] = wcsdup(item.valid.iface ? item.iface.c_str():empty_string);
			#if !defined(__APPLE__) && !defined(__FreeBSD__)				
			CustomColumnData[ArpRoutesColumnTypeIndex] = wcsdup(towstr(rttype(item.type)).c_str());
			CustomColumnData[ArpRoutesColumnStateIndex] = wcsdup(towstr(ndmsgstate(item.state)).c_str());
			#else
			if( item.valid.flags )
				CustomColumnData[ArpRoutesColumnFlagsIndex] = wcsdup(towstr(RouteFlagsToString(item.flags, 0)).c_str());
			else
				CustomColumnData[ArpRoutesColumnFlagsIndex] = wcsdup(empty_string);
			CustomColumnData[ArpRoutesColumnStateIndex] = wcsdup(empty_string);
			#endif
			pi->CustomColumnNumber = ArpRoutesColumnMaxIndex;
			pi->CustomColumnData = CustomColumnData;
		}
		pi++;
	}
	return int(true);
}
