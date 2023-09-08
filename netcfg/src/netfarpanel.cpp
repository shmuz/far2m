#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "netcfgplugin.h"
#include "netfarpanel.h"
#include "fardialog.h"

#include <utils.h>

#include <assert.h>

#include <common/log.h>
#include <common/errname.h>
#include <common/netutils.h>

#include <sys/types.h>
#include <sys/socket.h>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "netfarpanel.cpp"

NetFarPanel::NetFarPanel(uint32_t index_, std::map<uint32_t, std::wstring> & ifs_):
	FarPanel(index_),
	ifs(ifs_)
{
	LOG_INFO("\n");
}

NetFarPanel::~NetFarPanel()
{
	LOG_INFO("\n");
}

const wchar_t * NetFarPanel::GetInterfaceByIndex(uint32_t ifindex)
{
		auto it = ifs.find(ifindex);
		if( it != ifs.end() )
			return it->second.c_str();
		return L"";
}

bool NetFarPanel::SelectInterface(HANDLE hDlg, uint32_t setIndex, uint32_t *ifindex)
{
	int if_count = ifs.size()+1, index = 0;
	auto menuElements = std::make_unique<FarMenuItem[]>(if_count);
	memset(menuElements.get(), 0, sizeof(FarMenuItem)*if_count);
	for( const auto& [if_index, name] : ifs ) {
		menuElements[index].Text = name.c_str();
		index++;
	}

	menuElements[index].Text = L"";
	index++;

	menuElements[0].Selected = 1;
	index = NetCfgPlugin::psi.Menu(NetCfgPlugin::psi.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, \
			 L"Select interface:", 0, L"hrd", nullptr, nullptr, menuElements.get(), if_count);

	if( index >= 0 && index < if_count ) {
		NetCfgPlugin::psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, setIndex, (LONG_PTR)menuElements[index].Text);
		for( auto & [ if_index, name ]: ifs ) {
			if( name == menuElements[index].Text ) {
				if( ifindex )
					*ifindex = if_index;
				return true;
			}
		}
	}
	return false;
}

uint8_t NetFarPanel::SelectFamily(HANDLE hDlg, uint32_t setIndex)
{
	static const wchar_t * menuElements[] = {
		L"inet",
		L"inet6",
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
		L"link",
		L"mpls",
		L"bridge",
		L""
	#endif
		};
	auto index = Select(hDlg, &menuElements[0], ARRAYSIZE(menuElements), setIndex);
	static const uint8_t family[] = {
		AF_INET,
		AF_INET6,
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
		AF_PACKET,
		AF_MPLS,
		AF_BRIDGE,
	#else
		AF_LINK,
	#endif
		AF_UNSPEC
	};

	if( index >= 0 && index < static_cast<int>(ARRAYSIZE(family)) )
		return family[index];

	return AF_UNSPEC;
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
uint8_t NetFarPanel::SelectProto(HANDLE hDlg, uint32_t setIndex, uint8_t old_proto)
{
		static const wchar_t * menuElements[] = {
				L"unspec",
				L"redirect",
				L"kernel",
				L"boot",
				L"static",
				L"dhcp",
				L"gated",
				L"ra",
				L"mrt",
				L"zebra",
				L"bird",
				L"dnrouted",
				L"xorp",
				L"ntk",
				L"keepalived",
				L"babel",
				L"bgp",
				L"isis",
				L"ospf",
				L"rip",
				L"eigrp",
				L"Enter:" };
		static const uint8_t type[] = {
				RTPROT_UNSPEC,
				RTPROT_REDIRECT,
				RTPROT_KERNEL,
				RTPROT_BOOT,	
				RTPROT_STATIC,	
				RTPROT_DHCP,	
				RTPROT_GATED,	
				RTPROT_RA,	
				RTPROT_MRT,	
				RTPROT_ZEBRA,	
				RTPROT_BIRD,	
				RTPROT_DNROUTED,
				RTPROT_XORP,	
				RTPROT_NTK,	
				RTPROT_KEEPALIVED,
				RTPROT_BABEL,	
				RTPROT_BGP,	
				RTPROT_ISIS,	
				RTPROT_OSPF,	
				RTPROT_RIP,	
				RTPROT_EIGRP };
			auto num = SelectNum(hDlg, &menuElements[0], ARRAYSIZE(menuElements), L"Proto:", setIndex);
			if( num >= 0 && num < static_cast<int>(ARRAYSIZE(type)) )
				return type[num];
			if( num == ARRAYSIZE(type) )
				return (uint8_t)GetNumberItem(hDlg, setIndex);
			return old_proto;
}

uint32_t NetFarPanel::SelectTable(HANDLE hDlg, uint32_t setIndex, uint32_t old_table)
{
	static const wchar_t * menuElements[] = {
		L"default",
		L"main",
		L"local",
		L"",
		L"Enter:"
		};
	auto table_new = SelectNum(hDlg, &menuElements[0], ARRAYSIZE(menuElements), L"Table:", setIndex);
	if( table_new >= 0 && table_new <= 2 )
		return table_new + RT_TABLE_DEFAULT;
	else if(table_new == 3)
		return 0;
	else if(table_new == 4)
		return (uint32_t)GetNumberItem(hDlg, setIndex);
	return old_table;
}
#endif