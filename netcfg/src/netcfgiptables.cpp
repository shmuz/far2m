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
#define LOG_SOURCE_FILE "netcfgiptables.cpp"

#if !defined(__APPLE__) && !defined(__FreeBSD__)

NetcfgTablesRoute::NetcfgTablesRoute(uint32_t index_, std::deque<IpRouteInfo> & inet_):
	FarPanel(index_),
	inet(inet_)
{
	LOG_INFO("\n");
}

NetcfgTablesRoute::~NetcfgTablesRoute()
{
	LOG_INFO("\n");
}

int NetcfgTablesRoute::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change)
{
	LOG_INFO("\n");
	if( controlState == 0 && key == VK_RETURN ) {
			PanelInfo pi = {0};
			if( auto ppi = GetCurrentPanelItem(&pi) ) {
				PanelRedrawInfo pri;
				topIndex = pi.TopPanelItem;
				dirIndex = pi.CurrentItem;
				pri.TopPanelItem = 0;
				pri.CurrentItem = 0;
				FreePanelItem(ppi);
				NetCfgPlugin::psi.Control(hPlugin, FCTL_UPDATEPANEL, TRUE, 0);
				NetCfgPlugin::psi.Control(hPlugin, FCTL_REDRAWPANEL, 0, (LONG_PTR)&pri);
				return TRUE;
			}
	}

	return IsPanelProcessKey(key, controlState);
}

int NetcfgTablesRoute::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	LOG_INFO("\n");
	std::map<int, int> tables{{0,0},{253,0},{254,0},{255,0}};
	for( const auto & item : inet ) {
		auto [it, success] = tables.try_emplace(item.osdep.table, 0);
		tables[0]++;
		it->second++;
	}
	*pItemsNumber = tables.size();
	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));
	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;
	for( const auto & item : tables ) {
		pi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		pi->FindData.nFileSize = item.first;

		pi->FindData.dwFileAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
		pi->FindData.lpwszFileName = wcsdup(towstr(rtruletable(item.first)).c_str());

		const wchar_t ** CustomColumnData = (const wchar_t **)malloc(RouteIpTablesColumnMaxIndex*sizeof(const wchar_t *));
		if( CustomColumnData ) {
			memset(CustomColumnData, 0, 1*sizeof(const wchar_t *));
			CustomColumnData[RouteIpTablesColumnTotalIndex] = DublicateCountString(item.second);
			pi->CustomColumnNumber = RouteIpTablesColumnMaxIndex;
			pi->CustomColumnData = CustomColumnData;
		}
		pi++;
	}
	return int(true);
}

void NetcfgTablesRoute::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber)
{
	auto freeNumber = itemsNumber;
	while( freeNumber-- ) {
		assert( (panelItem+freeNumber)->FindData.dwFileAttributes & FILE_FLAG_DELETE_ON_CLOSE );
		free((void *)(panelItem+freeNumber)->FindData.lpwszFileName);
	}
	FarPanel::FreeFindData(panelItem, itemsNumber);
}

uint32_t NetcfgTablesRoute::GetTable(void)
{
	uint32_t table = 0;
	if( auto ppi = GetCurrentPanelItem() ) {
		table = ppi->FindData.nFileSize;
		FreePanelItem(ppi);
	}
	return table;
}

void NetcfgTablesRoute::SetLastPosition(HANDLE hPlugin)
{
	PanelRedrawInfo pri;
	pri.TopPanelItem = topIndex;
	pri.CurrentItem = dirIndex;
	NetCfgPlugin::psi.Control(hPlugin, FCTL_UPDATEPANEL, TRUE, 0);
	NetCfgPlugin::psi.Control(hPlugin, FCTL_REDRAWPANEL, 0, (LONG_PTR)&pri);
}

#endif
