#include "netcfginterfaces.h"

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

#if !defined(__APPLE__) && !defined(__FreeBSD__)				
#include <common/netlink.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <memory>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "netcfginterfaces.cpp"

NetcfgInterfaces::NetcfgInterfaces(PanelIndex index):FarPanel(index)
{
	LOG_INFO("\n");
	nifs = std::make_unique<NetInterfaces>();
	change = true;
}

NetcfgInterfaces::~NetcfgInterfaces()
{
	LOG_INFO("\n");
}

int NetcfgInterfaces::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	LOG_INFO("\n");
	if( change && nifs->Update() ) {
		nifs->Log();
		change = false;
	}

	*pItemsNumber = nifs->size();
	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));

	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;

	std::wstring _tmp;
	for( const auto& [name, net_if] : *nifs ) {
		
		pi->FindData.lpwszFileName = name.c_str();

		pi->FindData.dwFileAttributes = net_if->IsCarrierOn() ? FILE_ATTRIBUTE_EXECUTABLE:(FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_HIDDEN);
		pi->FindData.nFileSize = net_if->recv_bytes+net_if->send_bytes;

		PluginUserData * user_data = (PluginUserData *)malloc(sizeof(PluginUserData));
		if( user_data ) {
			user_data->size = sizeof(PluginUserData);
			assert( net_if->size == sizeof(NetInterface) );
			user_data->data.net_if = net_if;
			pi->Flags = PPIF_USERDATA;
			pi->UserData = (DWORD_PTR)user_data;
		}

		const wchar_t ** CustomColumnData = (const wchar_t **)malloc(InterfaceColumnMaxIndex*sizeof(const wchar_t *));
		memset(CustomColumnData, 0, InterfaceColumnMaxIndex*sizeof(const wchar_t *));


		_tmp.clear();
		for( const auto& [ip, ipinfo] : net_if->ip ) {
			if( !_tmp.empty() )
				_tmp += L",";
			_tmp += ip;
		}

		CustomColumnData[InterfaceColumnIpIndex] = wcsdup(_tmp.c_str());


		for( const auto& [mac, macinfo] : net_if->mac ) {
			CustomColumnData[InterfaceColumnMacIndex] = wcsdup(mac.c_str());
			break;
		}
		if( CustomColumnData[InterfaceColumnMacIndex] == 0 )
			CustomColumnData[InterfaceColumnMacIndex] = wcsdup(L"");

		_tmp.clear();
		for( const auto& [ip, ipinfo] : net_if->ip6 ) {
			if( !_tmp.empty() )
				_tmp += L",";
			_tmp += ip;
		}
		CustomColumnData[InterfaceColumnIp6Index] = wcsdup(_tmp.c_str());

		CustomColumnData[InterfaceColumnMtuIndex] = DublicateCountString(net_if->mtu);

		CustomColumnData[InterfaceColumnRecvBytesIndex] = DublicateFileSizeString(net_if->recv_bytes);
		CustomColumnData[InterfaceColumnSendBytesIndex] = DublicateFileSizeString(net_if->send_bytes);

		CustomColumnData[InterfaceColumnRecvPktsIndex] = DublicateCountString(net_if->recv_packets);
		CustomColumnData[InterfaceColumnSendPktsIndex] = DublicateCountString(net_if->send_packets);
		CustomColumnData[InterfaceColumnRecvErrsIndex] = DublicateCountString(net_if->recv_errors);
		CustomColumnData[InterfaceColumnSendErrsIndex] = DublicateCountString(net_if->send_errors);
		CustomColumnData[InterfaceColumnMulticastIndex] = DublicateCountString(net_if->multicast);
		CustomColumnData[InterfaceColumnCollisionsIndex] = DublicateCountString(net_if->collisions);

		CustomColumnData[InterfaceColumnPermanentMacIndex] = wcsdup(net_if->permanent_mac.c_str());

		pi->CustomColumnData = CustomColumnData;
		pi->CustomColumnNumber = InterfaceColumnMaxIndex;
		pi++;
	}
	return int(true);
}

const int DIALOG_WIDTH = 78;

enum {
	WinCaptionIndex,
	WinNameTextIndex,
	WinIfNameEditIndex,
	WinMACNameTextIndex,
	WinIfMACEditIndex,
	WinPermMACTextIndex,
	WinUpFlagIndex,
	WinCarrierTextIndex,
	WinCarrierOnOffTextIndex,
	WinMulticastFlagIndex,
	WinAllMulticastFlagIndex,
	WinPromiscFlagIndex,
	WinNoARPFlagIndex,
	WinMtuTextIndex,
	WinMtuEditIndex,
	WinIfFlagsIndex,
	WinMasterTextIndex,
	WinMasterButtonIndex,
	WinPrefixMax
};

enum {
	WinIpTextIndex,
	WinIpEditIndex,
	WinIpMaskTextIndex,
	WinIpMaskEditIndex,
	WinIpBroadcastTextIndex,
	WinIpBroadcastEditIndex,
	WinIpPointToPointTextIndex = WinIpBroadcastTextIndex,
	WinIpPointToPointEditIndex = WinIpBroadcastEditIndex,
	WinIpMaxIndex
};

enum {
	WinIp6TextIndex,
	WinIp6EditIndex,
	WinIp6MaskTextIndex,
	WinIp6MaskEditIndex,
	WinIp6MaxIndex
};


enum {
	WinStatSeparatorIndex,
	WinStatRecvSendCaptionTextIndex,
	WinStatRecvTextIndex,
	WinStatRecvBytesIndex,
	WinStatRecvPacketsIndex,
	WinStatRecvErrorsIndex,
	WinStatMulticastTextIndex,
	WinStatMulticastIndex,
	WinStatSendTextIndex,
	WinStatSendBytesIndex,
	WinStatSendPacketsIndex,
	WinStatSendErrorsIndex,
	WinStatCollisionsTextIndex,
	WinStatCollisionsIndex,
	WinStatMaxIndex
};

enum {
	WinFooterSeparatorIndex,
	WinFooterOkIndex,
	WinFooterCancelIndex,
};

LONG_PTR WINAPI SettingInterfaceDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgInterfaces * netCfg = 0;

	LOG_INFO("MSG: 0x%08X\n", msg);

	switch( msg ) {

	case DN_INITDIALOG:
		netCfg = (NetcfgInterfaces *)param2;
		break;
	case DM_CLOSE:
		netCfg = 0;
		break;
	case DN_BTNCLICK:
		if( param1 == WinMasterButtonIndex ) {
			int elementSet[1] = {WinMasterButtonIndex};
			assert( netCfg != 0 );
			netCfg->SelectInterface(hDlg, elementSet, ARRAYSIZE(elementSet), true);
			return true;
		}
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

bool NetcfgInterfaces::EditInterface(NetInterface * net_if)
{
	assert( net_if->size == sizeof(NetInterface) );

	bool change = false;

	static const DlgConstructorItem prefix[] = {
		//  Type       NewLine       X1              X2        Flags          PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, DIALOG_WIDTH-4,   0,            {0}},

		{DI_TEXT,      true,   5,              0,                0,            {.lngIdstring = MIfConfigName}},
		{DI_EDIT,      false,  11,             27,               0,            {0}},
		{DI_TEXT,      false,  29,             0,                0,            {.ptrData = L"MAC:"}},
		{DI_EDIT,      false,  34,             51,               0,            {0}},
		{DI_TEXT,      false,  53,             0,                0,            {0}},

		{DI_CHECKBOX,  true,   6,              0,                0,            {.ptrData = L"UP"}},
		{DI_TEXT,      false,  13,             0,                0,            {.ptrData = L"carrier:"}},
		{DI_TEXT,      false,  22,             0,                0,            {0}},
		{DI_CHECKBOX,  false,  31,             0,                0,            {.ptrData = L"MULTICAST"}},
		{DI_CHECKBOX,  false,  56,             0,                0,            {.ptrData = L"ALLMULTICAST"}},

		{DI_CHECKBOX,  true,   6,              0,                0,            {.ptrData = L"PROMISC"}},
		{DI_CHECKBOX,  false,  31,             0,                0,            {.ptrData = L"NOARP(no MAC)"}},
		{DI_TEXT,      false,  56,             0,                0,            {.ptrData = L"mtu:"}},
		{DI_EDIT,      false,  61,            71,                0,            {0}},


		{DI_TEXT,      true,   5,              0,                0,            {0}},
		{DI_TEXT,      false,  46,             0,                0,            {.ptrData = L"master:"}},
		{DI_BUTTON,    false,  54,             0,                0,            {0}},


		{DI_TEXT,      true,   5,             0,    DIF_BOXCOLOR|DIF_SEPARATOR, {0}},

		{DI_ENDDIALOG, 0}
	};

	static const  DlgConstructorItem suffix[] = {
		{DI_TEXT,     true,   5,   0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_BUTTON,   true,   0,   0, DIF_CENTERGROUP, {.lngIdstring = MOk}},
		{DI_BUTTON,   false,  0,   0, DIF_CENTERGROUP, {.lngIdstring = MCancel}},
		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&prefix[0]);

	// Fill prefix
	std::wstring caption(GetMsg(MIfConfigTitle));
	caption += net_if->name;
	caption += L" ";
	fdc.SetText(WinCaptionIndex, caption.c_str());

	fdc.SetText(WinIfNameEditIndex, net_if->name.c_str());

#if defined(__APPLE__)
	fdc.OrFlags(WinIfNameEditIndex, DIF_DISABLE);

	fdc.OrFlags(WinMulticastFlagIndex, DIF_DISABLE);
	fdc.OrFlags(WinAllMulticastFlagIndex, DIF_DISABLE);
	fdc.OrFlags(WinNoARPFlagIndex, DIF_DISABLE);
	fdc.OrFlags(WinMasterTextIndex, DIF_HIDDEN);
	fdc.OrFlags(WinMasterButtonIndex, DIF_HIDDEN);
#else
	fdc.SetSelected(WinMulticastFlagIndex, net_if->IsMulticast());
	fdc.SetSelected(WinAllMulticastFlagIndex, net_if->IsAllmulticast());
	fdc.SetSelected(WinNoARPFlagIndex, net_if->IsNoARP());

	if( net_if->osdep.valid.master )
		if( auto ifmaster = nifs->FindByIndex(net_if->osdep.master) )
			fdc.SetText(WinMasterButtonIndex, ifmaster->name.c_str() );
#endif

	fdc.SetText(WinIfMACEditIndex, net_if->mac.begin()->first.c_str());

	std::wstring permanent_mac(L"(");
	permanent_mac += net_if->permanent_mac;
	permanent_mac += L")";

	fdc.SetText(WinPermMACTextIndex, permanent_mac.c_str());

	if( net_if->permanent_mac.empty() ) {
		fdc.OrFlags(WinMACNameTextIndex, DIF_HIDDEN);
		fdc.OrFlags(WinIfMACEditIndex, DIF_HIDDEN);
		fdc.OrFlags(WinPermMACTextIndex, DIF_HIDDEN);
	}

	std::wstring flags(L"Other flags: ");

	fdc.SetSelected(WinUpFlagIndex, net_if->IsUp());
	fdc.OrFlags(WinUpFlagIndex, DIF_SETCOLOR | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | (net_if->IsUp() ? FOREGROUND_GREEN:FOREGROUND_RED));

	fdc.SetText(WinCarrierOnOffTextIndex, net_if->IsCarrierOn() ? L"ON":L"OFF");
	fdc.OrFlags(WinCarrierOnOffTextIndex, DIF_SETCOLOR | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | (net_if->IsCarrierOn() ? FOREGROUND_GREEN:FOREGROUND_RED) );

	fdc.SetSelected(WinPromiscFlagIndex, net_if->IsPromisc());

	flags += net_if->IsLoopback() ? L"LOOPBACK, ":L"";
	flags += net_if->IsBroadcast() ? L"BROADCAST, ":L"";
	flags += net_if->IsPointToPoint() ? L"POINTOPOINT, ":L"";
	flags += net_if->IsNotrailers() ? L"NOTRAILERS, ":L"";
	flags += net_if->IsDormant() ? L"DORMANT, ":L"";
	flags += net_if->IsEcho() ? L"ECHO, ":L"";

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	flags += net_if->IsMaster() ? L"MASTER, ":L"";
	flags += net_if->IsSlave() ? L"SLAVE, ":L"";
	#endif

	wchar_t max32[sizeof("0xFFFFFFFF")] = {0};
	if( NetCfgPlugin::FSF.snprintf(max32, ARRAYSIZE(max32), L"0x%08X", net_if->ifa_flags) > 0 ) {
		flags += L"(";
		flags += max32;
		flags += L")";
	}

	fdc.SetText(WinIfFlagsIndex, flags.c_str());

	fdc.SetCountText(WinMtuEditIndex, net_if->mtu);

	unsigned int offIp = fdc.GetNumberOfItems();

	static const DlgConstructorItem ip_template[] = {
		//  Type       NewLine X1              X2              Flags           PtrData
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"ip:"}},
		{DI_EDIT,      false,  9,             24,                0,            {0}},
		{DI_TEXT,      false,  26,             0,                0,            {.ptrData = L"mask:"}},
		{DI_EDIT,      false,  32,            47,                0,            {0}},
		{DI_TEXT,      false,  49,             0,                0,            {0}},
		{DI_EDIT,      false,  57,            72,                0,            {0}},
		{DI_ENDDIALOG, 0}
	};

	for( const auto& [ip, ipinfo] : net_if->ip ) {
		unsigned int off = fdc.GetNumberOfItems();
		fdc.AppendItems(&ip_template[0]);

		fdc.SetText(WinIpEditIndex+off, ip.c_str());
		fdc.SetText(WinIpMaskEditIndex+off, ipinfo.netmask.c_str());

		if( net_if->IsBroadcast() ) {
			fdc.SetText(WinIpBroadcastTextIndex+off, L"brcast:");
			fdc.SetText(WinIpBroadcastEditIndex+off, ipinfo.broadcast.c_str());
		} else if(net_if->IsPointToPoint()) {
			fdc.SetText(WinIpPointToPointTextIndex+off, L"pt2pt:");
			fdc.SetText(WinIpPointToPointEditIndex+off, ipinfo.point_to_point.c_str());
		} else {
			fdc.OrFlags(WinIpBroadcastTextIndex+off, DIF_HIDDEN);
			fdc.OrFlags(WinIpBroadcastEditIndex+off, DIF_HIDDEN);
		}
	}

	unsigned int offAddIp = fdc.GetNumberOfItems();
	fdc.AppendItems(&ip_template[0]);
	if( net_if->IsBroadcast() )
		fdc.SetText(WinIpBroadcastTextIndex+offAddIp, L"brcast:");
	else if(net_if->IsPointToPoint())
		fdc.SetText(WinIpPointToPointTextIndex+offAddIp, L"pt2pt:");
	else {
		fdc.OrFlags(WinIpBroadcastTextIndex+offAddIp, DIF_HIDDEN);
		fdc.OrFlags(WinIpBroadcastEditIndex+offAddIp, DIF_HIDDEN);
	}

	static const DlgConstructorItem separotor_template = {DI_TEXT, true, 5, 0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}};

	unsigned int offIp6 = fdc.Append(&separotor_template) + 1;

	static const DlgConstructorItem ip6_template[] = {
		//  Type       NewLine X1              X2              Flags           PtrData
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"ip6:"}},
		{DI_EDIT,      false,  10,            39,                0,            {0}},
		{DI_TEXT,      false,  41,             0,                0,            {.ptrData = L"mask6(use /xx):"}},
		{DI_EDIT,      false,  57,            62,                0,            {0}},
		{DI_ENDDIALOG, 0}
	};

	for( const auto& [ip, ipinfo] : net_if->ip6 ) {
		unsigned int off = fdc.GetNumberOfItems();
		fdc.AppendItems(&ip6_template[0]);
		fdc.SetText(WinIp6EditIndex+off, ip.c_str());
		fdc.SetText(WinIp6MaskEditIndex+off, ipinfo.netmask.c_str());
	}

	unsigned int offAddIp6 = fdc.GetNumberOfItems();
	unsigned int offStats = fdc.AppendItems(&ip6_template[0]);

	static const DlgConstructorItem stats_template[] = {
		//  Type       NewLine X1              X2              Flags           PtrData
		{DI_TEXT,      true,   5,              0,  DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_TEXT,      true,   13,             0,                0,            {.ptrData = L"bytes:       packets:      errors:"}},
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"recv:"}},
		{DI_TEXT,      false,  11,            20,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  24,            35,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  37,            48,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  50,             0,                0,            {.ptrData = L"multicast:"}},
		{DI_TEXT,      false,  62,            72,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"send:"}},
		{DI_TEXT,      false,  11,            20,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  24,            35,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  37,            48,       DIF_CENTERTEXT,        {0}},
		{DI_TEXT,      false,  50,             0,                0,            {.ptrData = L"collisions:"}},
		{DI_TEXT,      false,  62,            72,       DIF_CENTERTEXT,        {0}},
		{DI_ENDDIALOG, 0}
	};

	unsigned int offSuffix = fdc.AppendItems(&stats_template[0]);

	fdc.SetFileSizeText(offStats+WinStatRecvBytesIndex, net_if->recv_bytes);
	fdc.SetCountText(offStats+WinStatRecvPacketsIndex, net_if->recv_packets);
	fdc.SetCountText(offStats+WinStatRecvErrorsIndex, net_if->recv_errors);
	fdc.SetCountText(offStats+WinStatMulticastIndex, net_if->multicast);
	fdc.SetFileSizeText(offStats+WinStatSendBytesIndex, net_if->send_bytes);
	fdc.SetCountText(offStats+WinStatSendPacketsIndex, net_if->send_packets);
	fdc.SetCountText(offStats+WinStatSendErrorsIndex, net_if->send_errors);
	fdc.SetCountText(offStats+WinStatCollisionsIndex, net_if->collisions);

	fdc.AppendItems(&suffix[0]);

	fdc.SetFocus(WinIfNameEditIndex);
	fdc.SetDefaultButton(offSuffix+WinFooterOkIndex);

	fdc.SetHelpTopic(L"InterfacePanel");

	FarDialog dlg(&fdc, SettingInterfaceDialogProc, (LONG_PTR)this);
	unsigned int result = (unsigned int)dlg.Run();

	if( result - offSuffix == WinFooterOkIndex ) {

		std::vector<ItemChange> chlst;
		if( !dlg.CreateChangeList(chlst) )
			return false;

		for( auto & item : chlst ) {
			switch(item.itemNum) {
			case WinIfNameEditIndex:
				change |= net_if->SetInterfaceName(item.newVal.ptrData);
				break;
			case WinIfMACEditIndex:
				change |= net_if->SetMac(item.newVal.ptrData);
				break;
			case WinUpFlagIndex:
				change |= net_if->SetUp(item.newVal.Selected);
				break;
			case WinMulticastFlagIndex:
				change |= net_if->SetMulticast(item.newVal.Selected);
				break;
			case WinAllMulticastFlagIndex:
				change |= net_if->SetAllmulticast(item.newVal.Selected);
				break;
			case WinPromiscFlagIndex:
				change |= net_if->SetPromisc(item.newVal.Selected);
				break;
			case WinNoARPFlagIndex:
				change |= net_if->SetNoARP(item.newVal.Selected);
				break;
			case WinMtuEditIndex:
				change |= net_if->SetMtu(NetCfgPlugin::FSF.atoi(item.newVal.ptrData));
				break;
			default:
				if( item.itemNum >= offIp6 ) {

					uint32_t rindex = (item.itemNum - offIp6)/WinIp6MaxIndex;
					uint32_t base = rindex*WinIp6MaxIndex+offIp6;
					uint32_t iindex = (item.itemNum - offIp6)%WinIp6MaxIndex;

					LOG_INFO("IP6: item.itemNum %d, offIp6 %d, offAddIp6 %d, base %d, rindex %d, iindex %d\n", item.itemNum, offIp6, offAddIp6, base, rindex, iindex);

					if( iindex == WinIp6EditIndex &&
						base != offAddIp6 &&
						wcslen(item.newVal.ptrData) == 0 ) {
						change |= net_if->DeleteIp6(item.oldVal.ptrData);
						break;
					}

					const wchar_t * ip = dlg.GetConstText(base+WinIp6EditIndex);
					const wchar_t * mask = dlg.GetConstText(base+WinIp6MaskEditIndex);

					if( base == offAddIp6 ) {
	        				change |= net_if->AddIp6(ip, mask);
						break;
					}

					change |= net_if->ChangeIp6(fdc.GetConstText(base+WinIp6EditIndex), ip, mask);

				} else if( item.itemNum >= offIp ) {

					uint32_t rindex = (item.itemNum - offIp)/WinIpMaxIndex;
					uint32_t base = rindex*WinIpMaxIndex+offIp;
					uint32_t iindex = (item.itemNum - offIp)%WinIpMaxIndex;

					if( iindex == WinIpEditIndex &&
						item.itemNum != (offAddIp + WinIpEditIndex) &&
						wcslen(item.newVal.ptrData) == 0 ) {
						change |= net_if->DeleteIp(item.oldVal.ptrData);
						break;
					}

					const wchar_t * ip = dlg.GetConstText(base+WinIpEditIndex);
					const wchar_t * mask = dlg.GetConstText(base+WinIpMaskEditIndex);
					const wchar_t * bcip = dlg.GetConstText(base+WinIpBroadcastEditIndex);

					if( base == offAddIp ) {
	        				change |= net_if->AddIp(ip, mask, bcip);
						break;
					}

					change |= net_if->ChangeIp(fdc.GetConstText(base+WinIpEditIndex), ip, mask, bcip);
				}
				break;
			}
		}
	}
	return change;
}

enum {
	WinTcpCaptionIndex,
	WinTcpInterfaceTextIndex,
	WinTcpInterfaceButtonIndex,
	WinTcpSpaceIndex,
	WinTcpPromiscCheckboxIndex,
	WinTcpFileTextIndex,
	WinTcpFileEditIndex,
	WinTcpSeparatorIndex,
	WinTcpOkButtonIndex,
	WinTcpCancelButtonIndex,
	WinTcpDumpMaxIndex
};

static void SetCapFilePath(HANDLE hDlg, int item, const wchar_t * capname)
{
	int nameSize = NetCfgPlugin::psi.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)0);
	auto path = std::make_unique<wchar_t[]>(nameSize);
	NetCfgPlugin::psi.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, nameSize, (LONG_PTR)path.get());
	std::wstring _file(path.get());
	if( _file.empty() )
		_file = L"/tmp";
	_file += L"/";
	_file += capname;
	_file += L".cap";
	NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, item, (LONG_PTR)_file.c_str());
	return;
}

void NetcfgInterfaces::SelectInterface(HANDLE hDlg, int * elementSet, size_t total, bool emptyEntry)
{
	int if_count = nifs->size() + (emptyEntry ? 1:0), index = 0;
	LOG_INFO("if_count %u nifs->size() %u emptyEntry %u\n", if_count, nifs->size(), emptyEntry);
	//int if_count = nifs->size(), index = 0;
	auto menuElements = std::make_unique<FarMenuItem[]>(if_count);
	memset(menuElements.get(), 0, sizeof(FarMenuItem)*if_count);
	for( const auto& [name, net_if] : *nifs ) {
		menuElements[index].Text = name.c_str();
		index++;
	}

	if( emptyEntry )
		menuElements[index].Text = L"";

	menuElements[0].Selected = 1;
	index = NetCfgPlugin::psi.Menu(NetCfgPlugin::psi.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, \
			 L"Select interface:", 0, L"hrd", nullptr, nullptr, menuElements.get(), if_count);

	if( index >= 0 && index < if_count ) {
		if( total >= 1 ) 
			NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, elementSet[0], (LONG_PTR)menuElements[index].Text);
		if( total == 2 )
			SetCapFilePath(hDlg, elementSet[1], menuElements[index].Text);
	}
	return;
}

LONG_PTR WINAPI SettingTcpdumpDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgInterfaces * netCfg = 0;

	LOG_INFO("MSG: 0x%08X\n", msg);

	switch( msg ) {

	case DN_INITDIALOG:
		netCfg = (NetcfgInterfaces *)param2;
		break;
	case DM_CLOSE:
		netCfg = 0;
		break;
	case DN_BTNCLICK:
		if( param1 == WinTcpInterfaceButtonIndex ) {
			int elementSet[2] = {WinTcpInterfaceButtonIndex, WinTcpFileEditIndex};
			assert( netCfg != 0 );
			netCfg->SelectInterface(hDlg, elementSet, ARRAYSIZE(elementSet));
			return true;
		}
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

bool NetcfgInterfaces::TcpdumpInterface(NetInterface * net_if)
{
	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine       X1              X2        Flags          PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, DIALOG_WIDTH-4,   0,            {.ptrData = L"Tcpdump:"}},
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"Interface:"}},
		{DI_BUTTON,    false,  16,             0,                0,            {0}},
		{DI_CHECKBOX,  false,  62,             0,                0,            {.ptrData = L"PROMISC"}},
		{DI_TEXT,      true,   5,              0,                0,            {0}},
		{DI_TEXT,      true,   5,              0,                0,            {.ptrData = L"File:"}},
		{DI_EDIT,      false,  11,             72,               0,            {0}},
		{DI_TEXT,      true,   5,              0,  DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_BUTTON,    true,   0,              0,         DIF_CENTERGROUP,     {.lngIdstring = MOk}},
		{DI_BUTTON,    false,  0,              0,         DIF_CENTERGROUP,     {.lngIdstring = MCancel}},
		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	fdc.SetHelpTopic(L"Tcpdump");
	//fdc.SetMaxTextLen(WinTcpInterfaceButtonIndex, MAX_INTERFACE_NAME_LEN);
	fdc.SetText(WinTcpInterfaceButtonIndex, net_if->name.c_str());
	fdc.SetDefaultButton(WinTcpOkButtonIndex);
	fdc.SetFocus(WinTcpOkButtonIndex);

	FarDialog dlg(&fdc, SettingTcpdumpDialogProc, (LONG_PTR)this);

	SetCapFilePath(dlg.GetDlg(), WinTcpFileEditIndex, net_if->name.c_str());

	if( dlg.Run() == WinTcpOkButtonIndex ) {
		auto interface = dlg.GetText(WinTcpInterfaceButtonIndex);
		if( nifs->find(interface) != nifs->end() ) {
			nifs->TcpDumpStart(interface.c_str(), dlg.GetConstText(WinTcpFileEditIndex), dlg.GetCheck(WinTcpPromiscCheckboxIndex));
		}
	}
	return true;
}

bool NetcfgInterfaces::TcpdumpStop(void)
{
	int if_count = nifs->tcpdump.size(), index = 0;
	auto menuElements = std::make_unique<FarMenuItem[]>(if_count);
	memset(menuElements.get(), 0, sizeof(FarMenuItem)*if_count);
	for( const auto & name : nifs->tcpdump ) {
		menuElements[index].Text = name.c_str();
		index++;
	}
	if( index ) {
		menuElements[0].Selected = 1;
		index = NetCfgPlugin::psi.Menu(NetCfgPlugin::psi.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, \
					 L"Stop tcpdump on interface:", 0, L"hrd", nullptr, nullptr, menuElements.get(), index);
		if( index >= 0 && index < if_count )
			nifs->TcpDumpStop(menuElements[index].Text);
	} else {
		const wchar_t * msgItems[] = { L"Tcpdump: ", GetMsg(MNoAnyTcpdumpTask), GetMsg(MOk) };
		NetCfgPlugin::psi.Message(NetCfgPlugin::psi.ModuleNumber, 0, NULL, msgItems, ARRAYSIZE(msgItems), 1);
	}
	return false;
}

int NetcfgInterfaces::ProcessKey(HANDLE hPlugin,int key,unsigned int controlState, bool & redraw)
{
	if( controlState == 0 ) {
		switch( key ) {
		case VK_F1:
			NetCfgPlugin::psi.ShowHelp(NetCfgPlugin::psi.ModuleName, L"InterfacePanel", FHELP_USECONTENTS|FHELP_NOSHOWERROR);
			return TRUE;
		case VK_F2:
			change = true;
			redraw = true;
			return TRUE;
		case VK_F4:
		case VK_F5:
			if( auto ppi = GetCurrentPanelItem() ) {
				if( 	NetCfgPlugin::FSF.LStricmp(ppi->FindData.lpwszFileName, L"..") != 0 && \
					ppi->Flags & PPIF_USERDATA && \
					ppi->UserData && \
					((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData) ) {
					auto net_if = ((PluginUserData *)ppi->UserData)->data.net_if;
					redraw = change = key == VK_F4 ? EditInterface(net_if):TcpdumpInterface(net_if);
				}
				FreePanelItem(ppi);
			}
			return TRUE;
		case VK_F6:
			redraw = change = TcpdumpStop();
			return TRUE;
		}
	}

	return IsPanelProcessKey(key, controlState);
}
