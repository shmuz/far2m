#ifndef __NETROUTES_H__
#define __NETROUTES_H__

#include "netroute.h"
#include <deque>
#include <map>

struct NetRoutes {

	std::map<uint32_t, std::wstring> ifs;
	std::deque<ArpRouteInfo> arp;
	std::deque<IpRouteInfo> inet;
	std::deque<IpRouteInfo> inet6;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	std::deque<IpRouteInfo> mcinet;
	std::deque<IpRouteInfo> mcinet6;
	std::deque<RuleRouteInfo> rule;
	std::deque<RuleRouteInfo> rule6;
	std::deque<RuleRouteInfo> mcrule;
	std::deque<RuleRouteInfo> mcrule6;
#endif

	bool ipv4_forwarding;
	bool ipv6_forwarding;

	NetRoutes();
	~NetRoutes();

	bool SetIpForwarding(bool on);
	bool SetIp6Forwarding(bool on);

	bool Update(void);
	void Log(void);
	void Clear(void);

#if !defined(__APPLE__) && !defined(__FreeBSD__)
protected:
	bool UpdateByProcNet(void);
	bool UpdateByNetlink(void);
	bool UpdateNeigbours(const NeighborRecord * nb);
private:
	void SetNameByIndex(const char *ifname, uint32_t index);
	bool UpdateByNetlink(void * netlink, unsigned char af_family);
#else
protected:
	bool UpdateInterfaces(void);
#endif
};

#endif /* __NETROUTES_H__ */
