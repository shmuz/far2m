#ifndef __NETFARPANEL_H__
#define __NETFARPANEL_H__

#include "farpanel.h"
#include <memory>
#include <map>

class NetFarPanel : public FarPanel {
private:
	std::map<uint32_t, std::wstring> & ifs;
public:

	bool SelectInterface(HANDLE hDlg, uint32_t setIndex, uint32_t *ifindex);
	const wchar_t * GetInterfaceByIndex(uint32_t ifindex);
	uint8_t SelectFamily(HANDLE hDlg, uint32_t setIndex);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	uint8_t SelectProto(HANDLE hDlg, uint32_t setIndex, uint8_t old_proto);
	uint32_t SelectTable(HANDLE hDlg, uint32_t setIndex, uint32_t old_table);
#endif

	explicit NetFarPanel(uint32_t index, std::map<uint32_t, std::wstring> & ifs);
	virtual ~NetFarPanel();
};

#endif /* __NETFARPANEL_H__ */
