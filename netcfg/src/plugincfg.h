#ifndef __CONFIGPLUGIN_H__
#define __CONFIGPLUGIN_H__

#include <farplug-wide.h>
#include <farkeys.h>
#include <map>
#include "netcfglng.h"

enum {
	PanelModeBrief,
	PanelModeMedium,
	PanelModeFull,
	PanelModeWide,
	PanelModeDetailed,
	PanelModeDiz,
	PanelModeLongDiz,
	PanelModeOwners,
	PanelModeLinks,
	PanelModeAlternative,
	PanelModeMax,
};

typedef struct {
	const wchar_t * statusColumnTypes;
	const wchar_t * statusColumnWidths;
	const wchar_t * columnTypes[2];
	const wchar_t * columnWidths[2];
	const wchar_t * columnTitles[2][15];
	uint32_t keyBarTitles[12];
	uint32_t keyBarShiftTitles[12];
	uint32_t panelTitle;
	uint32_t format;
	uint32_t flags;
} CfgDefaults;

typedef enum {
	IfcsPanelIndex,
	RouteInetPanelIndex,
	RouteInet6PanelIndex,
	RouteArpPanelIndex,
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	RouteMcInetPanelIndex,
	RouteMcInet6PanelIndex,
	RouteRuleInetPanelIndex,
	RouteIpTablesPanelIndex,
	#endif
	MaxPanelIndex
} PanelIndex;


class PluginCfg {

	private:
		static std::map<PanelIndex, CfgDefaults> def;
		static size_t init;

		const char * GetPanelName(PanelIndex index) const;

		static bool logEnable;

		const wchar_t * GetMsg(int msgId);

		friend LONG_PTR WINAPI CfgDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);

		void SaveConfig(void) const;

	public:
		explicit PluginCfg();
		~PluginCfg();

		static bool interfacesAddToDisksMenu;
		static bool interfacesAddToPluginsMenu;

		static bool routesAddToDisksMenu;
		static bool routesAddToPluginsMenu;

		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		static bool mcinetPanelValid;
		static bool mcinet6PanelValid;
		#endif

		void FillPanelData(struct PanelData * data, PanelIndex index);
		void ReloadPanelString(struct PanelData * data, PanelIndex index);

		void GetPluginInfo(struct PluginInfo *info);
		int Configure(int itemNumber);
};

#endif /* __CONFIGPLUGIN_H__ */
