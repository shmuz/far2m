#ifndef __NETCFGRULES_H__
#define __NETCFGRULES_H__

#include "netfarpanel.h"
#include "netroute/netroutes.h"
#include <memory>

enum {
	RouteRuleColumnRuleIndex,
	RouteRuleColumnMaxIndex
};

class NetcfgIpRule : public NetFarPanel
{
private:
	std::deque<RuleRouteInfo> & rule;

	uint8_t family;

	RuleRouteInfo * r;
	RuleRouteInfo new_r;

	friend LONG_PTR WINAPI EditRuleDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);

	bool FillNewIpRule(void);
	bool DeleteRule(void);
	bool EditRule(void);
	bool CreateRule(void);
public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber) override;
	explicit NetcfgIpRule(uint32_t index, uint8_t family, std::deque<RuleRouteInfo> & rule, std::map<uint32_t, std::wstring> & ifs);
	~NetcfgIpRule();
};

#endif // __NETCFGRULES_H__
