#include "plugincfg.h"
#include "netcfgplugin.h"
#include "netcfglng.h"
#include "fardialog.h"
#include "farpanel.h"

#include <string>
#include <assert.h>


#include <utils.h>
#include <KeyFileHelper.h>

#include <common/log.h>

#define LOG_SOURCE_FILE "plugincfg.cpp"
#define LOG_MAX_PATH 256

static char initial_log[LOG_MAX_PATH] = {'/','t','m','p','/','f','a','r','2','l','.','n','e','t','c','f','g','.','l','o','g',0};
const char * LOG_FILE = initial_log;
//static_assert( sizeof("/dev/null") < LOG_MAX_PATH );

#define INI_LOCATION InMyConfig("plugins/netcfg/config.ini")
#define INI_SECTION "Settings"

const char * PluginCfg::GetPanelName(PanelIndex index) const
{
	static const char * names[] = {
		"ifaces",	// IfcsPanelIndex
		"inet",		// RouteInetPanelIndex
		"inet6",	// RouteInet6PanelIndex
		"arp",		// RouteArpPanelIndex
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		"mcinet",	// RouteMcInetPanelIndex
		"mcinet6",	// RouteMcInet6PanelIndex
		"rule",	    // RouteRuleInetPanelIndex
		"routetables", // RouteIpTablesPanelIndex
		#endif
		"max"		// RouteMcInet6PanelIndex
	};
	assert( index < (ARRAYSIZE(names)-1) );
	return names[index];
}

size_t PluginCfg::init = 0;
std::map<PanelIndex, CfgDefaults> PluginCfg::def = {\
		{IfcsPanelIndex, {
		L"N,C0,SF,C1,C2",
		L"7,16,7,17,0",
		// name                       N
		// ip                         C0
		// recv_bytes + send_bytes    SF
		// mac                        C1
		// ipv6                       C2
		// mtu                        C3
		// recv_bytes                 C4
		// send_bytes                 C5
		// recv_packets               C6
		// send_packets               C7
		// recv_errors                C8
		// send_errors                C9
		// multicast                  C10
		// collisions                 C11
		// permanent_mac              C12
		{L"N,C0,SF,C1,C2", L"N,C0,C1,C2,C3,C4,C5,C6,C7,C8,C9,C10,C11,C12"},
		{L"7,15,7,17,0", L"7,15,17,25,0,0,0,0,0,0,0,0,0,17"},
		{{L"ifc",L"ip",L"snd+rcv", L"mac", L"ipv6", 0}, {L"ifc",L"ip",L"mac", L"ipv6", L"mtu", L"rcv", L"snd", L"rpkts", L"spkts",L"rerr",L"serr",L"mcast",L"colls",L"permmac"}},
		{0,MF2,MEmptyString,MF4,MF5,MF6,MEmptyString,MEmptyString,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkInterfacesTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS
		}},
		{RouteInetPanelIndex, {
		L"N,C0,C1,C2,C3,C4",
		L"18,14,7,17,0,0",
		// dest ip and mask           N
		// gateway                    C0
		// dev                        C1
		// prefsrc                    C2
		// type                       C3
		// metric                     C4
		{L"N,C0,C1,C2,C3,C4", L"N,C0,C1,C2,C3,C4"},
		{L"18,14,7,17,0,0", L"18,14,7,17,0,0"},
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{{L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}, {L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}},
		{0,MF2,MF3Rules,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#else
		{{L"to", L"via", L"dev", L"prefsrc", L"flags", L"expire", 0}, {L"to", L"via", L"dev", L"prefsrc", L"flags", L"expire", 0}},
		{0,MF2,MEmptyString,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#endif
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkRoutesTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS
		}},
		{RouteInet6PanelIndex, {
		L"N,C0,C1,C2,C3",
		L"29,16,7,17,0",
		{L"N,C0,C1,C2,C3,C4", L"N,C0,C1,C2,C3,C4"},
		{L"29,16,7,17,0,0", L"29,16,7,17,0,0"},
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{{L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}, {L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}},
		{0,MF2,MF3Rules,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#else
		{{L"to", L"via", L"dev", L"prefsrc", L"flags", L"expire", 0}, {L"to", L"via", L"dev", L"prefsrc", L"flags", L"expire", 0}},
		{0,MF2,MEmptyString,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#endif
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkRoutes6Title,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS
		}},
		{RouteArpPanelIndex, {
		L"N,C0,C1,C2,C3",
		L"0,18,7,10,9",
		{L"N,C0,C1,C2,C3", L"N,C0,C1,C2,C3"},
		{L"0,18,7,10,9", L"0,18,7,10,9"},
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{{L"to ip", L"mac", L"dev", L"type", L"state", 0}, {L"to ip", L"mac", L"dev", L"type", L"state", 0}},
		{0,MF2,MEmptyString,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#else
		{{L"to ip", L"mac", L"dev", L"flags", L"state", 0}, {L"to ip", L"mac", L"dev", L"flags", L"state", 0}},
		{0,MF2,MEmptyString,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		#endif
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkRoutesArpTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE
		}},
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{RouteMcInetPanelIndex, {
		L"N,C0,C1,C2,C3,C4",
		L"18,16,7,17,0,0",
		{L"N,C0,C1,C2,C3,C4", L"N,C0,C1,C2,C3,C4"},
		{L"18,16,7,17,0,0", L"18,16,7,17,0,0"},
		{{L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}, {L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}},
		{0,MF2,MF3Rules,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkMcRoutesTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS
		}},
		{RouteMcInet6PanelIndex, {
		L"N,C0,C1,C2,C3",
		L"29,16,7,17,0",
		{L"N,C0,C1,C2,C3,C4", L"N,C0,C1,C2,C3,C4"},
		{L"29,16,7,17,0,0", L"29,16,7,17,0,0"},
		{{L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}, {L"to", L"via", L"dev", L"prefsrc", L"proto", L"metric", 0}},
		{0,MF2,MF3Rules,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkMcRoutes6Title,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS
		}},
		{RouteRuleInetPanelIndex, {
		L"N,C0",
		L"6,0",
		// prio           N
		// rule           C0
		{L"N,C0", L"N,C0"},
		{L"6,0", L"6,0"},
		{{L"prio", L"rule", 0}, {L"prio", L"rule", 0}},
		{0,MF2,MF3Routes,MF4,MEmptyString,MF6Switch,MF7Settings,MF8Routes,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MF4Create,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkRouteRulesTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE
		}},
		{RouteIpTablesPanelIndex, {
		L"N,C0",
		L"10,0",
		// name           N
		// total          C0
		{L"N,C0", L"N,C0"},
		{L"10,0", L"10,0"},
		{{L"Table:", L"Total routes:", 0}, {L"Table:", L"Total routes:", 0}},
		{0,MF2,MF3Rules,MEmptyString,MEmptyString,MF6Switch,MF7Settings,MEmptyString,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		MPanelNetworkRouteRulesTitle,
		MFormatNetcfgPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE
		}}
		#endif
		};

bool PluginCfg::logEnable = true;
bool PluginCfg::interfacesAddToDisksMenu = false;
bool PluginCfg::interfacesAddToPluginsMenu = false;
bool PluginCfg::routesAddToDisksMenu = false;
bool PluginCfg::routesAddToPluginsMenu = false;

#if !defined(__APPLE__) && !defined(__FreeBSD__)
bool PluginCfg::mcinetPanelValid = false;
bool PluginCfg::mcinet6PanelValid = false;
#endif


void PluginCfg::ReloadPanelString(struct PanelData * data, PanelIndex index)
{
	assert( def.find(index) != def.end() );
	auto & cfg = def[index];
	auto keyBar = &data->keyBar.Titles[0];
	for( auto item : cfg.keyBarTitles ) {
		if( item && item < MMaxString )
			*keyBar = (TCHAR*)GetMsg(item);
		keyBar++;
	}

	keyBar = &data->keyBar.ShiftTitles[0];
	for( auto item : cfg.keyBarShiftTitles ) {
		if( item && item < MMaxString )
			*keyBar = (TCHAR*)GetMsg(item);
		keyBar++;
	}

	constexpr static wchar_t * empty_string = const_cast<wchar_t *>(L"");
	constexpr static wchar_t * defAltkeys[12] = {0,0,empty_string,empty_string,empty_string,empty_string,empty_string,0,0,empty_string,empty_string,empty_string};
	constexpr static wchar_t * defkeys[12] = {empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string};
	memmove(data->keyBar.AltTitles, defAltkeys, sizeof(defAltkeys));
	memmove(data->keyBar.CtrlShiftTitles, defkeys, sizeof(defkeys));
	memmove(data->keyBar.AltShiftTitles, defkeys, sizeof(defkeys));
	memmove(data->keyBar.CtrlAltTitles, defkeys, sizeof(defkeys));

	data->openInfo.Format=(TCHAR*)GetMsg(cfg.format);
	data->openInfo.PanelTitle=(TCHAR*)GetMsg(cfg.panelTitle);
}

void PluginCfg::FillPanelData(struct PanelData * data, PanelIndex index)
{
	assert( def.find(index) != def.end() );

	auto & cfg = def[index];
	auto & nmodes = data->panelModesArray;
	for( size_t i =0; i < ARRAYSIZE(nmodes); i++ ) {
		nmodes[i].FullScreen = FALSE;
		nmodes[i].DetailedStatus = 0;
		nmodes[i].AlignExtensions = 0;
		nmodes[i].CaseConversion = TRUE;
		nmodes[i].StatusColumnTypes = cfg.statusColumnTypes;
		nmodes[i].StatusColumnWidths = cfg.statusColumnWidths;
	}

	nmodes[4].ColumnTypes = cfg.columnTypes[0];
	nmodes[4].ColumnWidths = cfg.columnWidths[0];
	nmodes[4].ColumnTitles = cfg.columnTitles[0];

	nmodes[5].ColumnTypes =  cfg.columnTypes[1];
	nmodes[5].ColumnWidths = cfg.columnWidths[1];
	nmodes[5].ColumnTitles = cfg.columnTitles[1];
	nmodes[5].FullScreen = TRUE;

	data->openInfo.Flags = cfg.flags;
	data->openInfo.CurDir=_T("");
	data->openInfo.StartPanelMode=_T('4');
	data->openInfo.StartSortMode = SM_NAME;

	ReloadPanelString(data, index);
}

PluginCfg::PluginCfg()
{
	LOG_INFO("init %d\n", init);

	if( init++ )
		return;

	LOG_INFO("=== INIT ===\n");

	{
		KeyFileReadSection kfr(INI_LOCATION, INI_SECTION);

		interfacesAddToDisksMenu = (bool)kfr.GetInt("interfacesAddToDisksMenu", true);
		interfacesAddToPluginsMenu = (bool)kfr.GetInt("interfacesAddToPluginsMenu", true);
		routesAddToDisksMenu = (bool)kfr.GetInt("routesAddToDisksMenu", true);
		routesAddToPluginsMenu = (bool)kfr.GetInt("routesAddToPluginsMenu", true);

		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		mcinetPanelValid = (bool)kfr.GetInt("mcinetPanelValid", true);
		mcinet6PanelValid = (bool)kfr.GetInt("mcinet6PanelValid", true);
		if( !mcinetPanelValid && !mcinet6PanelValid )
			def[RouteArpPanelIndex].keyBarTitles[6-1] = MF6Routes;
		else if( !mcinetPanelValid )
			def[RouteArpPanelIndex].keyBarTitles[6-1] = MF6McRoutes6;
		#endif

		logEnable = (bool)kfr.GetInt("logEnable", true);
	       	if( logEnable ) {
			std::string logfile = kfr.GetString("logfile", initial_log);
			if( logfile.size() < (LOG_MAX_PATH-1) && logfile.size() >= sizeof("/a") )
				memmove(initial_log, logfile.c_str(), logfile.size()+1);
		} else
			memmove(initial_log, "/dev/null", sizeof("/dev/null"));
	}

	KeyFileReadHelper kfrh(INI_LOCATION);
	for( auto & [index, item] : def ) {
		const char * name = GetPanelName(index);

		item.statusColumnTypes = wcsdup(kfrh.GetString(name, "statusColumnTypes", item.statusColumnTypes).c_str());
		item.statusColumnWidths = wcsdup(kfrh.GetString(name, "statusColumnWidths", item.statusColumnWidths).c_str());

		item.columnTypes[0] = wcsdup(kfrh.GetString(name, "columnTypes4", item.columnTypes[0]).c_str());
		item.columnTypes[1] = wcsdup(kfrh.GetString(name, "columnTypes5", item.columnTypes[1]).c_str());

		item.columnWidths[0] = wcsdup(kfrh.GetString(name, "columnWidths4", item.columnWidths[0]).c_str());
		item.columnWidths[1] = wcsdup(kfrh.GetString(name, "columnWidths5", item.columnWidths[1]).c_str());

		std::string field_prefix("title4_");
		uint32_t offset = 0;
		for( auto & title: item.columnTitles[0] ) {
			if( title ) {
				std::string field = field_prefix + std::to_string(offset);
				title = wcsdup(kfrh.GetString(name, field.c_str(), title).c_str());
			}
			offset++;
		}

		field_prefix = "title5_";
		offset = 0;
		for( auto & title: item.columnTitles[1] ) {
			if( title ) {
				std::string field = field_prefix + std::to_string(offset);
				title = wcsdup(kfrh.GetString(name, field.c_str(), title).c_str());
			}
			offset++;
		}
	}
}

void PluginCfg::SaveConfig(void) const
{
	LOG_INFO("=== Save config ===\n");

	KeyFileHelper kfh(INI_LOCATION);
	for( auto & [index, item] : def ) {
		const char * name = GetPanelName(index);
		kfh.SetString(name, "statusColumnTypes", item.statusColumnTypes);
		kfh.SetString(name, "statusColumnWidths", item.statusColumnWidths);
		kfh.SetString(name, "columnTypes4", item.columnTypes[0]);
		kfh.SetString(name, "columnTypes5", item.columnTypes[1]);
		kfh.SetString(name, "columnWidths4", item.columnWidths[0]);
		kfh.SetString(name, "columnWidths5", item.columnWidths[1]);

		std::string field_prefix("title4_");
		uint32_t offset = 0;
		for( auto & title: item.columnTitles[0] ) {
			if( title ) {
				std::string field = field_prefix + std::to_string(offset);
				kfh.SetString(name, field.c_str(), title);
			}
			offset++;
		}

		field_prefix = "title5_";
		offset = 0;
		for( auto & title: item.columnTitles[1] ) {
			if( title ) {
				std::string field = field_prefix + std::to_string(offset);
				kfh.SetString(name, field.c_str(), title);
			}
			offset++;
		}
	}

	kfh.SetInt(INI_SECTION, "interfacesAddToDisksMenu", interfacesAddToDisksMenu);
	kfh.SetInt(INI_SECTION, "interfacesAddToPluginsMenu", interfacesAddToPluginsMenu);
	kfh.SetInt(INI_SECTION, "routesAddToDisksMenu", routesAddToDisksMenu);
	kfh.SetInt(INI_SECTION, "routesAddToPluginsMenu", routesAddToPluginsMenu);
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	kfh.SetInt(INI_SECTION, "mcinetPanelValid", mcinetPanelValid);
	kfh.SetInt(INI_SECTION, "mcinet6PanelValid", mcinet6PanelValid);
	#endif

	std::string _logfile(LOG_FILE);
	kfh.SetString(INI_SECTION, "logfile", _logfile);
	kfh.SetInt(INI_SECTION, "logEnable", logEnable);
	kfh.Save();
}

PluginCfg::~PluginCfg()
{
	LOG_INFO("init %d\n", init);

	if( --init )
		return;

	LOG_INFO("=== FREE ===\n");

	for( auto & [index, item] : def ) {
		free((void *)item.statusColumnTypes);
		free((void *)item.statusColumnWidths);
		free((void *)item.columnTypes[0]);
		free((void *)item.columnTypes[1]);
		free((void *)item.columnWidths[0]);
		free((void *)item.columnWidths[1]);

		for( auto & title: item.columnTitles[0] ) {
			if( title )
				free((void *)title);
		}
		for( auto & title: item.columnTitles[1] ) {
			if( title )
				free((void *)title);
		}
	}
}

const wchar_t * PluginCfg::GetMsg(int msgId)
{
	assert( NetCfgPlugin::psi.GetMsg != 0 );
	return NetCfgPlugin::psi.GetMsg(NetCfgPlugin::psi.ModuleNumber, msgId);
}

void PluginCfg::GetPluginInfo(struct PluginInfo *info)
{
	info->StructSize = sizeof(PluginInfo);
	info->Flags = 0;

	static const wchar_t *diskMenuStrings[2];
	static const wchar_t *pluginMenuStrings[2];
	static const wchar_t *pluginConfigStrings[1];

	diskMenuStrings[0] = GetMsg(MInterfacesDiskMenuString);
	diskMenuStrings[1] = GetMsg(MRoutesDiskMenuString);
	if( !interfacesAddToDisksMenu && routesAddToDisksMenu )
		diskMenuStrings[0] = GetMsg(MRoutesDiskMenuString);

	info->DiskMenuStrings = diskMenuStrings;
	info->DiskMenuStringsNumber = (int)interfacesAddToDisksMenu + (int)routesAddToDisksMenu;

	pluginMenuStrings[0] = GetMsg(MInterfacesDiskMenuString);
	pluginMenuStrings[1] = GetMsg(MRoutesDiskMenuString);
	if( !interfacesAddToPluginsMenu && routesAddToPluginsMenu )
		pluginMenuStrings[0] = GetMsg(MRoutesDiskMenuString);

	info->PluginMenuStrings = pluginMenuStrings;
	info->PluginMenuStringsNumber = (int)interfacesAddToPluginsMenu + (int)routesAddToPluginsMenu;

	pluginConfigStrings[0] = GetMsg(MPluginConfigString);
	info->PluginConfigStrings = pluginConfigStrings;
	info->PluginConfigStringsNumber = ARRAYSIZE(pluginConfigStrings);

	info->CommandPrefix = 0;
}

static const int DIALOG_WIDTH = 78;

enum {
	WinCfgCaptionIndex,
	WinCfgInterfacesTextIndex,
	WinCfgAddDiskMenuIndex,
	WinCfgAddPluginsMenuIndex,
	WinCfgSeparator1Index,
	WinCfgRoutesTextIndex,
	WinCfgAddRoutesDiskMenuIndex,
	WinCfgAddRoutesPluginsMenuIndex,
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	WinCfgMcInetIndex,
	WinCfgMcInet6Index,
	#endif
	WinCfgSeparator2Index,
	WinCfgEanbleLogIndex,
	WinCfgEanbleLogStoreIndex,
	WinCfgEanbleLogEditIndex,
	WinCfgConfigSaveSettingsCheckboxIndex,
	WinCfgConfigSaveSettingsCheckboxStoreIndex,
	WinCfgMaxIndex
};

LONG_PTR WINAPI CfgDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	if( msg == DN_DRAWDLGITEM ) {
		if( param1 == WinCfgEanbleLogIndex ) {
			ShowHideElements(hDlg,
				WinCfgEanbleLogIndex,
				WinCfgEanbleLogStoreIndex,
				WinCfgEanbleLogEditIndex,
				WinCfgEanbleLogEditIndex);
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		} else if( param1 == WinCfgMcInetIndex ) {
			ShowHideElements(hDlg,
				WinCfgConfigSaveSettingsCheckboxIndex,
				WinCfgConfigSaveSettingsCheckboxStoreIndex,
				WinCfgMcInetIndex,
				WinCfgMcInet6Index);
		#endif
		}
	}
	return NetCfgPlugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

int PluginCfg::Configure(int itemNumber)
{
	LOG_INFO("itemNumber = %d\n", itemNumber);
	bool change = false;

	static const DlgConstructorItem dci[] = {
		{DI_DOUBLEBOX,false,  DEFAUL_X_START, DIALOG_WIDTH-4, 0, {.lngIdstring = MConfigPluginSettings}},
		{DI_TEXT,     true,   5,  0, 0, {.lngIdstring = MInterfacesDiskMenuString}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MAddDisksMenu}},
		{DI_CHECKBOX, false, 40,  0, 0, {.lngIdstring = MAddToPluginsMenu}},
		{DI_TEXT,     true,   5,  0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_TEXT,     true,   5,  0, 0, {.lngIdstring = MRoutesDiskMenuString}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MAddDisksMenu}},
		{DI_CHECKBOX, false, 40,  0, 0, {.lngIdstring = MAddToPluginsMenu}},
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		{DI_CHECKBOX,  true,   5, 0, DIF_DISABLE, {.ptrData = L"Enable Multicast IPv4 panel"}},
		{DI_CHECKBOX,  false,  40, 0, DIF_DISABLE, {.ptrData = L"Enable Multicast IPv6 panel"}},
		#endif
		{DI_TEXT,     true,   5,  0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MEnableLog}},
		/* Store */{DI_CHECKBOX, false,  80,  0, DIF_HIDDEN, {0}},
		{DI_EDIT,     false, 21, DIALOG_WIDTH-6, 0, {0}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MConfigSaveSettings}},
		/* Store */{DI_CHECKBOX, false,  80,  0, DIF_HIDDEN, {0}},
		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dci[0]);

	fdc.SetSelected(WinCfgAddDiskMenuIndex, interfacesAddToDisksMenu);
	fdc.SetSelected(WinCfgAddPluginsMenuIndex, interfacesAddToPluginsMenu);

	fdc.SetSelected(WinCfgAddRoutesDiskMenuIndex, routesAddToDisksMenu);
	fdc.SetSelected(WinCfgAddRoutesPluginsMenuIndex, routesAddToPluginsMenu);

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	fdc.SetSelected(WinCfgMcInetIndex, mcinetPanelValid);
	fdc.SetSelected(WinCfgMcInet6Index, mcinet6PanelValid);
	#endif

	fdc.SetSelected(WinCfgEanbleLogIndex, logEnable);

	std::string _logfile(LOG_FILE);
	std::wstring logfile(_logfile.begin(), _logfile.end());
	fdc.SetText(WinCfgEanbleLogEditIndex, logfile.c_str());
	fdc.SetX(WinCfgEanbleLogEditIndex, wcslen(GetMsg(MEnableLog))+9);

	if( !logEnable )
		fdc.OrFlags(WinCfgEanbleLogEditIndex, DIF_DISABLE);

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	fdc.SetHelpTopic(L"Config");

	FarDialog dlg(&fdc, CfgDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		for( auto & item : chlst ) {
			switch(item.itemNum) {
			case WinCfgAddDiskMenuIndex:
				interfacesAddToDisksMenu = bool(item.newVal.Selected);
				break;
			case WinCfgAddPluginsMenuIndex:
				interfacesAddToPluginsMenu = bool(item.newVal.Selected);
				break;
			case WinCfgAddRoutesDiskMenuIndex:
				routesAddToDisksMenu = bool(item.newVal.Selected);
				break;
			case WinCfgAddRoutesPluginsMenuIndex:
				routesAddToPluginsMenu = bool(item.newVal.Selected);
				break;
			#if !defined(__APPLE__) && !defined(__FreeBSD__)
			case WinCfgMcInetIndex:
				mcinetPanelValid = bool(item.newVal.Selected);
				break;
			case WinCfgMcInet6Index:
				mcinet6PanelValid = bool(item.newVal.Selected);
				break;
			#endif
			case WinCfgEanbleLogEditIndex:
				logfile = item.newVal.ptrData;
				if( logfile.size() < (LOG_MAX_PATH-1) && logfile.size() >= sizeof("/a") ) {
					_logfile = Wide2MB( logfile.c_str() );
					memmove(initial_log, _logfile.c_str(), _logfile.size()+1);
				}
				break;
			case WinCfgEanbleLogIndex:
				logEnable = bool(item.newVal.Selected);
				if( !logEnable )
					memmove(initial_log, "/dev/null", sizeof("/dev/null"));
				break;
			case WinCfgConfigSaveSettingsCheckboxIndex:
				SaveConfig();
				break;
			};
		}
	}
	return change;
}
