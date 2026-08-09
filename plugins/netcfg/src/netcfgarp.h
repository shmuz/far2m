#ifndef __NETCFGARP_H__
#define __NETCFGARP_H__

#include "farpanel.h"
#include "netroute/netroutes.h"
#include <memory>

enum {
	ArpRoutesColumnMacIndex,
	ArpRoutesColumnDevIndex,
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	ArpRoutesColumnTypeIndex,
	#else
	ArpRoutesColumnFlagsIndex,
	#endif
	ArpRoutesColumnStateIndex,
	ArpRoutesColumnMaxIndex
};

class NetcfgArpRoute : public NetFarPanel
{
private:
	std::deque<ArpRouteInfo> & arp;

	ArpRouteInfo * a;
	ArpRouteInfo new_a;

	friend LONG_PTR WINAPI EditArpDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);

	bool FillNewArp(void);
	bool DeleteArp(void);
	bool EditArp(void);
	bool CreateArp(void);

public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	explicit NetcfgArpRoute(uint32_t index, std::deque<ArpRouteInfo> & arp, std::map<uint32_t, std::wstring> & ifs);
	~NetcfgArpRoute();
};

#endif // __NETCFGARP_H__
