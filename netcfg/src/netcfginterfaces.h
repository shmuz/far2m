#ifndef __NETCFGINTERFACES_H__
#define __NETCFGINTERFACES_H__

#include "farpanel.h"
#include "netif/netifs.h"
#include <memory>

enum {
	InterfaceColumnIpIndex,
	InterfaceColumnMacIndex,
	InterfaceColumnIp6Index,
	InterfaceColumnMtuIndex,
	InterfaceColumnRecvBytesIndex,
	InterfaceColumnSendBytesIndex,
	InterfaceColumnRecvPktsIndex,
	InterfaceColumnSendPktsIndex,
	InterfaceColumnRecvErrsIndex,
	InterfaceColumnSendErrsIndex,
	InterfaceColumnMulticastIndex,
	InterfaceColumnCollisionsIndex,
	InterfaceColumnPermanentMacIndex,
	InterfaceColumnMaxIndex
};

class NetcfgInterfaces : public FarPanel
{
private:
	std::unique_ptr<NetInterfaces> nifs;

	bool change;
	
	// copy and assignment not allowed
	NetcfgInterfaces(const NetcfgInterfaces&) = delete;
	void operator=(const NetcfgInterfaces&) = delete;

	friend LONG_PTR WINAPI SettingInterfaceDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);
	bool EditInterface(NetInterface * net_if);

	void SelectInterface(HANDLE hDlg, int * elementSet, size_t total, bool emptyEntry = false);
	friend LONG_PTR WINAPI SettingTcpdumpDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);
	bool TcpdumpInterface(NetInterface * net_if);
	bool TcpdumpStop();
public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	explicit NetcfgInterfaces(PanelIndex index);
	virtual ~NetcfgInterfaces();
};


#endif // __NETCFGINTERFACES_H__