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
#define LOG_SOURCE_FILE "netcfgplugin.cpp"

struct PluginStartupInfo NetCfgPlugin::psi = {0};
struct FarStandardFunctions NetCfgPlugin::FSF = {0};

NetCfgPlugin::NetCfgPlugin(const PluginStartupInfo * info)
{
	assert( (unsigned int)info->StructSize >= sizeof(PluginStartupInfo) );
	assert( psi.StructSize == 0 );
	psi=*info;

	assert( (unsigned int)info->FSF->StructSize >= sizeof(FarStandardFunctions) );
	assert( FSF.StructSize == 0 );
	FSF=*info->FSF;

	psi.FSF=&FSF;

	cfg = new PluginCfg();

	panel.push_back(std::make_shared<NetcfgInterfaces>(IfcsPanelIndex));
	panel.push_back(std::make_shared<NetcfgRoutes>());

	LOG_INFO("\n");
}

NetCfgPlugin::~NetCfgPlugin()
{
	LOG_INFO("\n");

	panel.clear();

	delete cfg;
	cfg = nullptr;

	memset(&psi, 0, sizeof(psi));
	memset(&FSF, 0, sizeof(FSF));
}

int NetCfgPlugin::Configure(int itemNumber)
{
	assert( cfg != 0 );
	return cfg->Configure(itemNumber);
}

HANDLE NetCfgPlugin::OpenPlugin(int openFrom, INT_PTR item)
{
	LOG_INFO("openFrom %u, item %u\n", openFrom, item);

	if( (openFrom == OPEN_DISKMENU && !cfg->interfacesAddToDisksMenu && cfg->routesAddToDisksMenu) ||
	    (openFrom == OPEN_PLUGINSMENU && !cfg->interfacesAddToPluginsMenu && cfg->routesAddToPluginsMenu))
		item = PanelNetworkRoutes;

	if( static_cast<size_t>(item) < panel.size() )
		return static_cast<HANDLE>(panel[item].get());
	return 0;
}

void NetCfgPlugin::ClosePlugin(HANDLE hPlugin)
{
	LOG_INFO("hPlugin %p\n", hPlugin);
}

void NetCfgPlugin::GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *info)
{
	LOG_INFO("GetOpenPluginInfo(hPlugin = %p)\n", hPlugin);
	static_cast<FarPanel *>(hPlugin)->GetOpenPluginInfo(info);
}

void NetCfgPlugin::GetPluginInfo(struct PluginInfo *info)
{
	LOG_INFO("\n");
	cfg->GetPluginInfo(info);
}

int NetCfgPlugin::GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->GetFindData(pPanelItem, pItemsNumber);
}

void NetCfgPlugin::FreeFindData(HANDLE hPlugin,struct PluginPanelItem * panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->FreeFindData(panelItem, itemsNumber);
}

int NetCfgPlugin::ProcessKey(HANDLE hPlugin,int key,unsigned int controlState)
{
	LOG_INFO("hPlugin: %p key 0x%08X VK_F2 0x%08X\n", hPlugin, key, VK_F2);

	bool redraw = false;
	int res = static_cast<FarPanel *>(hPlugin)->ProcessKey(hPlugin, key, controlState, redraw);

	if( redraw ) {
		LOG_INFO("NetCfgPlugin::ProcessKey() redraw\n");
		psi.Control(hPlugin, FCTL_UPDATEPANEL, TRUE, 0);
		psi.Control(hPlugin, FCTL_REDRAWPANEL, 0, 0);
	}
	return res;
}
