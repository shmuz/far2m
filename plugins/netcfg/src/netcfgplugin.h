#ifndef __NETCFGPLUGIN_H__
#define __NETCFGPLUGIN_H__

#include "netif/netifs.h"
#include "netroute/netroutes.h"
#include "plugincfg.h"
#include "fardialog.h"

#include "netcfgroutes.h"
#include "netcfginterfaces.h"

struct PluginUserData {
	DWORD size;
	union {
		NetInterface * net_if;
		IpRouteInfo * inet;
		ArpRouteInfo * arp;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		RuleRouteInfo * rule;
#endif
	} data;
};

class NetCfgPlugin {
	private:
		// Configuration
		PluginCfg * cfg;

		// Panels
		enum {
			PanelNetworkInterfaces,
			PanelNetworkRoutes,
			PanelTypeMax
		};
		std::vector<std::shared_ptr<FarPanel>> panel;

		// copy and assignment not allowed
		NetCfgPlugin(const NetCfgPlugin&) = delete;
		void operator=(const NetCfgPlugin&) = delete;

	public:

		explicit NetCfgPlugin(const PluginStartupInfo * info);
		~NetCfgPlugin();

		// far2l backconnect
		static struct PluginStartupInfo psi;
		static struct FarStandardFunctions FSF;
	
		// far2l api
		int GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber);
		void FreeFindData(HANDLE hPlugin, struct PluginPanelItem * PanelItem, int ItemsNumber);
		void GetPluginInfo(struct PluginInfo *info);

		HANDLE OpenPlugin(int openFrom, INT_PTR item);
		void ClosePlugin(HANDLE hPlugin);
		void GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *info);
		int ProcessKey(HANDLE hPlugin,int key,unsigned int controlState);
		int Configure(int itemNumber);
};

#endif /* __NETCFGPLUGIN_H__ */
