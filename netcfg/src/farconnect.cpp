#include "netcfgplugin.h"

#include <common/log.h>
#include <common/errname.h>
#include <assert.h>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "farconnect.cpp"

// plugin
static class NetCfgPlugin * gNet = 0;

SHAREDSYMBOL int WINAPI _export ConfigureW(int itemNumber)
{
	LOG_INFO("\n");
	assert( gNet != 0 );
	return gNet->Configure(itemNumber);
}

// export fot netif lib
int RootExec(const char * cmd)
{
	assert( gNet != 0 );
	if( !gNet )
		return -1;
	std::string _acmd(cmd);
	std::wstring _cmd(_acmd.begin(), _acmd.end());
	int res = gNet->FSF.Execute(_cmd.c_str(), EF_SUDO);
	LOG_INFO("exec \"%s\" return %d\n", cmd, res);
	return res;
}

SHAREDSYMBOL int WINAPI _export ProcessKeyW(HANDLE hPlugin,int key,unsigned int controlState)
{
	return gNet->ProcessKey(hPlugin, key, controlState);
}

#include <time.h>

SHAREDSYMBOL int WINAPI _export ProcessEventW(HANDLE hPlugin,int event,void *param)
{
	int res = FALSE;
	static clock_t t;

	switch(event) {
	case FE_CHANGEVIEWMODE:
		LOG_INFO(":FE_CHANGEVIEWMODE \"%S\"\n", (const wchar_t *)param);
		res = FALSE;
		break;
	case FE_REDRAW:
		LOG_INFO(":FE_REDRAW %p\n", param);
		res = FALSE; // redraw
		//res = TRUE; // no redraw
		break;
	case FE_IDLE:
		{
		// t = clock();
		//LOG_INFO(":FE_IDLE %p\n", param);
		clock_t t1 = clock();
		LOG_INFO("FE_IDLE %p time %f\n", param, ((double)t1 - t) / CLOCKS_PER_SEC);
		t = t1;
		res = FALSE;
		}
		break;
	case FE_CLOSE:
		LOG_INFO(":FE_CLOSE %p\n", param);
		res = FALSE; // close panel
		// res = TRUE; // no close panel
		break;
	case FE_BREAK:
		// Ctrl-Break is pressed
		// Processing of this event is performed in separate thread,
		// plugin must not use FAR service functions.
#if INTPTR_MAX == INT32_MAX
		LOG_INFO(":FE_BREAK CTRL_BREAK_EVENT %llu\n", param);
#elif INTPTR_MAX == INT64_MAX
		LOG_INFO(":FE_BREAK CTRL_BREAK_EVENT %lu\n", param);
#else
    #error "Environment not 32 or 64-bit."
#endif
		res = FALSE;
		break;
	case FE_COMMAND:
		LOG_INFO(":FE_COMMAND \"%S\"\n", (const wchar_t *)param);
		res = FALSE; // allow standard command execution
		//res = TRUE; //  TRUE if it is going to process the command internally.
		break;
	case FE_GOTFOCUS:
		LOG_INFO(":FE_GOTFOCUS %p\n", param);
		res = FALSE;
		break;
	case FE_KILLFOCUS:
		LOG_INFO(":FE_KILLFOCUS %p\n", param);
		res = FALSE;
		break;
	default:
		LOG_INFO(":UNKNOWN %p\n", param);
		break;
	};
	return res;
}

SHAREDSYMBOL void WINAPI _export FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	LOG_INFO("hPlugin %p pPanelItem %p ItemsNumber %u\n", hPlugin, PanelItem, ItemsNumber);
	gNet->FreeFindData(hPlugin, PanelItem, ItemsNumber);
	return;
}

SHAREDSYMBOL int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
	LOG_INFO("hPlugin %p OpMode %d *pPanelItem %p *pItemsNumber %u\n", hPlugin, OpMode, *pPanelItem, *pItemsNumber);
	gNet->GetFindData(hPlugin, pPanelItem, pItemsNumber);
	return TRUE;
}

SHAREDSYMBOL void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *info)
{
	LOG_INFO("hPlugin %p info %p\n", hPlugin, info);
	gNet->GetOpenPluginInfo(hPlugin, info);	
}

SHAREDSYMBOL HANDLE WINAPI _export OpenPluginW(int openFrom, INT_PTR item)
{
	LOG_INFO("(int OpenFrom=%u,INT_PTR Item=%u)\n", openFrom, item);

	return gNet->OpenPlugin(openFrom, item);
}

SHAREDSYMBOL void WINAPI _export ClosePluginW(HANDLE hPlugin)
{
	LOG_INFO("hPlugin 0x%p\n", hPlugin);
	gNet->ClosePlugin(hPlugin);
}

SHAREDSYMBOL void WINAPI _export GetPluginInfoW(struct PluginInfo *info)
{
	LOG_INFO("(struct PluginInfo *info = %p)\n", info);
	gNet->GetPluginInfo(info);
}

SHAREDSYMBOL void WINAPI _export SetStartupInfoW(const struct PluginStartupInfo * info)
{
	assert( gNet == 0 );
	gNet = new NetCfgPlugin(info);
}

SHAREDSYMBOL int WINAPI _export GetMinFarVersionW()
{
//	LOG_INFO("return 0x%08X\n", FARMANAGERVERSION);
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI _export ExitFARW()
{
	LOG_INFO("\n");
	delete gNet;
	gNet = 0;
}
