#ifndef __NETCFGIPROUTES_H__
#define __NETCFGIPROUTES_H__

#include "netfarpanel.h"
#include "netroute/netroutes.h"

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include "netcfgiptables.h"
#include "netcfgrules.h"
#endif

#include "netcfgarp.h"
#include <memory>

enum {
	RoutesColumnViaIndex,
	RoutesColumnDevIndex,
	RoutesColumnPrefsrcIndex,
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	RoutesColumnTypeIndex,
	#else
	RoutesColumnFlagsIndex,
	#endif
	RoutesColumnMetricIndex,
	RoutesColumnDataMaxIndex
};

#define ROUTE_TABLE_DIRS static_cast<uint32_t>(-1)

class NetcfgIpRoute : public NetFarPanel
{
private:
	std::deque<IpRouteInfo> & inet;
	std::map<uint32_t, std::wstring> & ifs;

	uint8_t family;

	#if !defined(__APPLE__) && !defined(__FreeBSD__)

	std::unique_ptr<NetcfgIpRule> rule;
	std::unique_ptr<NetcfgTablesRoute> tables;

	typedef enum {
		PanelUndefined,
		PanelRoutes,
		PanelTables,
		PanelRules
	} RoutesPanelMode;

	RoutesPanelMode panel;
	RoutesPanelMode oldPanel;

	uint32_t table;

	void FillNextHope(uint32_t iindex, ItemChange & item, NextHope & nh);

	NextHope new_nh;

	bool SelectEncap(HANDLE hDlg, Encap & enc);

	#endif
	bool DeleteIpRoute(void);

	IpRouteInfo * rt;
	IpRouteInfo new_rt;
	bool change;

	bool EditIpRoute(void);
	bool CreateIpRoute(void);
	bool FillNewIpRoute(void);

	friend LONG_PTR WINAPI EditIpRouteDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);
	friend LONG_PTR WINAPI EditEncapDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);
public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	void GetOpenPluginInfo(struct OpenPluginInfo * info) override;
	void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber) override;
	explicit NetcfgIpRoute(uint32_t index, uint8_t family, std::deque<IpRouteInfo> & inet, std::deque<RuleRouteInfo> & rule, std::map<uint32_t, std::wstring> & ifs);
	#else
	explicit NetcfgIpRoute(uint32_t index, uint8_t family, std::deque<IpRouteInfo> & inet, std::map<uint32_t, std::wstring> & ifs);
	#endif
	virtual ~NetcfgIpRoute();
};

#endif // __NETCFGIPROUTES_H__
