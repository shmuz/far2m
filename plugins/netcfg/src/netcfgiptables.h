#ifndef __NETCFGIPTABLES_H__
#define __NETCFGIPTABLES_H__

#include "farpanel.h"
#include "netroute/netroutes.h"
#include "netcfgrules.h"
#include "netcfgarp.h"
#include <memory>

enum {
	RouteIpTablesColumnTotalIndex,
	RouteIpTablesColumnMaxIndex
};

class NetcfgTablesRoute : public FarPanel
{
private:
	std::deque<IpRouteInfo> & inet;
	uint32_t dirIndex;
	uint32_t topIndex;
public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber) override;
	uint32_t GetTable(void);
	void SetLastPosition(HANDLE hPlugin);
	explicit NetcfgTablesRoute(uint32_t index, std::deque<IpRouteInfo> & inet);
	~NetcfgTablesRoute();
};

#endif // __NETCFGIPTABLES_H__
