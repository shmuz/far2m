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
#define LOG_SOURCE_FILE "netcfgiproutes.cpp"

#if !defined(__APPLE__) && !defined(__FreeBSD__)
NetcfgIpRoute::NetcfgIpRoute(uint32_t index_, uint8_t family_, std::deque<IpRouteInfo> & inet_, std::deque<RuleRouteInfo> & rule_, std::map<uint32_t, std::wstring> & ifs_):
	NetFarPanel(index_, ifs_),
	inet(inet_),
	ifs(ifs_),
	family(family_)
{

	table = 0;
	rt = nullptr;

	rule = std::make_unique<NetcfgIpRule>(RouteRuleInetPanelIndex, family_, rule_, ifs_);
	tables = std::make_unique<NetcfgTablesRoute>(RouteIpTablesPanelIndex, inet_);

	panel = PanelTables;

	LOG_INFO("index_ %u this %p\n", index_, this);
}
#else
NetcfgIpRoute::NetcfgIpRoute(uint32_t index_, uint8_t family_, std::deque<IpRouteInfo> & inet_, std::map<uint32_t, std::wstring> & ifs_):
	NetFarPanel(index_, ifs_),
	inet(inet_),
	ifs(ifs_),
	family(family_)
{
	rt = nullptr;
	LOG_INFO("index_ %u this %p\n", index_, this);
}
#endif

NetcfgIpRoute::~NetcfgIpRoute()
{
	LOG_INFO("\n");
}

const int DIALOG_WIDTH = 80;

bool NetcfgIpRoute::DeleteIpRoute(void)
{
	LOG_INFO("\n");
	bool change = false;

	PanelInfo pi = {0};
	GetPanelInfo(pi);

	do {
		if( !pi.SelectedItemsNumber )
			break;

		FarDlgConstructor fdc(L"Delete route(s):", DIALOG_WIDTH);
		fdc.SetHelpTopic(L"DeleteRoute");

		std::vector<IpRouteInfo *> delroutes;

		static const DlgConstructorItem route = {DI_TEXT, true, 5, 0, 0, {0}};

		for( int i = 0; i < pi.SelectedItemsNumber; i++ ) {
			auto ppi = GetSelectedPanelItem(i);
			if( ppi && !(ppi->FindData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) && ppi->Flags & PPIF_USERDATA ) {

        			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));

				auto rt = ((PluginUserData *)ppi->UserData)->data.inet;
				delroutes.push_back(rt);

				auto off = fdc.Append(&route);
				std::wstring _rt(std::to_wstring(i+1)+L". ");
				_rt += rt->destIpandMask.empty() ? L"default":rt->destIpandMask;
				if( !rt->gateway.empty() )
					_rt += L" via " + rt->gateway;
				if( !rt->iface.empty() )
					_rt += L" dev " + rt->iface;

				#if !defined(__APPLE__) && !defined(__FreeBSD__)
				if( rt->osdep.table )
					_rt += L" table " + towstr(rtruletable(rt->osdep.table));
				#endif

				if( _rt.size() > (DIALOG_WIDTH - 10) ) {
					_rt.resize(DIALOG_WIDTH - 13);
					_rt += L"...";
				}

				fdc.SetText(off, _rt.c_str(), true);
			}
			FreePanelItem(ppi);
		}

		if( delroutes.size() == 0 )
			break;

		auto offSufix = fdc.AppendOkCancel();

		fdc.SetDefaultButton(offSufix + WinSuffixOkIndex);
		fdc.SetFocus(offSufix + WinSuffixOkIndex);

		FarDialog dlg(&fdc);

		if( dlg.Run() != static_cast<int>(offSufix) + WinSuffixOkIndex )
			break;

		for( auto & rt : delroutes )
			change |= rt->DeleteIpRoute();

	} while(0);

	return change;
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
const int EDIT_ENCAP_DIALOG_WIDTH = 80;

enum {
	WinEditIpEncapCaptionTextIndex,
	WinEditIpEncapTypeTextIndex,
	WinEditIpEncapTypeButtonIndex,

	WinEditIpEncapIpIdTextIndex,
	WinEditIpEncapIpIdEditIndex,
	WinEditIpEncapIpTTLTextIndex,
	WinEditIpEncapIpTTLEditIndex,
	WinEditIpEncapIpTosTextIndex,
	WinEditIpEncapIpTosEditIndex,
	WinEditIpEncapIpFlagsTextIndex,

	WinEditIpEncapMPLSLabelTextIndex,
	WinEditIpEncapMPLSLabelEditIndex,
	WinEditIpEncapMPLSTTLTextIndex,
	WinEditIpEncapMPLSTTLEditIndex,

	WinEditIpEncapSeparatorTextIndex,
	WinEditIpEncapIpSrcTextIndex,
	WinEditIpEncapIpSrcEditIndex,
	WinEditIpEncapIpDstTextIndex,
	WinEditIpEncapIpDstEditIndex,
	WinEditIpEncapIpCsumCheckBoxIndex,
	WinEditIpEncapIpKeyCheckBoxIndex,
	WinEditIpEncapIpSeqCheckBoxIndex,

	WinEditIpEncapSeparator2TextIndex,

	WinEditIpEncapIpOptsTextIndex,
	WinEditIpEncapIpOptsButtonIndex,

	WinEditIpEncapIpOptsGeneveEditIndex,
	WinEditIpEncapIpOptsVxlanEditIndex,
	WinEditIpEncapIpOptsErspanEditIndex,

	WinEditIpEncapMaxIndex
};

LONG_PTR WINAPI EditEncapDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgIpRoute * rtCfg = 0;

	switch( msg ) {
	case DN_INITDIALOG:
		rtCfg = (NetcfgIpRoute *)param2;
		break;
	case DM_CLOSE:
		rtCfg = 0;
		break;
	case DN_BTNCLICK:
		assert( rtCfg != 0 );
		switch( param1 ) {
		case WinEditIpEncapTypeButtonIndex:
		{
			static const wchar_t * menuElements[] = {  // enum lwtunnel_encap_types {    
				L"",		                   // 	LWTUNNEL_ENCAP_NONE,      
				L"mpls",	                   // 	LWTUNNEL_ENCAP_MPLS,      
				L"ip",		                   // 	LWTUNNEL_ENCAP_IP,        
				L"ila",		                   // 	LWTUNNEL_ENCAP_ILA,       
				L"ip6",		                   // 	LWTUNNEL_ENCAP_IP6,       
				L"seg6",	                   //   LWTUNNEL_ENCAP_SEG6,      
				L"bpf",                            //   LWTUNNEL_ENCAP_BPF,       
				L"seg6local",                      //   LWTUNNEL_ENCAP_SEG6_LOCAL,
				L"rpl",                            //   LWTUNNEL_ENCAP_RPL,       
				L"ioam6",                          //   LWTUNNEL_ENCAP_IOAM6,     
				L"xfrm",                           //   LWTUNNEL_ENCAP_XFRM,      
				L"Enter:"                          //   __LWTUNNEL_ENCAP_MAX,     
				};                                 //   };                           
			auto index = rtCfg->SelectNum(hDlg, &menuElements[0], ARRAYSIZE(menuElements), L"Encap type:", WinEditIpEncapTypeButtonIndex);
			if( index < 0 )
				return true;
			switch( index ) {
			case LWTUNNEL_ENCAP_MPLS:
				HideDialogItems(hDlg, WinEditIpEncapIpIdTextIndex, WinEditIpEncapIpFlagsTextIndex);
				HideDialogItems(hDlg, WinEditIpEncapSeparatorTextIndex, WinEditIpEncapIpOptsErspanEditIndex);
				UnhideDialogItems(hDlg, WinEditIpEncapMPLSLabelTextIndex, WinEditIpEncapMPLSTTLEditIndex);
				break;
			case LWTUNNEL_ENCAP_IP:
			case LWTUNNEL_ENCAP_IP6:
				HideDialogItems(hDlg, WinEditIpEncapMPLSLabelTextIndex, WinEditIpEncapMPLSTTLEditIndex);
				UnhideDialogItems(hDlg, WinEditIpEncapIpIdTextIndex, WinEditIpEncapIpFlagsTextIndex);
				UnhideDialogItems(hDlg, WinEditIpEncapSeparatorTextIndex, WinEditIpEncapIpOptsErspanEditIndex);
				break;
			default:
				HideDialogItems(hDlg, WinEditIpEncapIpIdTextIndex, WinEditIpEncapIpOptsErspanEditIndex);
				break;
			};
			return true;
		}
		break;
		case WinEditIpEncapIpOptsButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"geneve_opts",
				L"vxlan_opts",
				L"erspan_opts",
				L""
				};

			auto index = rtCfg->Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), WinEditIpEncapIpOptsButtonIndex);
			if( index < 0 )
				return true;
			HideDialogItems(hDlg, WinEditIpEncapIpOptsGeneveEditIndex, WinEditIpEncapIpOptsErspanEditIndex);
			if( index < 3 )
				UnhideDialogItems(hDlg, index+WinEditIpEncapIpOptsGeneveEditIndex, index+WinEditIpEncapIpOptsGeneveEditIndex);
			return true;
		}
		};
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

static uint16_t GetEncapType( const wchar_t * type )
{
	static const wchar_t * elements[] = {  // enum lwtunnel_encap_types {    
		L"",		                   // 	LWTUNNEL_ENCAP_NONE,      
		L"mpls",	                   // 	LWTUNNEL_ENCAP_MPLS,      
		L"ip",		                   // 	LWTUNNEL_ENCAP_IP,        
		L"ila",		                   // 	LWTUNNEL_ENCAP_ILA,       
		L"ip6",		                   // 	LWTUNNEL_ENCAP_IP6,       
		L"seg6",	                   //   LWTUNNEL_ENCAP_SEG6,      
		L"bpf",                            //   LWTUNNEL_ENCAP_BPF,       
		L"seg6local",                      //   LWTUNNEL_ENCAP_SEG6_LOCAL,
		L"rpl",                            //   LWTUNNEL_ENCAP_RPL,       
		L"ioam6",                          //   LWTUNNEL_ENCAP_IOAM6,     
		L"xfrm"                            //   LWTUNNEL_ENCAP_XFRM,      
		};                                 //   };

	for(uint16_t i = 0; i < ARRAYSIZE(elements); i++ )
		if( NetCfgPlugin::FSF.LStricmp(type, elements[i]) == 0 )
			return i;
	return 0;
}

bool NetcfgIpRoute::SelectEncap(HANDLE hDlg, Encap & enc)
{
	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_ENCAP_DIALOG_WIDTH-4,  0,     {.ptrData = L"Encap:"}},
		{DI_TEXT,      true,   5,              0,           0,                    {.ptrData = L"type:"}},
		{DI_BUTTON,    false,  11,             0,           0,                    {0}},

		{DI_TEXT,      false,  19,             0,           0,                    {.ptrData = L"id:"}},
		{DI_EDIT,      false,  23,            38,           DIF_LEFTTEXT,         {0}},
		{DI_TEXT,      false,  41,             0,           0,                    {.ptrData = L"ttl:"}},
		{DI_EDIT,      false,  46,            49,           DIF_LEFTTEXT,         {0}},
		{DI_TEXT,      false,  51,             0,           0,                    {.ptrData = L"tos:"}},
		{DI_EDIT,      false,  56,            59,           DIF_LEFTTEXT,         {0}},
		{DI_TEXT,      false,  61,             0,           0,                    {0}},

		{DI_TEXT,      false,  20,             0,           0,                    {.ptrData = L"label:"}},
		{DI_EDIT,      false,  27,            44,           DIF_LEFTTEXT,         {0}},
		{DI_TEXT,      false,  46,             0,           0,                    {.ptrData = L"ttl:"}},
		{DI_EDIT,      false,  51,            54,           DIF_LEFTTEXT,         {.ptrData = 0}},

		{DI_TEXT,      true,    5,             0,           0,                    {0}},
		{DI_TEXT,      true,    5,             0,           0,                    {.ptrData = L"src:"}},
		{DI_EDIT,      false,  10,            26,           DIF_LEFTTEXT,         {0}},
		{DI_TEXT,      false,  28,             0,           0,                    {.ptrData = L"dst:"}},
		{DI_EDIT,      false,  33,            49,           DIF_LEFTTEXT,         {0}},

		{DI_CHECKBOX,  false,  51,             0,            0,                   {.ptrData = L"csum"}},
		{DI_CHECKBOX,  false,  60,             0,            0,                   {.ptrData = L"key"}},
		{DI_CHECKBOX,  false,  68,             0,            0,                   {.ptrData = L"seq"}},

		{DI_TEXT,      true,    5,             0,           0,                    {0}},
		{DI_TEXT,      true,    5,             0,           0,                    {.ptrData = L"opts:"}},
		{DI_BUTTON,    false,  11,             0,           0,                    {0}},

		{DI_EDIT,      false,  27,             74,          DIF_LEFTTEXT,                    {0}},
		{DI_EDIT,      false,  27,             74,          DIF_LEFTTEXT,                    {0}},
		{DI_EDIT,      false,  27,             74,          DIF_LEFTTEXT,                    {0}},

		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	fdc.SetText(WinEditIpEncapTypeButtonIndex, towstr(lwtunnelencaptype(enc.type)).c_str(), true);
	switch( enc.type ) {
	case LWTUNNEL_ENCAP_MPLS:

		fdc.HideItems(WinEditIpEncapIpIdTextIndex, WinEditIpEncapIpFlagsTextIndex);
		fdc.HideItems(WinEditIpEncapSeparatorTextIndex, WinEditIpEncapIpOptsErspanEditIndex);

		if( enc.data.mpls.valid.dst )
			fdc.SetText(WinEditIpEncapMPLSLabelEditIndex, towstr(enc.data.mpls.dst).c_str(), true);
		if( enc.data.mpls.valid.ttl )
			fdc.SetCountText(WinEditIpEncapMPLSTTLEditIndex, enc.data.mpls.ttl);
		break;

	// TODO: this is hack?
	case LWTUNNEL_ENCAP_IP6:
	case LWTUNNEL_ENCAP_IP:
	{
		fdc.HideItems(WinEditIpEncapMPLSLabelTextIndex, WinEditIpEncapMPLSTTLEditIndex);

		if( enc.data.ip.valid.id )
			fdc.SetCountText(WinEditIpEncapIpIdEditIndex, enc.data.ip.id);

		if( enc.data.ip.valid.ttl )
			fdc.SetCountText(WinEditIpEncapIpTTLEditIndex, enc.data.ip.ttl);

		if( enc.data.ip.valid.hoplimit )
			fdc.SetCountText(WinEditIpEncapIpTTLEditIndex, enc.data.ip.hoplimit);

		if( enc.data.ip.valid.tos )
			fdc.SetCountText(WinEditIpEncapIpTosEditIndex, enc.data.ip.tos);

		if( enc.data.ip.valid.tc )
			fdc.SetCountText(WinEditIpEncapIpTosEditIndex, enc.data.ip.tc);

		if( enc.data.ip.valid.src )
			fdc.SetText(WinEditIpEncapIpSrcEditIndex, towstr(enc.data.ip.src).c_str(), true);

		if( enc.data.ip.valid.dst )
			fdc.SetText(WinEditIpEncapIpDstEditIndex, towstr(enc.data.ip.dst).c_str(), true);

		if( enc.data.ip.valid.flags ) {
			std::wstring flags(L"flags:");
			wchar_t max16[sizeof("0xFFFF")] = {0};
			if( NetCfgPlugin::FSF.snprintf(max16, ARRAYSIZE(max16), L"0x%04X", enc.data.ip.flags) > 0 ) {
				flags += L"(";
				flags += max16;
				flags += L")";
			}
			fdc.SetText(WinEditIpEncapIpFlagsTextIndex, flags.c_str(), true);
			fdc.SetSelected(WinEditIpEncapIpCsumCheckBoxIndex, (enc.data.ip.flags & TUNNEL_CSUM) != 0);
			fdc.SetSelected(WinEditIpEncapIpKeyCheckBoxIndex, (enc.data.ip.flags & TUNNEL_KEY) != 0);
			fdc.SetSelected(WinEditIpEncapIpSeqCheckBoxIndex, (enc.data.ip.flags & TUNNEL_SEQ) != 0);

			if( enc.data.ip.flags & TUNNEL_GENEVE_OPT && enc.data.ip.valid.geneve ) {
				fdc.OrFlags(WinEditIpEncapIpOptsVxlanEditIndex, DIF_HIDDEN);
				fdc.OrFlags(WinEditIpEncapIpOptsErspanEditIndex, DIF_HIDDEN);
				fdc.SetText(WinEditIpEncapIpOptsButtonIndex, L"geneve_opts");
				fdc.SetText(WinEditIpEncapIpOptsGeneveEditIndex, towstr(enc.data.ip.geneve_opts).c_str(), true);
			} else if( enc.data.ip.flags & TUNNEL_VXLAN_OPT && enc.data.ip.valid.vxlan ) {
				fdc.OrFlags(WinEditIpEncapIpOptsGeneveEditIndex, DIF_HIDDEN);
				fdc.OrFlags(WinEditIpEncapIpOptsErspanEditIndex, DIF_HIDDEN);
				fdc.SetText(WinEditIpEncapIpOptsButtonIndex, L"vxlan_opts");
				if( enc.data.ip.valid.vxlan_gbp )
					fdc.SetCountText(WinEditIpEncapIpOptsVxlanEditIndex, enc.data.ip.vxlan_gbp);
			} else if( enc.data.ip.flags & TUNNEL_ERSPAN_OPT && enc.data.ip.valid.erspan ) {
				fdc.OrFlags(WinEditIpEncapIpOptsVxlanEditIndex, DIF_HIDDEN);
				fdc.OrFlags(WinEditIpEncapIpOptsGeneveEditIndex, DIF_HIDDEN);
				fdc.SetText(WinEditIpEncapIpOptsButtonIndex, L"erspan_opts");
				fdc.SetText(WinEditIpEncapIpOptsErspanEditIndex, towstr(enc.data.ip.erspan_opts).c_str(), true);
			}
		}
	}
		break;
	default:
		fdc.HideItems(WinEditIpEncapIpIdTextIndex, WinEditIpEncapIpOptsErspanEditIndex);
		break;
	};

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	FarDialog dlg(&fdc, EditEncapDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			switch( item.itemNum ) {
			case WinEditIpEncapTypeButtonIndex:
				enc.type = GetEncapType(item.newVal.ptrData);
				break;
			case WinEditIpEncapIpIdEditIndex:
				enc.data.ip.valid.id = !item.empty;
				enc.data.ip.id = dlg.GetInt64(item.itemNum);
				break;
			case WinEditIpEncapIpTTLEditIndex:
				if( enc.type == LWTUNNEL_ENCAP_IP ) {
					enc.data.ip.valid.ttl = !item.empty;
					enc.data.ip.ttl = (uint8_t)dlg.GetInt32(item.itemNum);
				} else if( enc.type == LWTUNNEL_ENCAP_IP6 ) {
					enc.data.ip.valid.hoplimit = !item.empty;
					enc.data.ip.hoplimit = (uint8_t)dlg.GetInt32(item.itemNum);
				}
				break;
			case WinEditIpEncapIpTosEditIndex:
				if( enc.type == LWTUNNEL_ENCAP_IP ) {
					enc.data.ip.valid.tos = !item.empty;
					enc.data.ip.tos = (uint8_t)dlg.GetInt32(item.itemNum);
				} else if( enc.type == LWTUNNEL_ENCAP_IP6 ) {
					enc.data.ip.valid.tc = !item.empty;
					enc.data.ip.tc = (uint8_t)dlg.GetInt32(item.itemNum);
				}
				break;
			case WinEditIpEncapMPLSLabelEditIndex:
				{
				enc.data.mpls.valid.dst = !item.empty;
				auto _s = tostr(item.newVal.ptrData);
				memmove(enc.data.mpls.dst, _s.c_str(), std::min(_s.size(),sizeof(enc.data.mpls.dst)));
				}
				break;
			case WinEditIpEncapMPLSTTLEditIndex:
				enc.data.mpls.valid.ttl = !item.empty;
				enc.data.mpls.valid.ttl = (uint8_t)dlg.GetInt32(item.itemNum);
				break;
			case WinEditIpEncapIpSrcEditIndex:
				{
				enc.data.ip.valid.src = !item.empty;
				auto _s = tostr(item.newVal.ptrData);
				memmove(enc.data.ip.src, _s.c_str(), std::min(_s.size(),sizeof(enc.data.ip.src)));
				}
				break;
			case WinEditIpEncapIpDstEditIndex:
				{
				enc.data.ip.valid.dst = !item.empty;
				auto _s = tostr(item.newVal.ptrData);
				memmove(enc.data.ip.dst, _s.c_str(), std::min(_s.size(),sizeof(enc.data.ip.dst)));
				}
				break;
			case WinEditIpEncapIpCsumCheckBoxIndex:
				enc.data.ip.valid.flags = 1;
				if( item.newVal.Selected )
					enc.data.ip.flags |= TUNNEL_CSUM;
				else
					enc.data.ip.flags &= ~TUNNEL_CSUM;
				break;
			case WinEditIpEncapIpKeyCheckBoxIndex:
				enc.data.ip.valid.flags = 1;
				if( item.newVal.Selected )
					enc.data.ip.flags |= TUNNEL_KEY;
				else
					enc.data.ip.flags &= ~TUNNEL_KEY;
				break;
			case WinEditIpEncapIpSeqCheckBoxIndex:
				enc.data.ip.valid.flags = 1;
				if( item.newVal.Selected )
					enc.data.ip.flags |= TUNNEL_SEQ;
				else
					enc.data.ip.flags &= ~TUNNEL_SEQ;
				break;
			case WinEditIpEncapIpOptsButtonIndex:
				enc.data.ip.valid.vxlan = 0;
				enc.data.ip.valid.erspan = 0;
				enc.data.ip.valid.geneve = 0;
				enc.data.ip.flags &= ~(TUNNEL_VXLAN_OPT | TUNNEL_ERSPAN_OPT | TUNNEL_GENEVE_OPT);
				if( NetCfgPlugin::FSF.LStricmp(item.newVal.ptrData, L"geneve_opts") == 0 ) {
					enc.data.ip.valid.geneve = 1;
					enc.data.ip.flags |= TUNNEL_GENEVE_OPT;
				} else if( NetCfgPlugin::FSF.LStricmp(item.newVal.ptrData, L"vxlan_opts") == 0 ) {
					enc.data.ip.valid.vxlan = 1;
					enc.data.ip.flags |= TUNNEL_VXLAN_OPT;
				} else if( NetCfgPlugin::FSF.LStricmp(item.newVal.ptrData, L"erspan_opts") == 0 ) {
					enc.data.ip.valid.erspan = 1;
					enc.data.ip.flags |= TUNNEL_ERSPAN_OPT;
				}
				break;
			case WinEditIpEncapIpOptsGeneveEditIndex:
				{
				auto _s = tostr(item.newVal.ptrData);
				memmove(enc.data.ip.geneve_opts, _s.c_str(), std::min(_s.size(),sizeof(enc.data.ip.geneve_opts)));
				}
				break;
			case WinEditIpEncapIpOptsVxlanEditIndex:
				enc.data.ip.valid.vxlan_gbp = !item.empty;
				enc.data.ip.vxlan_gbp = NetCfgPlugin::FSF.atoi(item.newVal.ptrData);
				break;
			case WinEditIpEncapIpOptsErspanEditIndex:
				{
				auto _s = tostr(item.newVal.ptrData);
				memmove(enc.data.ip.erspan_opts, _s.c_str(), std::min(_s.size(),sizeof(enc.data.ip.erspan_opts)));
				}
				break;
			default:
				break;
			};
		}
	}

	LOG_INFO("enc.type: %u\n", enc.type);
	return true;
}

#endif

const int EDIT_IP_DIALOG_WIDTH = 84;

#if !defined(__APPLE__) && !defined(__FreeBSD__)

enum {
	WinEditIpCaptionIndex,
	WinEditIpFirstHeaderTextIndex,
	WinEditIpTypeButtonIndex,
	WinEditIpDestMaskEditIndex,
	WinEditIpFamilyButtonIndex,
	WinEditIpGatewayEditIndex,
	WinEditIpSeparator1TextIndex,
	WinEditIpDeviceTextIndex,
	WinEditIpDeviceButtonIndex,
	WinEditIpTableButtonIndex,
	WinEditIpScopeButtonIndex,
	WinEditIpProtoButtonIndex,
	WinEditIpPrefButtonIndex,
	WinEditIpMetricEditIndex,
	WinEditIpSeparator2TextIndex,
	WinEditIpSecondHeaderTextIndex,
	WinEditIpPrefsrcEditIndex,
	WinEditIpSrcMaskEditIndex,
	WinEditIpTOSEditIndex,
	WinEditIpOnlinkCheckBoxIndex,
	WinEditIpEncapCheckBoxIndex,
	WinEditIpEncapCheckBoxStoreIndex,
	WinEditIpEncapButtonIndex,
	WinEditIpPrefixMaxIndex
};

enum {
	WinEditIpNextHopeSeparatorIndex,
	WinEditIpNextHopeNextHopeCheckBoxIndex,
	WinEditIpNextHopeNextHopeCheckBoxStoreIndex,
	WinEditIpNextHopeHeaderTextIndex,
	WinEditIpNextHopeOnlinkCheckBoxIndex,
	WinEditIpNextHopeEncapCheckBoxIndex,
	WinEditIpNextHopeEncapButtonIndex,
	WinEditIpNextHopeFamilyButtonIndex,
	WinEditIpNextHopeViaEditIndex,
	WinEditIpNextHopeDeviceButtonIndex,
	WinEditIpNextHopeWeightTextIndex,
	WinEditIpNextHopeWeightEditIndex,
	WinEditIpNextHopeEncapCheckBoxStoreIndex, // Must be more then WinEditIpNextHopeWeightEditIndex
	WinEditIpNextHopeMaxIndex
};

LONG_PTR WINAPI EditIpRouteDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgIpRoute * rtCfg = 0;

	switch( msg ) {
	case DN_INITDIALOG:
		rtCfg = (NetcfgIpRoute *)param2;
		break;
	case DM_CLOSE:
		rtCfg = 0;
		break;
	case DN_DRAWDLGITEM:
	{
		if( param1 > WinEditIpPrefixMaxIndex ) {
			uint32_t rindex = (param1 - WinEditIpPrefixMaxIndex)/WinEditIpNextHopeMaxIndex;
			uint32_t base = rindex*WinEditIpNextHopeMaxIndex+WinEditIpPrefixMaxIndex;
			uint32_t iindex = (param1 - WinEditIpPrefixMaxIndex)%WinEditIpNextHopeMaxIndex;

			if( rindex <= rtCfg->new_rt.osdep.nhs.size() && iindex == WinEditIpNextHopeNextHopeCheckBoxIndex ) {
				bool prev = bool(NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_GETCHECK, base+WinEditIpNextHopeNextHopeCheckBoxStoreIndex, 0));
				if( ShowHideElements(hDlg,
					base+WinEditIpNextHopeNextHopeCheckBoxIndex,
					base+WinEditIpNextHopeNextHopeCheckBoxStoreIndex,
					base+WinEditIpNextHopeHeaderTextIndex,
					base+WinEditIpNextHopeWeightEditIndex) ) {

					if( !prev ) {
						NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETCHECK, base+WinEditIpNextHopeEncapCheckBoxStoreIndex, true);
					}
					ShowHideElements(hDlg,
						base+WinEditIpNextHopeEncapCheckBoxIndex,
						base+WinEditIpNextHopeEncapCheckBoxStoreIndex,
						base+WinEditIpNextHopeEncapButtonIndex,
						base+WinEditIpNextHopeEncapButtonIndex);
				}
			}
		} else if( param1 == WinEditIpEncapCheckBoxIndex ) {
			ShowHideElements(hDlg,
				WinEditIpEncapCheckBoxIndex,
				WinEditIpEncapCheckBoxStoreIndex,
				WinEditIpEncapButtonIndex,
				WinEditIpEncapButtonIndex);
		}
		break;
	}
	case DN_BTNCLICK:
		assert( rtCfg != 0 );

		if( param1 < WinEditIpPrefixMaxIndex ) {
		switch( param1 ) {
		case WinEditIpDeviceButtonIndex:
			rtCfg->new_rt.valid.ifnameIndex = rtCfg->SelectInterface(hDlg, param1, &rtCfg->new_rt.ifnameIndex);
			return true;
		case WinEditIpTypeButtonIndex:
		{
		static const wchar_t * menuElements[] = {
				L"unspec",
				L"unicast",
				L"local",
				L"broadcast",
				L"anycast",
				L"multicast",
				L"blackhole",
				L"unreachable",
				L"prohibit",
				L"throw",
				L"nat"
				};
			auto type = rtCfg->Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), WinEditIpTypeButtonIndex);
			if( type >= 0 ) {
				rtCfg->new_rt.osdep.type = type;
				rtCfg->new_rt.valid.type = rtCfg->new_rt.osdep.type != 0;
			}
			return true;
		}
		case WinEditIpFamilyButtonIndex:
			rtCfg->new_rt.sa_family = rtCfg->SelectFamily(hDlg, WinEditIpFamilyButtonIndex);
			return true;
		case WinEditIpTableButtonIndex:
			rtCfg->new_rt.osdep.table = rtCfg->SelectTable(hDlg, param1, rtCfg->new_rt.osdep.table);
			rtCfg->new_rt.valid.table = rtCfg->new_rt.osdep.table != 0;
			return true;
		case WinEditIpScopeButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"host",
				L"link",
				L"global",
				L"Enter:"
				};
			rtCfg->new_rt.valid.scope = 1;
			switch( rtCfg->SelectNum(hDlg, &menuElements[0], ARRAYSIZE(menuElements), L"Scope:", WinEditIpScopeButtonIndex) ) {
			case 0:
				rtCfg->new_rt.osdep.scope = RT_SCOPE_HOST;
				break;
			case 1:
				rtCfg->new_rt.osdep.scope = RT_SCOPE_LINK;
				break;
			case 2:
				rtCfg->new_rt.osdep.scope = RT_SCOPE_UNIVERSE;
				break;
			default:
				rtCfg->new_rt.osdep.scope = (uint8_t)GetNumberItem(hDlg, WinEditIpScopeButtonIndex);
				break;

			};
			return true;
		}
		case WinEditIpProtoButtonIndex:
			rtCfg->new_rt.osdep.protocol = rtCfg->SelectProto(hDlg, param1, rtCfg->new_rt.osdep.protocol);
			rtCfg->new_rt.valid.protocol = rtCfg->new_rt.osdep.protocol != 0;
			return true;
		case WinEditIpPrefButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"medium",
				L"low",
				L"high"
				};
			rtCfg->new_rt.valid.icmp6pref = 1;
			switch( rtCfg->Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), WinEditIpPrefButtonIndex) ) {
			case 0:
				rtCfg->new_rt.osdep.icmp6pref = ICMPV6_ROUTER_PREF_MEDIUM;
				break;
			case 1:
				rtCfg->new_rt.osdep.icmp6pref = ICMPV6_ROUTER_PREF_LOW;
				break;
			case 2:
				rtCfg->new_rt.osdep.icmp6pref = ICMPV6_ROUTER_PREF_HIGH;
				break;
			};
			return true;
		}
		case WinEditIpEncapButtonIndex:
			rtCfg->SelectEncap(hDlg, rtCfg->new_rt.osdep.enc);
			return true;
		};
		} else {
			uint32_t rindex = (param1 - WinEditIpPrefixMaxIndex)/WinEditIpNextHopeMaxIndex;
			uint32_t base = rindex*WinEditIpNextHopeMaxIndex+WinEditIpPrefixMaxIndex;
			uint32_t iindex = (param1 - WinEditIpPrefixMaxIndex)%WinEditIpNextHopeMaxIndex;

			// active if 0 or real items + 1
			if( rindex <= rtCfg->new_rt.osdep.nhs.size() ) {
			auto & nh = rindex < rtCfg->new_rt.osdep.nhs.size() ? rtCfg->new_rt.osdep.nhs[rindex]:rtCfg->new_nh;
			switch( iindex ) {
			case WinEditIpNextHopeEncapButtonIndex:
				rtCfg->SelectEncap(hDlg, nh.enc);
				nh.valid.encap = nh.enc.type != LWTUNNEL_ENCAP_NONE;
				break;
			case WinEditIpNextHopeFamilyButtonIndex:
				nh.rtvia_family = rtCfg->SelectFamily(hDlg, base + WinEditIpNextHopeFamilyButtonIndex);
				break;
			case WinEditIpNextHopeDeviceButtonIndex:
				nh.valid.iface = rtCfg->SelectInterface(hDlg, param1, &nh.ifindex);
				if( nh.valid.iface )
					nh.iface = rtCfg->GetInterfaceByIndex(nh.ifindex);
				break;
			};
			return true;
			}
		}
		break;
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

void NetcfgIpRoute::FillNextHope(uint32_t iindex, ItemChange & item, NextHope & nh)
{
	switch(iindex) {
	case WinEditIpNextHopeNextHopeCheckBoxIndex:
		nh.valid.nexthope = (bool)item.newVal.Selected;
		break;
	case WinEditIpNextHopeOnlinkCheckBoxIndex:
		if( item.newVal.Selected )
			nh.flags |= RTNH_F_ONLINK;
		else
			nh.flags &= ~(RTNH_F_ONLINK);
		break;
	case WinEditIpNextHopeEncapCheckBoxIndex:
		nh.valid.encap = (bool)item.newVal.Selected;
		break;
	case WinEditIpNextHopeViaEditIndex:
		nh.rtvia_addr = item.newVal.ptrData;
		nh.valid.rtvia = !nh.rtvia_addr.empty();
		break;
	case WinEditIpNextHopeWeightEditIndex:
		nh.weight = (uint8_t)NetCfgPlugin::FSF.atoi(item.newVal.ptrData);
		break;
	};
}

bool NetcfgIpRoute::FillNewIpRoute(void)
{
	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_IP_DIALOG_WIDTH-4,  0,     {.ptrData = L"Edit route:"}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"type:           destination/mask:        via(fam):  via(gateway):"}},
		{DI_BUTTON,    true,   5,              0,           0,                 {.ptrData = 0}},
		{DI_EDIT,      false,  21,            44,           DIF_LEFTTEXT,      {.ptrData = 0}},
		{DI_BUTTON,    false,  46,             0,           0,                 {.ptrData = 0}},
		{DI_EDIT,      false,  57,            78,           DIF_LEFTTEXT,      {.ptrData = 0}},
		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"device:             table:      scope:     proto:       pref:      metric:"}},
		{DI_BUTTON,    true,   5,              0,           DIF_CENTERTEXT,    {.ptrData = 0}},
		{DI_BUTTON,    false,  25,             0,           0,                 {.ptrData = 0}},
		{DI_BUTTON,    false,  37,             0,           0,                 {.ptrData = 0}},
		{DI_BUTTON,    false,  48,             0,           0,                 {.ptrData = 0}},
		{DI_BUTTON,    false,  61,             0,           0,                 {.ptrData = 0}},
		{DI_EDIT,      false,  72,            78,           DIF_LEFTTEXT,      {.ptrData = 0}},

		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"prefsrc:            source/mask:             tos:"}},
		{DI_EDIT,      true,   5,             23,           DIF_LEFTTEXT,      {.ptrData = 0}},
		{DI_EDIT,      false,  25,            48,           DIF_LEFTTEXT,      {.ptrData = 0}},
		{DI_EDIT,      false,  50,            53,           DIF_LEFTTEXT,      {.ptrData = 0}},

		{DI_CHECKBOX,  false,  55,            0,            0,                 {.ptrData = L"onlink"}},
		{DI_CHECKBOX,  false,  66,            0,            0,                 {0}},
	/*Store*/ {DI_CHECKBOX,  false,  80,            0,            DIF_HIDDEN,        {0}},
		{DI_BUTTON,    false,  69,            0,            0,                 {.ptrData = L"encap"}},
		{DI_ENDDIALOG, 0}
	};

	static const DlgConstructorItem nexthop[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_TEXT,      true,   5,             0,            DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_CHECKBOX,  true,   5,             0,            0,                 {.ptrData = L"nexthop"}},
		{DI_CHECKBOX,  false,  80,            0,            DIF_HIDDEN,        {0}},
		{DI_TEXT,      false,  17,            0,            0,                 {.ptrData = L"via(gateway):            device:"}},
		{DI_CHECKBOX,  false,  55,            0,            0,                 {.ptrData = L"onlink"}},
		{DI_CHECKBOX,  false,  66,            0,            0,                 {0}},
		{DI_BUTTON,    false,  69,            0,            0,                 {.ptrData = L"encap"}},
		{DI_BUTTON,    true,   5,             0,            0,                 {0}},
		{DI_EDIT,      false,  17,           40,            DIF_LEFTTEXT,      {0}},
		{DI_BUTTON,    false,  42,            0,            DIF_CENTERTEXT,    {0}},
		{DI_TEXT,      false,  62,            0,            0,                 {.ptrData = L"weight:"}},
		{DI_EDIT,      false,  70,           77,            DIF_LEFTTEXT,      {0}},
	/*Store*/ {DI_CHECKBOX,  false,  80,            0,            DIF_HIDDEN,        {0}},
		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	fdc.SetText(WinEditIpDestMaskEditIndex, new_rt.valid.destIpandMask ? new_rt.destIpandMask.c_str():L"default");

	if( new_rt.valid.iface )
		fdc.SetText(WinEditIpDeviceButtonIndex, new_rt.iface.c_str());

	if( new_rt.valid.prefsrc )
		fdc.SetText(WinEditIpPrefsrcEditIndex, new_rt.prefsrc.c_str());

	if( new_rt.valid.fromsrcIpandMask )
		fdc.SetText(WinEditIpSrcMaskEditIndex, new_rt.osdep.fromsrcIpandMask.c_str());
	if( new_rt.valid.rtvia )
		fdc.SetText(WinEditIpGatewayEditIndex, new_rt.osdep.rtvia_addr.c_str());
	else if( new_rt.valid.gateway )
		fdc.SetText(WinEditIpGatewayEditIndex, new_rt.gateway.c_str());

	fdc.SetText(WinEditIpFamilyButtonIndex, towstr(ipfamilyname(new_rt.sa_family)).c_str(), true);

	fdc.SetText(WinEditIpTypeButtonIndex, new_rt.valid.type ? towstr(rttype(new_rt.osdep.type)).c_str():0, true);
	fdc.SetText(WinEditIpTableButtonIndex, new_rt.valid.table ? towstr(rtruletable(new_rt.osdep.table)).c_str():0, true);
	fdc.SetText(WinEditIpScopeButtonIndex, new_rt.valid.scope ? towstr(rtscopetype(new_rt.osdep.scope)).c_str():0, true);
	fdc.SetText(WinEditIpProtoButtonIndex, new_rt.valid.protocol ? towstr(rtprotocoltype(new_rt.osdep.protocol)).c_str():0, true);

	if( new_rt.sa_family == AF_INET )
		fdc.OrFlags(WinEditIpPrefButtonIndex, DIF_DISABLE);
	fdc.SetText(WinEditIpPrefButtonIndex, new_rt.valid.icmp6pref ? towstr(rticmp6pref(new_rt.osdep.icmp6pref)).c_str():0, true);

	if( new_rt.valid.metric )
		fdc.SetCountText(WinEditIpMetricEditIndex, new_rt.osdep.metric);

	if( new_rt.valid.tos && new_rt.osdep.tos )
		fdc.SetCountText(WinEditIpTOSEditIndex, new_rt.osdep.tos);

	if( !new_rt.valid.encap )
		fdc.OrFlags(WinEditIpEncapButtonIndex, DIF_DISABLE);
	else
		fdc.SetSelected(WinEditIpEncapCheckBoxIndex, true);

	if( new_rt.valid.flags && new_rt.flags & RTNH_F_ONLINK )
		fdc.SetSelected(WinEditIpOnlinkCheckBoxIndex, true);

	for( auto & item: new_rt.osdep.nhs ) {
		auto off = fdc.GetNumberOfItems();
		fdc.AppendItems(&nexthop[0]);
		fdc.SetSelected(off+WinEditIpNextHopeNextHopeCheckBoxIndex, true);
		fdc.SetSelected(off+WinEditIpNextHopeOnlinkCheckBoxIndex, (item.flags & RTNH_F_ONLINK) != 0);
		fdc.SetText(off+WinEditIpNextHopeFamilyButtonIndex, item.valid.rtvia ? towstr(ipfamilyname(item.rtvia_family)).c_str():0, true);
		fdc.SetText(off+WinEditIpNextHopeViaEditIndex, item.valid.rtvia ? item.rtvia_addr.c_str():item.gateway.c_str());
		auto it = ifs.find(item.ifindex);
		if( it != ifs.end() )
			fdc.SetText(off+WinEditIpNextHopeDeviceButtonIndex, it->second.c_str());
		fdc.SetCountText(off+WinEditIpNextHopeWeightEditIndex, item.weight);

		if( !item.valid.encap )
			fdc.OrFlags(off+WinEditIpNextHopeEncapButtonIndex, DIF_DISABLE);
		else
			fdc.SetSelected(off+WinEditIpNextHopeEncapCheckBoxIndex, true);
	}

	auto offNewNextHope = fdc.GetNumberOfItems();
	fdc.AppendItems(&nexthop[0]);

	for(int i = WinEditIpNextHopeHeaderTextIndex; i < WinEditIpNextHopeMaxIndex; i++ )
		fdc.OrFlags(offNewNextHope+i, DIF_DISABLE);

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	fdc.SetHelpTopic(L"RoutePanel");

	FarDialog dlg(&fdc, EditIpRouteDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			if( item.itemNum >= WinEditIpPrefixMaxIndex ) {
				uint32_t rindex = (item.itemNum - WinEditIpPrefixMaxIndex)/WinEditIpNextHopeMaxIndex;
				uint32_t iindex = (item.itemNum - WinEditIpPrefixMaxIndex)%WinEditIpNextHopeMaxIndex;

				if( rindex == new_rt.osdep.nhs.size() )
					FillNextHope(iindex, item, new_nh);
				else
					FillNextHope(iindex, item, new_rt.osdep.nhs[rindex]);
			} else {
				switch( item.itemNum ) {
				case WinEditIpDestMaskEditIndex:
					new_rt.destIpandMask = item.newVal.ptrData;
					new_rt.valid.destIpandMask = !item.empty;
					break;
				case WinEditIpGatewayEditIndex:
					new_rt.gateway = item.newVal.ptrData;
					new_rt.valid.gateway = !item.empty;
					break;
				case WinEditIpDeviceButtonIndex:
					new_rt.iface = item.newVal.ptrData;
					new_rt.valid.iface = !item.empty;
					break;
				case WinEditIpMetricEditIndex:
					new_rt.osdep.metric = NetCfgPlugin::FSF.atoi(item.newVal.ptrData);
					new_rt.valid.metric = !item.empty;
					break;
				case WinEditIpPrefsrcEditIndex:
					new_rt.prefsrc = item.newVal.ptrData;
					new_rt.valid.prefsrc = !item.empty;
					break;
				case WinEditIpSrcMaskEditIndex:
					new_rt.osdep.fromsrcIpandMask = item.newVal.ptrData;
					new_rt.valid.fromsrcIpandMask = !item.empty;
					break;
				case WinEditIpTOSEditIndex:
					new_rt.osdep.tos = (uint8_t)NetCfgPlugin::FSF.atoi(item.newVal.ptrData);
					new_rt.valid.tos = !item.empty;
					break;
				case WinEditIpOnlinkCheckBoxIndex:
					if( item.newVal.Selected )
						new_rt.flags |= RTNH_F_ONLINK;
					else
						new_rt.flags &= ~RTNH_F_ONLINK;
					new_rt.valid.flags = 1;
					break;
				case WinEditIpEncapCheckBoxIndex:
					new_rt.valid.encap = (bool)item.newVal.Selected;
					break;
				default:
					break;
				};
			}
		}
		if( new_nh.valid.nexthope ) {
			new_rt.osdep.nhs.push_back(new_nh);
			new_rt.valid.rtnexthop = 1;
		}
	}
	return change;
}

#else

enum {
	WinEditIpCaptionIndex,
	WinEditIpDestinationMaskTextIndex,
	WinEditIpDestMaskEditIndex,

	WinEditIpSeparator1TextIndex,

	WinEditIpGatewayIpRadiobuttonIndex,
	WinEditIpGatewayMacRadiobuttonIndex,
	WinEditIpGatewayInterfaceRadiobuttonIndex,

	WinEditIpGatewayIpEditIndex,
	WinEditIpGatewayMacEditIndex,
	WinEditIpGatewayInterfaceButtonIndex,

	WinEditIpSeparator2TextIndex,
	WinEditIpPrefsrcTextIndex,
	WinEditIpPrefsrcEditIndex,
	WinEditIpExpireTextIndex,
	WinEditIpExpireEditIndex,

	WinEditIpSeparator3TextIndex,

	WinEditIpIfscopeTextIndex,
	WinEditIpIfscopeButtonIndex,

	WinEditIpNormalRadiobuttonIndex,
	WinEditIpRejectRadiobuttonIndex,
	WinEditIpBlackholeRadiobuttonIndex,

	WinEditIpSeparator4TextIndex,
	WinEditIpStaticCheckboxIndex,

	WinEditIpCloningCheckboxIndex,
	WinEditIpProto1CheckboxIndex,
	WinEditIpProto2CheckboxIndex,
	WinEditIpXresolveCheckboxIndex,
	WinEditIpLlinfoCheckboxIndex,

	WinEditIpPrefixMaxIndex
};


LONG_PTR WINAPI EditIpRouteDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgIpRoute * rtCfg = 0;

	switch( msg ) {
	case DN_INITDIALOG:
		rtCfg = (NetcfgIpRoute *)param2;
		break;
	case DM_CLOSE:
		rtCfg = 0;
		break;
	case DN_DRAWDLGITEM:
		break;
	case DN_BTNCLICK:
		assert( rtCfg != 0 );
		switch( param1 ) {
		case WinEditIpGatewayInterfaceButtonIndex:
			rtCfg->new_rt.valid.ifnameIndex = rtCfg->SelectInterface(hDlg, param1, &rtCfg->new_rt.ifnameIndex);
			return true;
		case WinEditIpIfscopeButtonIndex:
			{
			uint32_t scope_index;
			rtCfg->SelectInterface(hDlg, param1, &scope_index);
			return true;
			}
		default:
			break;
		};
		break;
	};
	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

bool NetcfgIpRoute::FillNewIpRoute(void)
{
	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_IP_DIALOG_WIDTH-4,  0,     {.ptrData = L"Edit route:"}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"destination/mask:"}},
		{DI_EDIT,      false,  23,            78,           DIF_LEFTTEXT,      {.ptrData = 0}},

		{DI_TEXT,      true,   5,              0,           0,                 {0}},

		{DI_RADIOBUTTON, true,   5,             0,           DIF_GROUP,         {.ptrData = L"gateway"}},
		{DI_RADIOBUTTON, false, 46,             0,           0,                 {.ptrData = L"mac"}},
		{DI_RADIOBUTTON, false, 66,             0,           0,                 {.ptrData = L"interface"}},
		{DI_EDIT,      true,     5,            44,           0,                 {0}},
		{DI_EDIT,      false,   46,            64,           0,                 {0}},
		{DI_BUTTON,    false,   66,             0,           0,                 {0}},

		{DI_TEXT,      true,    5,              0,           0,                 {0}},
		{DI_TEXT,      true,    5,              0,           0,                 {.ptrData = L"prefsrc:"}},
		{DI_EDIT,      false,  14,             60,           0,                 {.ptrData = 0}},
		{DI_TEXT,      false,  62,              0,           0,                 {.ptrData = L"expire:"}},
		{DI_EDIT,      false,  70,             78,           0,                 {.ptrData = 0}},

		{DI_TEXT,      true,    5,              0,           0,                 {0}},

		{DI_TEXT,      true,    5,              0,           0,                 {.ptrData = L"ifscope:"}},
		{DI_BUTTON,    false,  14,              0,           0,                 {0}},


		{DI_RADIOBUTTON, false,44,              0,           DIF_GROUP,         {.ptrData = L"normal"}},
		{DI_RADIOBUTTON, false,55,              0,           0,                 {.ptrData = L"reject"}},
		{DI_RADIOBUTTON, false,66,              0,           0,                 {.ptrData = L"blackhole"}},


		{DI_TEXT,      true,    5,              0,           0,                 {0}},
		{DI_CHECKBOX,  true,    5,              0,           0,                 {.ptrData = L"static"}},
		{DI_CHECKBOX,  false,  17,              0,           0,                 {.ptrData = L"cloning"}},
		{DI_CHECKBOX,  false,  30,              0,           0,                 {.ptrData = L"proto1"}},
		{DI_CHECKBOX,  false,  42,              0,           0,                 {.ptrData = L"proto2"}},
		{DI_CHECKBOX,  false,  54,              0,           0,                 {.ptrData = L"xresolve"}},
		{DI_CHECKBOX,  false,  67,              0,           0,                 {.ptrData = L"llinfo"}},

		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	fdc.SetText(WinEditIpDestMaskEditIndex, new_rt.valid.destIpandMask ? new_rt.destIpandMask.c_str():L"default");

	if( new_rt.valid.prefsrc )
		fdc.SetText(WinEditIpPrefsrcEditIndex, new_rt.prefsrc.c_str());

	if( new_rt.valid.gateway )
		switch( new_rt.osdep.gateway_type ) {
		case IpRouteInfo::OtherGatewayType:
		case IpRouteInfo::IpGatewayType:
			fdc.SetSelected(WinEditIpGatewayIpRadiobuttonIndex, true);
			fdc.SetText(WinEditIpGatewayIpEditIndex, new_rt.gateway.c_str());
			break;
		case IpRouteInfo::MacGatewayType:
			fdc.SetSelected(WinEditIpGatewayMacRadiobuttonIndex, true);
			fdc.SetText(WinEditIpGatewayMacEditIndex, new_rt.gateway.c_str());
			break;
		case IpRouteInfo::InterfaceGatewayType:
			fdc.SetSelected(WinEditIpGatewayInterfaceRadiobuttonIndex, true);
			fdc.SetText(WinEditIpGatewayInterfaceButtonIndex, new_rt.gateway.c_str());
			break;
		};

	if( new_rt.osdep.expire )
		fdc.SetCountText(WinEditIpExpireEditIndex, new_rt.osdep.expire);


	if( new_rt.valid.flags ) {
		if( new_rt.flags & RTF_BLACKHOLE )
			fdc.SetSelected(WinEditIpBlackholeRadiobuttonIndex, true);
		else if( new_rt.flags & RTF_REJECT )
			fdc.SetSelected(WinEditIpRejectRadiobuttonIndex, true);
		else
			fdc.SetSelected(WinEditIpNormalRadiobuttonIndex, true);
		if( new_rt.flags & RTF_IFSCOPE && new_rt.valid.iface )
			fdc.SetText(WinEditIpIfscopeButtonIndex, new_rt.iface.c_str());
		if( new_rt.flags & RTF_STATIC )
			fdc.SetSelected(WinEditIpStaticCheckboxIndex, true);
		if( new_rt.flags & RTF_CLONING )
			fdc.SetSelected(WinEditIpCloningCheckboxIndex, true);
		if( new_rt.flags & RTF_PROTO1 )
			fdc.SetSelected(WinEditIpProto1CheckboxIndex, true);
		if( new_rt.flags & RTF_PROTO2 )
			fdc.SetSelected(WinEditIpProto2CheckboxIndex, true);
		if( new_rt.flags & RTF_XRESOLVE )
			fdc.SetSelected(WinEditIpXresolveCheckboxIndex, true);
		if( new_rt.flags & RTF_LLINFO )
			fdc.SetSelected(WinEditIpLlinfoCheckboxIndex, true);
	}

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	FarDialog dlg(&fdc, EditIpRouteDialogProc, (LONG_PTR)this);
	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			switch( item.itemNum ) {
			case WinEditIpDestMaskEditIndex:
				new_rt.destIpandMask = item.newVal.ptrData;
				new_rt.valid.destIpandMask = !item.empty;
				break;
			case WinEditIpGatewayIpRadiobuttonIndex:
				if( item.newVal.Selected )
	                                new_rt.osdep.gateway_type = IpRouteInfo::IpGatewayType;
				break;
			case WinEditIpGatewayMacRadiobuttonIndex:
				if( item.newVal.Selected )
	                                new_rt.osdep.gateway_type = IpRouteInfo::MacGatewayType;
				break;
			case WinEditIpGatewayInterfaceRadiobuttonIndex:
				if( item.newVal.Selected )
	                                new_rt.osdep.gateway_type = IpRouteInfo::InterfaceGatewayType;
				break;
			case WinEditIpGatewayIpEditIndex:
				if( new_rt.osdep.gateway_type == IpRouteInfo::IpGatewayType || new_rt.osdep.gateway_type == IpRouteInfo::OtherGatewayType ) {
					new_rt.gateway = item.newVal.ptrData;
					new_rt.valid.gateway = !item.empty;
				}
				break;
			case WinEditIpGatewayMacEditIndex:
				if( new_rt.osdep.gateway_type == IpRouteInfo::MacGatewayType ) {
					new_rt.gateway = item.newVal.ptrData;
					new_rt.valid.gateway = !item.empty;
				}
				break;
			case WinEditIpGatewayInterfaceButtonIndex:
				if( new_rt.osdep.gateway_type == IpRouteInfo::InterfaceGatewayType ) {
					new_rt.gateway = item.newVal.ptrData;
					new_rt.valid.gateway = !item.empty;
				}
				break;
			case WinEditIpIfscopeButtonIndex:
				new_rt.iface = item.newVal.ptrData;
				new_rt.valid.iface = !item.empty;
				if( new_rt.valid.iface )
					new_rt.flags |= RTF_IFSCOPE;
				else
					new_rt.flags &= ~RTF_IFSCOPE;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpExpireEditIndex:
				new_rt.osdep.expire = NetCfgPlugin::FSF.atoi(item.newVal.ptrData);
				new_rt.valid.expire = !item.empty;
				break;
			case WinEditIpPrefsrcEditIndex:
				new_rt.prefsrc = item.newVal.ptrData;
				new_rt.valid.prefsrc = !item.empty;
				break;
			case WinEditIpBlackholeRadiobuttonIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_BLACKHOLE;
				else
					new_rt.flags &= ~RTF_BLACKHOLE;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpRejectRadiobuttonIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_REJECT;
				else
					new_rt.flags &= ~RTF_REJECT;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpNormalRadiobuttonIndex:
				if( item.newVal.Selected )
					new_rt.flags &= ~(RTF_BLACKHOLE|RTF_REJECT);
				new_rt.valid.flags = 1;
				break;
			case WinEditIpStaticCheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_STATIC;
				else
					new_rt.flags &= ~RTF_STATIC;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpCloningCheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_CLONING;
				else
					new_rt.flags &= ~RTF_CLONING;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpProto1CheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_PROTO1;
				else
					new_rt.flags &= ~RTF_PROTO1;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpProto2CheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_PROTO2;
				else
					new_rt.flags &= ~RTF_PROTO2;
				new_rt.valid.flags = 1;
				break;
			case WinEditIpXresolveCheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_XRESOLVE;
				else
					new_rt.flags &= ~RTF_XRESOLVE;
				new_rt.valid.flags = 1;
				break;

			case WinEditIpLlinfoCheckboxIndex:
				if( item.newVal.Selected )
					new_rt.flags |= RTF_LLINFO;
				else
					new_rt.flags &= ~RTF_LLINFO;
				new_rt.valid.flags = 1;
				break;
			default:
				break;
			};
		}
	}
	return change;
}
#endif


bool NetcfgIpRoute::EditIpRoute(void)
{
	LOG_INFO("\n");
	change = false;
	auto ppi = GetCurrentPanelItem();
	if( ppi ) {
		if( ppi->Flags & PPIF_USERDATA ) {
			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));
			rt = ((PluginUserData *)ppi->UserData)->data.inet;
			new_rt = *rt;
			if( FillNewIpRoute() )
				change = rt->ChangeIpRoute(new_rt);
			rt = nullptr;
		}
		FreePanelItem(ppi);
	}
	return change;
}

bool NetcfgIpRoute::CreateIpRoute(void)
{
	LOG_INFO("\n");

	change = false;

	rt = nullptr;
	new_rt = IpRouteInfo();

	new_rt.sa_family = family;

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	new_rt.osdep.type = RTN_UNICAST;
	new_rt.valid.type = 1;

	new_rt.valid.table = 1;
	new_rt.osdep.table = table != 0 ? table:RT_TABLE_MAIN;

	new_nh = NextHope();
	#else
	new_rt.osdep.gateway_type = IpRouteInfo::IpGatewayType;
	#endif
	if( FillNewIpRoute() )
		change = new_rt.CreateIpRoute();
	return change;
}

int NetcfgIpRoute::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change)
{
	if( controlState == 0 && key == VK_F1 ) {
		NetCfgPlugin::psi.ShowHelp(NetCfgPlugin::psi.ModuleName, L"RoutePanel", FHELP_USECONTENTS|FHELP_NOSHOWERROR);
		return TRUE;
	}

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( controlState == 0 && key == VK_F3 ) {
		if( panel == PanelRules ) {
			panel = oldPanel;
		} else {
			oldPanel = panel;
			panel = PanelRules;
		}
		change = true;
		return TRUE;
	}

	switch( panel ) {
	case PanelRules:
		return rule->ProcessKey(hPlugin, key, controlState, change);
	case PanelTables:
		if( key == VK_RETURN ) {
			panel = PanelRoutes;
			table = tables->GetTable();
		}
		return tables->ProcessKey(hPlugin, key, controlState, change);
	default:
		break;
	}
	#endif

	if( controlState == 0 ) {
		switch( key ) {
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		case VK_RETURN:
			if( auto ppi = GetCurrentPanelItem() ) {
				if( NetCfgPlugin::FSF.LStricmp(ppi->FindData.lpwszFileName, L"..") == 0 ) {
					panel = PanelTables;
					tables->SetLastPosition(hPlugin);
				}
				FreePanelItem(ppi);
				return TRUE;
			}
			break;
		#endif
		case VK_F8:
			change = DeleteIpRoute();
			return TRUE;
		case VK_F4:
			change |= EditIpRoute();
			return TRUE;
		}

	}

	if( controlState == PKF_SHIFT && key == VK_F4 ) {
		change |= CreateIpRoute();
		return TRUE;
	}

	return IsPanelProcessKey(key, controlState);
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
void NetcfgIpRoute::GetOpenPluginInfo(struct OpenPluginInfo * info)
{
	if( panel == PanelRules || panel == PanelTables ) {
		if( panel == PanelRules )
			rule->GetOpenPluginInfo(info);
		else
			tables->GetOpenPluginInfo(info);
		info->PanelTitle = GetPanelTitle();
		return;
	}

	FarPanel::GetOpenPluginInfo(info);
}

void NetcfgIpRoute::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber)
{
	if( panel == PanelRules ) {
		rule->FreeFindData(panelItem, itemsNumber);
		return;
	}

	if( panel == PanelTables ) {
		tables->FreeFindData(panelItem, itemsNumber);
		return;
	}

	auto freeNumber = itemsNumber;
	while( freeNumber-- ) {
		if( (panelItem+freeNumber)->FindData.dwFileAttributes & FILE_FLAG_DELETE_ON_CLOSE )
			free((void *)(panelItem+freeNumber)->FindData.lpwszFileName);
	}

	FarPanel::FreeFindData(panelItem, itemsNumber);
}
#endif

int NetcfgIpRoute::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	LOG_INFO("\n");

	const static wchar_t * default_route = L"default";
	const static wchar_t * empty_string = L"";
	*pItemsNumber = inet.size();

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( panel == PanelRules )
		return rule->GetFindData(pPanelItem, pItemsNumber);

	if( panel == PanelTables )
		return tables->GetFindData(pPanelItem, pItemsNumber);

	if( table )
		for( const auto & item : inet ) {
			if( item.osdep.table != table )
				(*pItemsNumber)--;
		}
	#endif

	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));
	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;
	for( const auto & item : inet ) {

		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		if( table && item.osdep.table != table )
			continue;
		#endif

		pi->FindData.lpwszFileName = item.destIpandMask.empty() ? default_route:item.destIpandMask.c_str();
		pi->FindData.dwFileAttributes = 0;
		pi->FindData.nFileSize = 0;

		PluginUserData * user_data = (PluginUserData *)malloc(sizeof(PluginUserData));
		if( user_data ) {
			user_data->size = sizeof(PluginUserData);
			user_data->data.inet = const_cast<IpRouteInfo*>(&item);
			pi->Flags = PPIF_USERDATA;
			pi->UserData = (DWORD_PTR)user_data;
		}

		const wchar_t ** CustomColumnData = (const wchar_t **)malloc(RoutesColumnDataMaxIndex*sizeof(const wchar_t *));
		if( CustomColumnData ) {
			memset(CustomColumnData, 0, RoutesColumnDataMaxIndex*sizeof(const wchar_t *));

			#if !defined(__APPLE__) && !defined(__FreeBSD__)				
			CustomColumnData[RoutesColumnViaIndex] = wcsdup(item.valid.gateway ? item.gateway.c_str(): (item.valid.rtvia ? item.osdep.rtvia_addr.c_str():empty_string));
			#else
			CustomColumnData[RoutesColumnViaIndex] = wcsdup(item.valid.gateway ? item.gateway.c_str():empty_string);
			#endif

			CustomColumnData[RoutesColumnDevIndex] = wcsdup(item.iface.c_str());
			CustomColumnData[RoutesColumnPrefsrcIndex] = wcsdup(item.prefsrc.c_str());
			#if !defined(__APPLE__) && !defined(__FreeBSD__)
			CustomColumnData[RoutesColumnTypeIndex] = wcsdup(item.valid.protocol ? towstr(rtprotocoltype(item.osdep.protocol)).c_str():empty_string);
			CustomColumnData[RoutesColumnMetricIndex] = item.valid.metric ? DublicateCountString(item.osdep.metric):wcsdup(empty_string);
			#else
			if( item.valid.flags )
				CustomColumnData[RoutesColumnFlagsIndex] = wcsdup(towstr(RouteFlagsToString(item.flags, 0)).c_str());
			else
				CustomColumnData[RoutesColumnFlagsIndex] = wcsdup(empty_string);
			CustomColumnData[RoutesColumnMetricIndex] = DublicateCountString(item.osdep.expire);
			#endif
			pi->CustomColumnNumber = RoutesColumnDataMaxIndex;
			pi->CustomColumnData = CustomColumnData;
		}
		pi++;
	}
	return int(true);
}
