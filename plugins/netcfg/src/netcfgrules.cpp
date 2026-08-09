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
#define LOG_SOURCE_FILE "netcfgrules.cpp"

#if !defined(__APPLE__) && !defined(__FreeBSD__)
NetcfgIpRule::NetcfgIpRule(uint32_t index_, uint8_t family_, std::deque<RuleRouteInfo> & rule_, std::map<uint32_t, std::wstring> & ifs_):
	NetFarPanel(index_, ifs_),
	rule(rule_),
	family(family_)
{
	LOG_INFO("\n");
}

NetcfgIpRule::~NetcfgIpRule()
{
	LOG_INFO("\n");
}

const int DIALOG_WIDTH = 80;

bool NetcfgIpRule::DeleteRule(void)
{
	LOG_INFO("\n");

	bool change = false;

	PanelInfo pi = {0};
	GetPanelInfo(pi);

	do {
		if( !pi.SelectedItemsNumber )
			break;

		FarDlgConstructor fdc(L"Delete rule(s):", DIALOG_WIDTH);
		fdc.SetHelpTopic(L"DeleteRule");

		std::vector<RuleRouteInfo *> delrules;

		static const DlgConstructorItem rule_dlg = {DI_TEXT, true, 5, 0, 0, {0}};

		for( int i = 0; i < pi.SelectedItemsNumber; i++ ) {
			auto ppi = GetSelectedPanelItem(i);
			if( ppi && ppi->Flags & PPIF_USERDATA ) {

        			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));

				auto r = ((PluginUserData *)ppi->UserData)->data.rule;
				delrules.push_back(r);

				auto off = fdc.Append(&rule_dlg);
				std::wstring _r(std::to_wstring(i+1)+L". ");

				_r += std::to_wstring(r->priority);
				_r += L": ";
				_r += r->rule;

				if( _r.size() > (DIALOG_WIDTH - 10) ) {
					_r.resize(DIALOG_WIDTH - 13);
					_r += L"...";
				}
				fdc.SetText(off, _r.c_str(), true);
			}
			FreePanelItem(ppi);
		}

		if( delrules.size() == 0 )
			break;

		auto offSufix = fdc.AppendOkCancel();

		fdc.SetDefaultButton(offSufix + WinSuffixOkIndex);
		fdc.SetFocus(offSufix + WinSuffixOkIndex);

		FarDialog dlg(&fdc);

		if( dlg.Run() != static_cast<int>(offSufix) + WinSuffixOkIndex )
			break;

		for( auto & r : delrules )
			change |= r->DeleteRule();

	} while(0);

	return change;
}

const int EDIT_RULE_DIALOG_WIDTH = 85;
enum {
	WinEditRuleCaptionIndex,
	WinEditRulePrefTextIndex,
	WinEditRulePrefEditIndex,

	WinEditRuleNotButtonIndex,

	WinEditRuleFromTextIndex,
	WinEditRuleFromEditIndex,

	WinEditRuleToTextIndex,
	WinEditRuleToEditIndex,

	WinEditRuleSeparator1TextIndex,

	WinEditRuleFwmarkTextIndex,
	WinEditRuleFwmarkEditIndex,

	WinEditRuleFwmaskTextIndex,
	WinEditRuleFwmaskEditIndex,

	WinEditRuleTosTextIndex,
	WinEditRuleTosEditIndex,

	WinEditRuleIpprotoTextIndex,
	WinEditRuleIpprotoButtonIndex,

	WinEditRuleSeparator2TextIndex,

	WinEditRuleIifTextIndex,
	WinEditRuleIifButtonIndex,

	WinEditRuleOifTextIndex,
	WinEditRuleOifButtonIndex,

	WinEditRuleL3mdevCheckboxIndex,

	WinEditRuleTunIdTextIndex,
	WinEditRuleTunIdEditIndex,

	WinEditRuleSeparator3TextIndex,

	WinEditRuleUidrangeTextIndex,
	WinEditRuleUidrangeBeginEditIndex,
	WinEditRuleUidrangeSeparatorTextIndex,
	WinEditRuleUidrangeEndEditIndex,

	WinEditRuleSportTextIndex,
	WinEditRuleSportBeginEditIndex,
	WinEditRuleSportSeparatorTextIndex,
	WinEditRuleSportEndEditIndex,

	WinEditRuleDportTextIndex,
	WinEditRuleDportBeginEditIndex,
	WinEditRuleDportSeparatorTextIndex,
	WinEditRuleDportEndEditIndex,

	WinEditRuleSeparator4TextIndex,

	WinEditRuleTableTextIndex,
	WinEditRuleTableButtonIndex,

	WinEditRuleProtocolTextIndex,
	WinEditRuleProtocolButtonIndex,

	WinEditRuleGotoTextIndex,
	WinEditRuleGotoEditIndex,

	WinEditRuleRealmsTextIndex,
	WinEditRuleRealmsBeginEditIndex,
	WinEditRuleRealmsSeparatorTextIndex,
	WinEditRuleRealmsEndEditIndex,

	WinEditRuleSeparator5TextIndex,

	WinEditRuleSuppressPrefixlengthTextIndex,
	WinEditRuleSuppressPrefixlengthEditIndex,

	WinEditRuleSuppressIfgroupTextIndex,
	WinEditRuleSuppressIfgroupEditIndex,

	WinEditRuleMaxIndex
};

LONG_PTR WINAPI EditRuleDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	static NetcfgIpRule * rtCfg = 0;

	switch( msg ) {

	case DN_INITDIALOG:
		rtCfg = (NetcfgIpRule *)param2;
		break;
	case DM_CLOSE:
		rtCfg = 0;
		break;
	case DN_DRAWDLGITEM:
		break;
	case DN_BTNCLICK:
		assert( rtCfg != 0 );
		switch( param1 ) {
		case WinEditRuleNotButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"",
				L"not"
				};
			auto num = rtCfg->Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), param1);
			if( num == 1 )
				rtCfg->new_r.flags |= FIB_RULE_INVERT;
			else if( num == 0 )
				rtCfg->new_r.flags &= ~FIB_RULE_INVERT;
			return true;
		}
		case WinEditRuleIifButtonIndex:
			rtCfg->new_r.valid.iiface = rtCfg->SelectInterface(hDlg, param1, nullptr);
			rtCfg->new_r.iiface = std::move(GetText(hDlg, param1));
			return true;
		case WinEditRuleOifButtonIndex:
			rtCfg->new_r.valid.oiface = rtCfg->SelectInterface(hDlg, param1, nullptr);
			rtCfg->new_r.oiface = std::move(GetText(hDlg, param1));
			return true;
		case WinEditRuleIpprotoButtonIndex:
		{
			static const wchar_t * menuElements[] = {
				L"",
				L"ip",                /* 0 (IPPROTO_HOPOPTS, IPv6 Hop-by-Hop Option) */
				L"icmp",              /* 1 (IPPROTO_ICMP, Internet Control Message) */
				L"igmp",              /* 2 (IPPROTO_IGMP, Internet Group Management) */
				L"ipencap",           /* 4 (IPPROTO_IPV4, IPv4 encapsulation) */
				L"tcp",               /* 6 (IPPROTO_TCP, Transmission Control) */
				L"udp",               /* 17 (IPPROTO_UDP, User Datagram) */
				L"ipv6",              /* 41 (IPPROTO_IPV6, IPv6 encapsulation) */
				L"gre",               /* 47 (IPPROTO_GRE, Generic Routing Encapsulation) */
				L"ipv6-icmp",          /* 58 (IPPROTO_ICMPV6, ICMP for IPv6) */
				L"Enter:"
				};
			static const uint8_t proto[] = {
				0,
				IPPROTO_HOPOPTS,
				IPPROTO_ICMP,
				IPPROTO_IGMP,
				4,
				IPPROTO_TCP,
				IPPROTO_UDP,
				IPPROTO_IPV6,
				IPPROTO_GRE,
				IPPROTO_ICMPV6};
			auto num = rtCfg->SelectNum(hDlg, &menuElements[0], ARRAYSIZE(menuElements), L"Ip protocol:", param1);
			rtCfg->new_r.valid.ip_protocol = 1;
			if( num == 0 )
				rtCfg->new_r.valid.ip_protocol = 0;
			else if( num >= 0 && num < static_cast<int>(ARRAYSIZE(proto)) )
				rtCfg->new_r.ip_protocol = proto[num];
			else
				rtCfg->new_r.ip_protocol = (uint8_t)GetNumberItem(hDlg, param1);
			return true;
		}
		case WinEditRuleTableButtonIndex:
			rtCfg->new_r.table = rtCfg->SelectTable(hDlg, param1, rtCfg->new_r.table);
			rtCfg->new_r.valid.table = rtCfg->new_r.table != 0;
			if( rtCfg->new_r.valid.table )
				rtCfg->new_r.action = FR_ACT_TO_TBL;
			return true;
		case WinEditRuleProtocolButtonIndex:
			rtCfg->new_r.protocol = rtCfg->SelectProto(hDlg, param1, rtCfg->new_r.valid.protocol);
			rtCfg->new_r.valid.protocol = rtCfg->new_r.protocol != 0;
			return true;
		};
		break;
	};

	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

bool NetcfgIpRule::FillNewIpRule(void)
{
	LOG_INFO("\n");

	bool change = false;

	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_RULE_DIALOG_WIDTH-4,  0,     {.ptrData = L"Edit rule:"}},
		{DI_TEXT,      true,  5,              0,           0,                 {.ptrData = L"pref:"}},
		{DI_EDIT,      false,  11,            16,           DIF_LEFTTEXT,      {0}},
		{DI_BUTTON,    false,  18,             0,           0,                 {0}},
		{DI_TEXT,      false,  26,             0,           0,                 {.ptrData = L"from:"}},
		{DI_EDIT,      false,  32,            52,           DIF_LEFTTEXT,      {0}},
		{DI_TEXT,      false,  54,             0,           0,                 {.ptrData = L"to:"}},
		{DI_EDIT,      false,  58,            78,           DIF_LEFTTEXT,      {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"fwmark/fwmask: 0x"}},
		{DI_EDIT,      false,  22,            30,           DIF_LEFTTEXT,      {0}},
		{DI_TEXT,      false,  31,             0,           0,                 {.ptrData = L"/0x"}},
		{DI_EDIT,      false,  34,            42,           DIF_LEFTTEXT,      {0}},
		{DI_TEXT,      false,  44,             0,           0,                 {.ptrData = L"tos:"}},
		{DI_EDIT,      false,  49,            52,           0,                 {0}},

		{DI_TEXT,      false,  54,             0,           0,                 {.ptrData = L"ipproto:"}},
		{DI_BUTTON,    false,  63,             0,           0,                 {0}},


		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"iif:"}},
		{DI_BUTTON,    false,  10,             0,           0,                 {0}},
		{DI_TEXT,      false,  30,             0,           0,                 {.ptrData = L"oif:"}},
		{DI_BUTTON,    false,  35,             0,           0,                 {0}},
		{DI_CHECKBOX,  false,  55,             0,           0,                 {.ptrData = L"l3mdev"}},

		{DI_TEXT,      false,  66,             0,           0,                 {.ptrData = L"tun_id:"}},
		{DI_EDIT,      false,  74,            79,           0,                 {0}},

		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"uidrange:"}},
		{DI_EDIT,      false,  15,            25,           0,                 {0}},
		{DI_TEXT,      false,  26,             0,           0,                 {.ptrData = L"-"}},
		{DI_EDIT,      false,  27,            37,           0,                 {0}},

		{DI_TEXT,      false,  39,             0,           0,                 {.ptrData = L"sport:"}},
		{DI_EDIT,      false,  46,            51,           0,                 {0}},
		{DI_TEXT,      false,  52,             0,           0,                 {.ptrData = L"-"}},
		{DI_EDIT,      false,  53,            58,           0,                 {0}},

		{DI_TEXT,      false,  60,             0,           0,                 {.ptrData = L"dport:"}},
		{DI_EDIT,      false,  67,            72,           0,                 {0}},
		{DI_TEXT,      false,  73,             0,           0,                 {.ptrData = L"-"}},
		{DI_EDIT,      false,  74,            79,           0,                 {0}},

		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"table:"}},
		{DI_BUTTON,    false,  12,             0,           0,                 {0}},
		{DI_TEXT,      false,  24,             0,           0,                 {.ptrData = L"protocol:"}},
		{DI_BUTTON,    false,  34,             0,           0,                 {0}},

		{DI_TEXT,      false,  47,             0,           0,                 {.ptrData = L"goto:"}},
		{DI_EDIT,      false,  53,            58,           0,                 {0}},

		{DI_TEXT,      false,  60,             0,           0,                 {.ptrData = L"realms:"}},
		{DI_EDIT,      false,  67,            72,           0,                 {0}},
		{DI_TEXT,      false,  73,             0,           0,                 {.ptrData = L"/"}},
		{DI_EDIT,      false,  74,            79,           0,                 {0}},

		{DI_TEXT,      true,   5,              0,           0,                 {0}},
		{DI_TEXT,      true,   5,              0,           0,                 {.ptrData = L"suppress_prefixlength:"}},
		{DI_EDIT,      false,  28,            31,           0,                 {0}},
		{DI_TEXT,      false,  33,             0,           0,                 {.ptrData = L"suppress_ifgroup:"}},
		{DI_EDIT,      false,  51,            54,           0,                 {0}},

		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	if( new_r.valid.priority )
		fdc.SetCountText(WinEditRulePrefEditIndex, new_r.priority);

	if( new_r.flags & FIB_RULE_INVERT )
		fdc.SetText(WinEditRuleNotButtonIndex, L"not");

	if( new_r.valid.fromIpandMask )
		fdc.SetText(WinEditRuleFromEditIndex, new_r.fromIpandMask.c_str());

	if( new_r.valid.toIpandMask )
		fdc.SetText(WinEditRuleToEditIndex, new_r.toIpandMask.c_str());

	if( new_r.valid.fwmark )
		fdc.SetCountHex32Text(WinEditRuleFwmarkEditIndex, new_r.fwmark, false);

	if( new_r.valid.fwmask )
		fdc.SetCountHex32Text(WinEditRuleFwmaskEditIndex, new_r.fwmask);

	if( new_r.tos )
		fdc.SetCountText(WinEditRuleTosEditIndex, new_r.tos);

	if( new_r.valid.ip_protocol )
		fdc.SetText(WinEditRuleIpprotoButtonIndex, towstr(iprotocolname(new_r.ip_protocol)).c_str(), true);

	if( new_r.valid.iiface )
		fdc.SetText(WinEditRuleIifButtonIndex, new_r.iiface.c_str());

	if( new_r.valid.oiface )
		fdc.SetText(WinEditRuleOifButtonIndex, new_r.oiface.c_str());

	if( new_r.valid.l3mdev )
		fdc.SetSelected(WinEditRuleL3mdevCheckboxIndex, (bool)new_r.l3mdev);

	if( new_r.valid.tun_id )
		fdc.SetCountText(WinEditRuleTunIdEditIndex, new_r.tun_id);

	if( new_r.valid.uid_range ) {
		fdc.SetCountText(WinEditRuleUidrangeBeginEditIndex, new_r.uid_range.start);
		fdc.SetCountText(WinEditRuleUidrangeEndEditIndex, new_r.uid_range.end);
	}

	if( new_r.valid.sport_range ) {
		fdc.SetCountText(WinEditRuleSportBeginEditIndex, new_r.sport_range.start);
		fdc.SetCountText(WinEditRuleSportEndEditIndex, new_r.sport_range.end);
	}

	if( new_r.valid.dport_range ) {
		fdc.SetCountText(WinEditRuleDportBeginEditIndex, new_r.dport_range.start);
		fdc.SetCountText(WinEditRuleDportEndEditIndex, new_r.dport_range.end);
	}

	if( new_r.valid.table && new_r.table )
		fdc.SetText(WinEditRuleTableButtonIndex, towstr(rtruletable(new_r.table)).c_str(), true);

	if( new_r.valid.protocol )
		fdc.SetText(WinEditRuleProtocolButtonIndex, towstr(rtprotocoltype(new_r.protocol)).c_str(), true);

	if( new_r.valid.goto_priority )
		fdc.SetCountText(WinEditRuleGotoEditIndex, new_r.goto_priority);

	if( new_r.valid.flowfrom )
		fdc.SetCountText(WinEditRuleRealmsBeginEditIndex, new_r.flowfrom);

	if( new_r.valid.flowto )
		fdc.SetCountText(WinEditRuleRealmsEndEditIndex, new_r.flowto);

	if( new_r.valid.suppress_prefixlength )
		fdc.SetCountText(WinEditRuleSuppressPrefixlengthEditIndex, new_r.suppress_prefixlength);

	if( new_r.valid.suppress_ifgroup )
		fdc.SetCountText(WinEditRuleSuppressIfgroupEditIndex, new_r.suppress_ifgroup);

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	FarDialog dlg(&fdc, EditRuleDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			switch( item.itemNum ) {
			case WinEditRulePrefEditIndex:
				new_r.valid.priority = !item.empty;
				new_r.priority = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleFromEditIndex:
				new_r.valid.fromIpandMask = !item.empty;
				new_r.fromIpandMask = item.newVal.ptrData;
				break;
			case WinEditRuleToEditIndex:
				new_r.valid.toIpandMask = !item.empty;
				new_r.toIpandMask = item.newVal.ptrData;
				break;
			case WinEditRuleFwmarkEditIndex:
				new_r.valid.fwmark = !item.empty;
				new_r.fwmark = dlg.GetInt32FromHex(item.itemNum, false);
				break;
			case WinEditRuleFwmaskEditIndex:
				new_r.valid.fwmask = !item.empty;
				new_r.fwmask = dlg.GetInt32FromHex(item.itemNum, false);
				break;
			case WinEditRuleTosEditIndex:
				new_r.tos = (uint8_t)dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleL3mdevCheckboxIndex:
				new_r.valid.l3mdev = new_r.l3mdev = item.newVal.Selected;
				break;
			case WinEditRuleTunIdEditIndex:
				new_r.valid.tun_id = !item.empty;
				new_r.tun_id = dlg.GetInt64(item.itemNum);
				break;
			case WinEditRuleUidrangeBeginEditIndex:
				new_r.valid.uid_range = !item.empty;
				new_r.uid_range.start = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleUidrangeEndEditIndex:
				new_r.uid_range.end = dlg.GetInt32(item.itemNum);
				if( !new_r.uid_range.end )
					new_r.uid_range.end = new_r.uid_range.start;
				break;
			case WinEditRuleSportBeginEditIndex:
				new_r.valid.sport_range = !item.empty;
				new_r.sport_range.start = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleSportEndEditIndex:
				new_r.sport_range.end = dlg.GetInt32(item.itemNum);
				if( !new_r.sport_range.end )
					new_r.sport_range.end = new_r.sport_range.start;
				break;
			case WinEditRuleDportBeginEditIndex:
				new_r.valid.dport_range = !item.empty;
				new_r.dport_range.start = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleDportEndEditIndex:
				new_r.dport_range.end = dlg.GetInt32(item.itemNum);
				if( !new_r.dport_range.end )
					new_r.dport_range.end = new_r.dport_range.start;
				break;
			case WinEditRuleGotoEditIndex:
				new_r.valid.goto_priority = !item.empty;
				if( new_r.valid.goto_priority )
					new_r.action = FR_ACT_GOTO;
				new_r.goto_priority = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleRealmsBeginEditIndex:
				new_r.valid.flowfrom = !item.empty;
				new_r.flowfrom = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleRealmsEndEditIndex:
				new_r.valid.flowto = !item.empty;
				new_r.flowto = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleSuppressPrefixlengthEditIndex:
				new_r.valid.suppress_prefixlength = !item.empty;
				new_r.suppress_prefixlength = dlg.GetInt32(item.itemNum);
				break;
			case WinEditRuleSuppressIfgroupEditIndex:
				new_r.valid.suppress_ifgroup = !item.empty;
				new_r.suppress_ifgroup = dlg.GetInt32(item.itemNum);
				break;
			};
		}
	}
	return change;
}

bool NetcfgIpRule::EditRule(void)
{
	LOG_INFO("\n");

	bool change = false;

	auto ppi = GetCurrentPanelItem();
	if( ppi ) {
		if( ppi->Flags & PPIF_USERDATA ) {

			assert(ppi->UserData && ((PluginUserData *)ppi->UserData)->size == sizeof(PluginUserData));

			r = ((PluginUserData *)ppi->UserData)->data.rule;
			new_r = *r;
			if( FillNewIpRule() )
				change = r->ChangeRule(new_r);
			r = nullptr;
		}
		FreePanelItem(ppi);
	}
	return change;
}

bool NetcfgIpRule::CreateRule(void)
{
	LOG_INFO("\n");
	bool change = false;

	r = nullptr;
	new_r = RuleRouteInfo();

	new_r.family = family;
	new_r.type = 0;
	new_r.table = 0;
	new_r.flags = 0;
	new_r.valid = {0};

	if( FillNewIpRule() )
		change = new_r.CreateRule();

	return change;
}

int NetcfgIpRule::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change)
{
	LOG_INFO("\n");

	if( controlState == 0 ) {
		switch( key ) {
		case VK_F8:
			{
			change = DeleteRule();
			return TRUE;
			}
		case VK_F4:
			change = EditRule();
			return TRUE;
		}
	}

	if( controlState == PKF_SHIFT && key == VK_F4 ) {
		change = CreateRule();
		return TRUE;
	}

	return IsPanelProcessKey(key, controlState);
}

int NetcfgIpRule::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	LOG_INFO("\n");

	NetCfgPlugin::psi.Control(PANEL_ACTIVE, FCTL_SETNUMERICSORT, 1, 0);

	*pItemsNumber = rule.size();
	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));
	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;

	for( const auto & item : rule ) {
		pi->FindData.lpwszFileName = DublicateCountString(item.priority);
		pi->FindData.dwFileAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
		pi->FindData.nFileSize = 0;

		PluginUserData * user_data = (PluginUserData *)malloc(sizeof(PluginUserData));
		if( user_data ) {
			user_data->size = sizeof(PluginUserData);
			user_data->data.rule = const_cast<RuleRouteInfo*>(&item);
			pi->Flags = PPIF_USERDATA;
			pi->UserData = (DWORD_PTR)user_data;
		}

		const wchar_t ** customColumnData = (const wchar_t **)malloc(RouteRuleColumnMaxIndex*sizeof(const wchar_t *));
		if( customColumnData ) {
			memset(customColumnData, 0, RouteRuleColumnMaxIndex*sizeof(const wchar_t *));
			customColumnData[RouteRuleColumnRuleIndex] = wcsdup(item.rule.c_str());
			pi->CustomColumnNumber = RouteRuleColumnMaxIndex;
			pi->CustomColumnData = customColumnData;
		}
		pi++;
	}
	return int(true);
}

void NetcfgIpRule::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber)
{
	FarPanel::FreeFindData(panelItem, itemsNumber);
	while( itemsNumber-- ) {
		assert( (panelItem+itemsNumber)->FindData.dwFileAttributes & FILE_FLAG_DELETE_ON_CLOSE );
		free((void *)(panelItem+itemsNumber)->FindData.lpwszFileName);
	}
}
#endif
