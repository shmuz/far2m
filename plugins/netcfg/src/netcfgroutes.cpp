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
#define LOG_SOURCE_FILE "netcfgroutes.cpp"

NetcfgRoutes::NetcfgRoutes():
	FarPanel()
{
	LOG_INFO("\n");

	nrts = std::make_unique<NetRoutes>();

	change = true;

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	panels.push_back(std::make_unique<NetcfgIpRoute>(RouteInetPanelIndex, AF_INET, nrts->inet, nrts->rule, nrts->ifs));
	panels.push_back(std::make_unique<NetcfgIpRoute>(RouteInet6PanelIndex, AF_INET6, nrts->inet6, nrts->rule6, nrts->ifs));
	#else
	panels.push_back(std::make_unique<NetcfgIpRoute>(RouteInetPanelIndex, AF_INET, nrts->inet, nrts->ifs));
	panels.push_back(std::make_unique<NetcfgIpRoute>(RouteInet6PanelIndex, AF_INET6, nrts->inet6, nrts->ifs));
	#endif

	panels.push_back(std::make_unique<NetcfgArpRoute>(RouteArpPanelIndex, nrts->arp, nrts->ifs));

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( mcinetPanelValid )
		panels.push_back(std::make_unique<NetcfgIpRoute>(RouteMcInetPanelIndex, RTNL_FAMILY_IPMR, nrts->mcinet, nrts->mcrule, nrts->ifs));
	if( mcinet6PanelValid )
		panels.push_back(std::make_unique<NetcfgIpRoute>(RouteMcInet6PanelIndex, RTNL_FAMILY_IP6MR, nrts->mcinet6, nrts->mcrule6, nrts->ifs));
	#endif

	active = 0;
}

NetcfgRoutes::~NetcfgRoutes()
{
	LOG_INFO("\n");

}

const int EDIT_SETTINGS_DIALOG_WIDTH = 84;

enum {
	WinEditSetCaptionIndex,
	WinEditSetIpv4ForwardingCheckboxIndex,
	WinEditSetIpv6ForwardingCheckboxIndex,
	WinEditSetMaxIndex
};

void NetcfgRoutes::EditSettings(void)
{
	static const DlgConstructorItem dialog[] = {
		//  Type       NewLine X1              X2         Flags                PtrData
		{DI_DOUBLEBOX, false,  DEFAUL_X_START, EDIT_SETTINGS_DIALOG_WIDTH-4,  0,     {.ptrData = L"Edit settings:"}},
		{DI_CHECKBOX,  true,   5,             0,           0,                  {.ptrData = L"ipv4 forwarding"}},
		{DI_CHECKBOX,  false, 42,             0,           0,                  {.ptrData = L"ipv6 forwarding"}},
		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dialog[0]);

	if( nrts->ipv4_forwarding )
		fdc.SetSelected(WinEditSetIpv4ForwardingCheckboxIndex, true);

	if( nrts->ipv6_forwarding )
		fdc.SetSelected(WinEditSetIpv6ForwardingCheckboxIndex, true);

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	FarDialog dlg(&fdc);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;

		if( !dlg.CreateChangeList(chlst) )
			return;

		for( auto & item : chlst ) {
			switch( item.itemNum ) {
			case WinEditSetIpv4ForwardingCheckboxIndex:
				change |= nrts->SetIpForwarding(item.newVal.Selected != 0);
				break;
			case WinEditSetIpv6ForwardingCheckboxIndex:
				change |= nrts->SetIp6Forwarding(item.newVal.Selected != 0);
				break;
			};
		}
	}
	return;
}

bool NetcfgRoutes::SelectActivePanel(void)
{
	static const int menuItems[] = {
		MPanelNetworkRoutesTitle,
		MPanelNetworkRoutes6Title,
		MPanelNetworkRoutesArpTitle
	};

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	int if_count = ARRAYSIZE(menuItems) + (int)mcinetPanelValid + (int)mcinet6PanelValid, index = 0;
#else
	int if_count = ARRAYSIZE(menuItems), index = 0;
#endif
	auto menuElements = std::make_unique<FarMenuItem[]>(if_count);
	memset(menuElements.get(), 0, sizeof(FarMenuItem)*if_count);
	for( const auto & num : menuItems ) {
		menuElements[index].Text = GetMsg(num);
		index++;
	}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( mcinetPanelValid ) {
		menuElements[index].Text = GetMsg(MPanelNetworkMcRoutesTitle);
		index++;
	}

	if( mcinet6PanelValid ) {
		menuElements[index].Text = GetMsg(MPanelNetworkMcRoutes6Title);
		index++;
	}
#endif

	if( index ) {
		menuElements[0].Selected = 1;
		index = NetCfgPlugin::psi.Menu(NetCfgPlugin::psi.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, \
					 L"Select active panel:", 0, L"hrd", nullptr, nullptr, menuElements.get(), index);
		if( index >= 0 && index < static_cast<int>(panels.size()) ) {
			active = (uint32_t)index;
			return true;
		}
	}
	return false;
}

int NetcfgRoutes::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & redraw)
{
	if( controlState == 0 ) {
		switch( key ) {
		case VK_F6:
			redraw = SelectActivePanel();
			return TRUE;
		case VK_F2:
			change = true;
			redraw = true;
			return TRUE;
		case VK_F7:
			EditSettings();
			redraw = change;
			return TRUE;
		}
	}

	auto res = panels[active]->ProcessKey(hPlugin, key, controlState, redraw);

	if( redraw )
		change = true;

	return res;
}

int NetcfgRoutes::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	if( change && nrts->Update() ) {
		nrts->Log();
		change = false;
	}

	LOG_INFO("active %u\n", active);
	return panels[active]->GetFindData(pPanelItem, pItemsNumber);
}

void NetcfgRoutes::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	panels[active]->FreeFindData(panelItem, itemsNumber);
}

void NetcfgRoutes::GetOpenPluginInfo(struct OpenPluginInfo * info)
{
	LOG_INFO("NetcfgRoutes::GetOpenPluginInfo()\n");
	panels[active]->GetOpenPluginInfo(info);
}
