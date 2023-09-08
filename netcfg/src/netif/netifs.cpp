#include "netifs.h"
#include "netifset.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/sizestr.h>
#include <common/netutils.h>
#include <common/netlink.h>

// Apple MacOS, FreeBSD and Linux has ifaddrs.h
extern "C" {
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
}

#ifndef MAIN_NETIF
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

#define LOG_SOURCE_FILE "netifs.cpp"

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <netpacket/packet.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/snmp.h>

#else // __APPLE__
#include <net/if_dl.h>
#endif

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

static std::wstring get_sockaddr_str(const struct sockaddr *sa, bool ismask = false)
{
	const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
	char s[maxlen+1] = {0};
	std::string addr;

	if( !sa )
		return std::wstring();

	switch( sa->sa_family ) {
	case AF_INET:
	{
		if( inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen) )
			addr = s;
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 sa6 = *((const struct sockaddr_in6 *)sa);
		if( inet_ntop(AF_INET6, &sa6.sin6_addr, s, maxlen) ) {
			if( ismask ) {
				int bits = 0;
				for( int i = 0; i < 16; i++ ) {
					while (sa6.sin6_addr.s6_addr[i]) {
						bits += sa6.sin6_addr.s6_addr[i] & 1;
						sa6.sin6_addr.s6_addr[i] >>= 1;
					}
				}
				if( snprintf(s, maxlen, "%u", bits) <= 0 )
					s[0] = 0;
			}
			addr = s;
		}
		break;
	}
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	case AF_PACKET:
	{
		struct sockaddr_ll * sll = (struct sockaddr_ll *)sa;
		unsigned char * mac = (unsigned char *)sll->sll_addr;
#else
	case AF_LINK:
	{
		struct sockaddr_dl * sdl = (struct sockaddr_dl *)sa;
		unsigned char * mac = (unsigned char *)sdl->sdl_data + sdl->sdl_nlen;
#endif
		if( snprintf(s, maxlen, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) > 0 )
			addr = s;
		break;
	}
	default:
		LOG_ERROR("unsupported sa_family: %u\n", sa->sa_family);
		break;
	}
	return std::wstring(addr.begin(), addr.end());
}

NetInterface * NetInterfaces::Add(const char * name)
{
	std::string _name(name);
	std::wstring _wname(_name.begin(), _name.end());
	if( ifs.find(_wname) == ifs.end() ) {
		ifs[_wname] = new NetInterface(_wname);
		ifs[_wname]->UpdateStats();
	}
	return ifs[_wname];
}

NetInterface * NetInterfaces::FindByIndex(uint32_t ifindex)
{
	for( auto & [name, net_if] : ifs ) {
		if( net_if->ifindex == ifindex )
			return net_if;
	}
	return 0;
}

void NetInterfaces::Clear(void) {
	for( const auto& [name, net_if] : ifs )
		delete net_if;
	ifs.clear();
}

bool NetInterfaces::UpdateByProcNet(void)
{
	struct ifaddrs *ifa_base, *ifa;
	if( getifaddrs(&ifa_base) != 0 ) {
		LOG_ERROR("getifaddrs(&ifa_base) ... error (%s) ", errorname(errno));
		return false;
	}

	Clear();

	for (ifa = ifa_base; ifa; ifa = ifa->ifa_next) {

		if( !ifa->ifa_name ) {
			LOG_ERROR("skip interface: %p\n", ifa->ifa_addr);
			continue;
		}

		NetInterface * net_if = Add(ifa->ifa_name);
		net_if->ifindex = if_nametoindex(ifa->ifa_name);

		if( !ifa->ifa_addr ) {
			MacAddressInfo mac;
			mac.flags = ifa->ifa_flags;
			net_if->ifa_flags = ifa->ifa_flags;
			net_if->mac[L""] = mac;
			net_if->permanent_mac = L"";
			continue;
		}

		IpAddressInfo ip;
		ip.flags = 0;
		ip.prefixlen = 0;
		ip.scope = 0;
		ip.family = ifa->ifa_addr ? ifa->ifa_addr->sa_family:AF_UNSPEC;

		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		ip.rt_priority = 0;
		ip.netnsid = 0;
		ip.proto = 0;
		memset(&ip.cacheinfo, 0, sizeof(ip.cacheinfo));
		#endif

		ip.ip = get_sockaddr_str((const struct sockaddr *)ifa->ifa_addr);
		if( ifa->ifa_netmask ) {
			ifa->ifa_netmask->sa_family = ifa->ifa_addr->sa_family;
			ip.netmask = get_sockaddr_str((const struct sockaddr *)ifa->ifa_netmask, true);
			ip.prefixlen = ifa->ifa_netmask->sa_family == AF_INET ? \
			IpMaskToBits(std::string(ip.netmask.begin(),ip.netmask.end()).c_str()):\
			Ip6MaskToBits(std::string(ip.netmask.begin(),ip.netmask.end()).c_str());
		}
		if( ifa->ifa_flags & IFF_BROADCAST && ifa->ifa_broadaddr ) {
			ifa->ifa_broadaddr->sa_family = ifa->ifa_addr->sa_family;
			ip.broadcast = get_sockaddr_str((const struct sockaddr *)ifa->ifa_broadaddr);
		} else if( ifa->ifa_flags & IFF_POINTOPOINT && ifa->ifa_dstaddr ) {
			ifa->ifa_dstaddr->sa_family = ifa->ifa_addr->sa_family;
			ip.point_to_point = get_sockaddr_str((const struct sockaddr *)ifa->ifa_dstaddr);
		}

		switch( ip.family ) {

		case AF_INET:
			net_if->ip[ip.ip] = ip;
			break;
		case AF_INET6:
			net_if->ip6[ip.ip] = ip;
			break;
		#if !defined(__APPLE__) && !defined(__FreeBSD__)
		case AF_PACKET:
		#else
		case AF_LINK:
		#endif
		{
			MacAddressInfo mac;

			// get_sockaddr_str can output mac addresses - we can use some data
			mac.flags = ifa->ifa_flags;
			net_if->ifa_flags = mac.flags;
			if( net_if->IsLoopback() )
				net_if->type = ARPHRD_LOOPBACK;
			else if( net_if->IsPointToPoint() && net_if->IsNoARP() )
				net_if->type = ARPHRD_NONE;
			else
				net_if->type = ARPHRD_ETHER;

			mac.mac = ip.ip;
			mac.broadcast = ip.broadcast;
			net_if->mac[mac.mac] = mac;

			net_if->permanent_mac = mac.mac;
			break;
		}
		default:
			LOG_ERROR("unsupported sa_family %u\n", ifa->ifa_addr->sa_family);
			break;
		}
	}

	freeifaddrs(ifa_base);
	return true;
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
//static_assert( IFLA_MAX == 64 );
std::wstring MacFromData(const unsigned char * mac)
{
	char s[sizeof("FF:FF:FF:FF:FF:FF")] = {0};
	if( snprintf(s, sizeof(s), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) > 0 )
		return std::wstring(s, s + strlen(s));
	return std::wstring();
}
// towstr can use only for ASCII symbols
static std::wstring towstr(const char * name)
{
	std::string _s(name);
	return std::wstring(_s.begin(), _s.end());
}

static void copystats64(NetInterface * net_if, struct rtnl_link_stats64 * stat64)
{
	net_if->send_packets = stat64->tx_packets;
	net_if->recv_packets = stat64->rx_packets;
	net_if->send_bytes = stat64->tx_bytes;
	net_if->recv_bytes = stat64->rx_bytes;
	net_if->send_errors = stat64->tx_errors;
	net_if->recv_errors = stat64->rx_errors;
	net_if->collisions = stat64->collisions;
	net_if->multicast = stat64->multicast;
}

static void getqos(struct rtattr *rta, int len, std::wstring & result)
{
	while( RTA_OK(rta, len) ) {
		char s[sizeof("4294967295:4294967295 ")] = {0};
		LOG_INFO("type: %u (%s) size %u\n", rta->rta_type, rta->rta_type == IFLA_VLAN_QOS_MAPPING ? "IFLA_VLAN_QOS_MAPPING":"", rta->rta_len);
		if( rta->rta_type == IFLA_VLAN_QOS_MAPPING ) {
			struct ifla_vlan_qos_mapping * vlanm = (struct ifla_vlan_qos_mapping *)RTA_DATA(rta);
			if( snprintf(s, sizeof(s), "%u:%u ", vlanm->from, vlanm->to) > 0 )
				result += towstr(s);
		}
		rta = RTA_NEXT(rta, len);
	}
}

bool NetInterfaces::UpdateByNetlink(void)
{
	bool res = false;
	void * netlink = OpenNetlink();
	if(!netlink)
		return false;

	do {

		const LinkRecord * lr = GetLinks(netlink);
		if( !lr )
			break;

		Clear();

		while( lr->ifm ) {
			NetInterface * net_if = 0;

			LOG_INFO("-------------------------------------\n");
			LOG_INFO("nlh.nlmsg_type: %d (%s)\n", lr->nlm.nlmsg_type, nlmsgtype(lr->nlm.nlmsg_type));
			LOG_INFO("rtm_family:   %u (%s)\n", lr->ifm->ifi_family, familyname(lr->ifm->ifi_family));
			LOG_INFO("ifi_type:     %u (%s)\n", lr->ifm->ifi_type, arphdrname(lr->ifm->ifi_type)); // ARPHRD_
			LOG_INFO("ifi_index:    %u\n", lr->ifm->ifi_index); // Link index
			LOG_INFO("ifi_flags:    0x%08X (%s)\n", lr->ifm->ifi_flags, ifflagsname(lr->ifm->ifi_flags)); // IFF_* flags
			LOG_INFO("ifi_change:   0x%08X\n", lr->ifm->ifi_change); // IFF_* change mask

			if( lr->tb[IFLA_IFNAME] && RTA_PAYLOAD(lr->tb[IFLA_IFNAME]) >= sizeof(uint8_t) ) {
				LOG_INFO("IFLA_IFNAME:              %s\n", (const char *)RTA_DATA(lr->tb[IFLA_IFNAME]));
				net_if = Add((const char *)RTA_DATA(lr->tb[IFLA_IFNAME]));
				LOG_INFO("IFLA_IFNAME:              %s\n", (const char *)RTA_DATA(lr->tb[IFLA_IFNAME]));
			}

			if( net_if ) {
				MacAddressInfo mac;

				net_if->ifindex = lr->ifm->ifi_index;
				net_if->type = lr->ifm->ifi_type;
				mac.flags = lr->ifm->ifi_flags;
				net_if->ifa_flags = mac.flags;

				if( lr->tb[IFLA_IFALIAS] && RTA_PAYLOAD(lr->tb[IFLA_IFALIAS]) >= sizeof(uint8_t) ) {
					net_if->osdep.alias = towstr((const char *)RTA_DATA(lr->tb[IFLA_IFALIAS]));
					LOG_INFO("IFLA_IFALIAS:             %S\n", net_if->osdep.alias.c_str());
				}

				if( lr->tb[IFLA_CARRIER] && RTA_PAYLOAD(lr->tb[IFLA_CARRIER]) >= sizeof(uint8_t) ) {
					net_if->osdep.carrier = RTA_UINT8_T(lr->tb[IFLA_CARRIER]);
					LOG_INFO("IFLA_CARRIER:             %d (%s)\n", net_if->osdep.carrier, net_if->osdep.carrier ? "IF_CARRIER_UP":"IF_CARRIER_DOWN");
				}
				if( lr->tb[IFLA_CARRIER_CHANGES] && RTA_PAYLOAD(lr->tb[IFLA_CARRIER_CHANGES]) >= sizeof(uint32_t) ) {
					net_if->osdep.carrier_changes = RTA_UINT32_T(lr->tb[IFLA_CARRIER_CHANGES]);
					LOG_INFO("IFLA_CARRIER_CHANGES:     %u\n", net_if->osdep.carrier_changes);
				}
				if( lr->tb[IFLA_CARRIER_UP_COUNT] && RTA_PAYLOAD(lr->tb[IFLA_CARRIER_UP_COUNT]) >= sizeof(uint32_t) ) {
					net_if->osdep.carrier_up_count = RTA_UINT32_T(lr->tb[IFLA_CARRIER_UP_COUNT]);
					LOG_INFO("IFLA_CARRIER_UP_COUNT:    %u\n", net_if->osdep.carrier_up_count);
				}
				if( lr->tb[IFLA_CARRIER_DOWN_COUNT] && RTA_PAYLOAD(lr->tb[IFLA_CARRIER_DOWN_COUNT]) >= sizeof(uint32_t) ) {
					net_if->osdep.carrier_down_count = RTA_UINT32_T(lr->tb[IFLA_CARRIER_DOWN_COUNT]);
					LOG_INFO("IFLA_CARRIER_DOWN_COUNT:  %u\n", net_if->osdep.carrier_down_count);
				}

				if( lr->tb[IFLA_TXQLEN] && RTA_PAYLOAD(lr->tb[IFLA_TXQLEN]) >= sizeof(uint32_t) ) {
					net_if->osdep.txqueuelen = RTA_UINT32_T(lr->tb[IFLA_TXQLEN]);
					LOG_INFO("IFLA_TXQLEN:              %u\n", net_if->osdep.txqueuelen);
				}
				if( lr->tb[IFLA_OPERSTATE] && RTA_PAYLOAD(lr->tb[IFLA_OPERSTATE]) >= sizeof(uint8_t) ) {
					net_if->osdep.operstate = RTA_UINT8_T(lr->tb[IFLA_OPERSTATE]);
					LOG_INFO("IFLA_OPERSTATE:           %u (%s)\n", net_if->osdep.operstate, ifoperstate(net_if->osdep.operstate));
				}
				if( lr->tb[IFLA_LINKMODE] && RTA_PAYLOAD(lr->tb[IFLA_LINKMODE]) >= sizeof(uint8_t) ) {
					net_if->osdep.link_mode = RTA_UINT8_T(lr->tb[IFLA_LINKMODE]);
					LOG_INFO("IFLA_LINKMODE:            %u (%s)\n", net_if->osdep.operstate, iflinkmode(net_if->osdep.link_mode));
				}
				if( lr->tb[IFLA_MTU] && RTA_PAYLOAD(lr->tb[IFLA_MTU]) >= sizeof(uint32_t) ) {
					net_if->mtu = RTA_UINT32_T(lr->tb[IFLA_MTU]);
					LOG_INFO("IFLA_MTU:                 %u\n", net_if->mtu);
				}
				if( lr->tb[IFLA_MIN_MTU] && RTA_PAYLOAD(lr->tb[IFLA_MIN_MTU]) >= sizeof(uint32_t) ) {
					net_if->osdep.minmtu = RTA_UINT32_T(lr->tb[IFLA_MIN_MTU]);
					LOG_INFO("IFLA_MIN_MTU:             %u\n", net_if->osdep.minmtu);
				}
				if( lr->tb[IFLA_MAX_MTU] && RTA_PAYLOAD(lr->tb[IFLA_MAX_MTU]) >= sizeof(uint32_t) ) {
					net_if->osdep.maxmtu = RTA_UINT32_T(lr->tb[IFLA_MAX_MTU]);
					LOG_INFO("IFLA_MAX_MTU:             %u\n", net_if->osdep.maxmtu);
				}
				if( lr->tb[IFLA_GROUP] && RTA_PAYLOAD(lr->tb[IFLA_GROUP]) >= sizeof(uint32_t) ) {
					net_if->osdep.group = RTA_UINT32_T(lr->tb[IFLA_GROUP]);
					LOG_INFO("IFLA_GROUP:               %u %s\n", net_if->osdep.group, net_if->osdep.group ? "":"(default)");
				}
				if( lr->tb[IFLA_PROMISCUITY] && RTA_PAYLOAD(lr->tb[IFLA_PROMISCUITY]) >= sizeof(uint32_t) ) {
					net_if->osdep.promiscuity = RTA_UINT32_T(lr->tb[IFLA_PROMISCUITY]);
					/*if( net_if->osdep.promiscuity )
						net_if->ifa_flags |= IFF_PROMISC;
					else
						net_if->ifa_flags &= ~IFF_PROMISC;*/
					mac.flags = net_if->ifa_flags;
					LOG_INFO("IFLA_PROMISCUITY:         %u %s\n", net_if->osdep.promiscuity, net_if->osdep.promiscuity ? "on":"off");
				}
				if( lr->tb[IFLA_GSO_MAX_SEGS] && RTA_PAYLOAD(lr->tb[IFLA_GSO_MAX_SEGS]) >= sizeof(uint32_t) ) {
					net_if->osdep.gso_max_segs = RTA_UINT32_T(lr->tb[IFLA_GSO_MAX_SEGS]);
					LOG_INFO("IFLA_GSO_MAX_SEGS:        %u\n", net_if->osdep.gso_max_segs);
				}
				if( lr->tb[IFLA_GSO_MAX_SIZE] && RTA_PAYLOAD(lr->tb[IFLA_GSO_MAX_SIZE]) >= sizeof(uint32_t) ) {
					net_if->osdep.gso_max_size = RTA_UINT32_T(lr->tb[IFLA_GSO_MAX_SIZE]);
					LOG_INFO("IFLA_GSO_MAX_SIZE:        %u\n", net_if->osdep.gso_max_size);
				}
				if( lr->tb[IFLA_GSO_IPV4_MAX_SIZE] && RTA_PAYLOAD(lr->tb[IFLA_GSO_IPV4_MAX_SIZE]) >= sizeof(uint32_t) ) {
					net_if->osdep.gso_ipv4_max_size = RTA_UINT32_T(lr->tb[IFLA_GSO_IPV4_MAX_SIZE]);
					LOG_INFO("IFLA_GSO_IPV4_MAX_SIZE:   %u\n", net_if->osdep.gso_ipv4_max_size);
				}
				if( lr->tb[IFLA_GRO_MAX_SIZE] && RTA_PAYLOAD(lr->tb[IFLA_GRO_MAX_SIZE]) >= sizeof(uint32_t) ) {
					net_if->osdep.gro_max_size = RTA_UINT32_T(lr->tb[IFLA_GRO_MAX_SIZE]);
					LOG_INFO("IFLA_GRO_MAX_SIZE:        %u\n", net_if->osdep.gro_max_size);
				}
				if( lr->tb[IFLA_GRO_IPV4_MAX_SIZE] && RTA_PAYLOAD(lr->tb[IFLA_GRO_IPV4_MAX_SIZE]) >= sizeof(uint32_t) ) {
					net_if->osdep.gro_ipv4_max_size = RTA_UINT32_T(lr->tb[IFLA_GRO_IPV4_MAX_SIZE]);
					LOG_INFO("IFLA_GRO_IPV4_MAX_SIZE:   %u\n", net_if->osdep.gro_ipv4_max_size);
				}
				if( lr->tb[IFLA_TSO_MAX_SEGS] && RTA_PAYLOAD(lr->tb[IFLA_TSO_MAX_SEGS]) >= sizeof(uint32_t) ) {
					net_if->osdep.tso_max_segs = RTA_UINT32_T(lr->tb[IFLA_TSO_MAX_SEGS]);
					LOG_INFO("IFLA_TSO_MAX_SEGS:        %u\n", net_if->osdep.tso_max_segs);
				}
				if( lr->tb[IFLA_TSO_MAX_SIZE] && RTA_PAYLOAD(lr->tb[IFLA_TSO_MAX_SIZE]) >= sizeof(uint32_t) ) {
					net_if->osdep.tso_max_size = RTA_UINT32_T(lr->tb[IFLA_TSO_MAX_SIZE]);
					LOG_INFO("IFLA_TSO_MAX_SIZE:        %u\n", net_if->osdep.tso_max_size);
				}
				if( lr->tb[IFLA_NUM_TX_QUEUES] && RTA_PAYLOAD(lr->tb[IFLA_NUM_TX_QUEUES]) >= sizeof(uint32_t) ) {
					net_if->osdep.numtxqueues = RTA_UINT32_T(lr->tb[IFLA_NUM_TX_QUEUES]);
					LOG_INFO("IFLA_NUM_TX_QUEUES:       %u\n", net_if->osdep.numtxqueues);
				}
				if( lr->tb[IFLA_NUM_RX_QUEUES] && RTA_PAYLOAD(lr->tb[IFLA_NUM_RX_QUEUES]) >= sizeof(uint32_t) ) {
					net_if->osdep.numrxqueues = RTA_UINT32_T(lr->tb[IFLA_NUM_RX_QUEUES]);
					LOG_INFO("IFLA_NUM_RX_QUEUES:       %u\n", net_if->osdep.numrxqueues);
				}
				if( lr->tb[IFLA_ADDRESS] && RTA_PAYLOAD(lr->tb[IFLA_ADDRESS]) >= ETH_ALEN ) {
					mac.mac = MacFromData((unsigned char *)RTA_DATA(lr->tb[IFLA_ADDRESS]));
					LOG_INFO("IFLA_ADDRESS:             %S\n", mac.mac.c_str());
				}
				if( lr->tb[IFLA_BROADCAST] && RTA_PAYLOAD(lr->tb[IFLA_BROADCAST]) >= ETH_ALEN ) {
					mac.broadcast = MacFromData((unsigned char *)RTA_DATA(lr->tb[IFLA_BROADCAST]));
					LOG_INFO("IFLA_BROADCAST:           %S\n", mac.broadcast.c_str());
				}
				if( lr->tb[IFLA_PERM_ADDRESS] && RTA_PAYLOAD(lr->tb[IFLA_PERM_ADDRESS]) >= ETH_ALEN ) {
					net_if->permanent_mac = MacFromData((unsigned char *)RTA_DATA(lr->tb[IFLA_PERM_ADDRESS]));
					LOG_INFO("IFLA_PERM_ADDRESS:        %S\n", net_if->permanent_mac.c_str());
				}
				if( lr->tb[IFLA_QDISC] && RTA_PAYLOAD(lr->tb[IFLA_QDISC]) >= sizeof(uint8_t) ) {
					net_if->osdep.qdisc = towstr((const char *)RTA_DATA(lr->tb[IFLA_QDISC]));
					LOG_INFO("IFLA_QDISC:               %S\n", net_if->osdep.qdisc.c_str());
				}
				if( lr->tb[IFLA_STATS64] && RTA_PAYLOAD(lr->tb[IFLA_STATS64]) >= sizeof(struct rtnl_link_stats64) ) {
					struct rtnl_link_stats64 *stats64 = (struct rtnl_link_stats64 *)RTA_DATA(lr->tb[IFLA_STATS64]);
					memmove(&net_if->osdep.stat64, stats64, sizeof(net_if->osdep.stat64));
					LOG_INFO("--------------64-------------------\n");
					copystats64(net_if, stats64);
					net_if->LogStats();
				}
				if( !lr->tb[IFLA_STATS64] && lr->tb[IFLA_STATS] && RTA_PAYLOAD(lr->tb[IFLA_STATS]) >= sizeof(struct rtnl_link_stats) ) {
					uint32_t *stats = (uint32_t *)RTA_DATA(lr->tb[IFLA_STATS]);
					uint64_t *stats64 = (uint64_t *)&net_if->osdep.stat64;
					//static_assert( (sizeof(struct rtnl_link_stats)/8) <= (sizeof(struct rtnl_link_stats64)/8) );
					for(size_t index = 0; index < (sizeof(struct rtnl_link_stats)/8); index++)
						stats64[index] = (uint64_t)stats[index];
					copystats64(net_if, &net_if->osdep.stat64);
					LOG_INFO("--------------32-------------------\n");
					net_if->LogStats();
				}

				if( lr->tb[IFLA_XDP] ) {
					struct rtattr *xdpinfo[IFLA_XDP_MAX+1];
					LOG_INFO("PARSE########################################\n");
					if( FillAttr((struct rtattr*)RTA_DATA(lr->tb[IFLA_XDP]),
								xdpinfo,
								IFLA_XDP_MAX,
								(short unsigned int)(~NLA_F_NESTED),
								RTA_PAYLOAD(lr->tb[IFLA_XDP]),
								iflaxdptype) ) {
						if( xdpinfo[IFLA_XDP_ATTACHED] && RTA_PAYLOAD(xdpinfo[IFLA_XDP_ATTACHED]) >= sizeof(uint8_t) ) {
							net_if->osdep.xdp_attached = RTA_UINT8_T(xdpinfo[IFLA_XDP_ATTACHED]);
							LOG_INFO("IFLA_XDP_ATTACHED:   %s (%d)\n", xdpattachedtype(net_if->osdep.xdp_attached), net_if->osdep.xdp_attached);
						}
/* These are stored into IFLA_XDP_ATTACHED on dump. */
/*enum {
	XDP_ATTACHED_NONE = 0,
	XDP_ATTACHED_DRV,
	XDP_ATTACHED_SKB,
	XDP_ATTACHED_HW,
	XDP_ATTACHED_MULTI,
};


enum {
	IFLA_XDP_UNSPEC,
	IFLA_XDP_FD,
	IFLA_XDP_ATTACHED,
	IFLA_XDP_FLAGS,
	IFLA_XDP_PROG_ID,
	IFLA_XDP_DRV_PROG_ID,
	IFLA_XDP_SKB_PROG_ID,
	IFLA_XDP_HW_PROG_ID,
	IFLA_XDP_EXPECTED_FD,
	__IFLA_XDP_MAX,
};*/
					}


				}

				if( lr->tb[IFLA_LINKINFO] ) {
					struct rtattr *linkinfo[IFLA_INFO_MAX+1];
					if( FillAttr((struct rtattr*)RTA_DATA(lr->tb[IFLA_LINKINFO]),
								linkinfo,
								IFLA_INFO_MAX,
								(short unsigned int)(~NLA_F_NESTED),
								RTA_PAYLOAD(lr->tb[IFLA_LINKINFO]),
								iflainfotype) ) {
						if( linkinfo[IFLA_INFO_KIND] && RTA_PAYLOAD(linkinfo[IFLA_INFO_KIND]) >= sizeof(uint8_t) ) {
							net_if->osdep.linkinfo_kind = towstr((const char *)RTA_DATA(linkinfo[IFLA_INFO_KIND]));
							LOG_INFO("IFLA_INFO_KIND:           %S\n", net_if->osdep.linkinfo_kind.c_str());
						}

						if( linkinfo[IFLA_INFO_DATA] ) {
							if( net_if->osdep.linkinfo_kind == L"vlan" ) {
								struct rtattr * vlan[IFLA_VLAN_MAX+1];
								if( FillAttr((struct rtattr*)RTA_DATA(linkinfo[IFLA_INFO_DATA]),
										vlan,
										IFLA_VLAN_MAX,
										(short unsigned int)(~NLA_F_NESTED),
										RTA_PAYLOAD(linkinfo[IFLA_INFO_DATA]),
										iflavlantype) && \
								    vlan[IFLA_VLAN_ID] && \
								    RTA_PAYLOAD(vlan[IFLA_VLAN_ID]) >= sizeof(uint16_t)) {
									if( vlan[IFLA_VLAN_PROTOCOL] && RTA_PAYLOAD(vlan[IFLA_VLAN_PROTOCOL]) >= sizeof(uint16_t) ) {
										net_if->osdep.link.vlan.vlanprotocol = RTA_UINT16_T(vlan[IFLA_VLAN_PROTOCOL]);
										LOG_INFO("IFLA_VLAN_PROTOCOL:       0x%04X (%s)\n", net_if->osdep.link.vlan.vlanprotocol, ethprotoname(ntohs(net_if->osdep.link.vlan.vlanprotocol)));
									}
									net_if->osdep.link.vlan.vlanid = RTA_UINT16_T(vlan[IFLA_VLAN_ID]);
									LOG_INFO("IFLA_VLAN_ID:             %u\n", net_if->osdep.link.vlan.vlanid);
									if( vlan[IFLA_VLAN_FLAGS] && RTA_PAYLOAD(vlan[IFLA_VLAN_FLAGS]) >= sizeof(struct ifla_vlan_flags) ) {
										net_if->osdep.link.vlan.vlanflags = ((struct ifla_vlan_flags *)RTA_DATA(vlan[IFLA_VLAN_FLAGS]))->flags;
										LOG_INFO("IFLA_VLAN_FLAGS:          0x%04X (%s)\n", net_if->osdep.link.vlan.vlanflags, vlanflags(net_if->osdep.link.vlan.vlanflags));
									}

									if( vlan[IFLA_VLAN_EGRESS_QOS] ) {
										getqos((struct rtattr *)RTA_DATA(vlan[IFLA_VLAN_EGRESS_QOS]), RTA_PAYLOAD(vlan[IFLA_VLAN_EGRESS_QOS]), net_if->osdep.egress_qos_map);
										LOG_INFO("IFLA_VLAN_EGRESS_QOS:     { %S}\n", net_if->osdep.egress_qos_map.c_str());
									}

									if( vlan[IFLA_VLAN_INGRESS_QOS] ) {
										getqos((struct rtattr *)RTA_DATA(vlan[IFLA_VLAN_INGRESS_QOS]), RTA_PAYLOAD(vlan[IFLA_VLAN_INGRESS_QOS]), net_if->osdep.ingress_qos_map);
										LOG_INFO("IFLA_VLAN_INGRESS_QOS:    { %S}\n", net_if->osdep.ingress_qos_map.c_str());
									}
								}

							}


//	return NLMSG_ALIGN(sizeof(struct ifinfomsg))
	       //+ nla_total_size(IFNAMSIZ) /* IFLA_IFNAME */
	       //+ nla_total_size(IFALIASZ) /* IFLA_IFALIAS */
	       //+ nla_total_size(IFNAMSIZ) /* IFLA_QDISC */
	       //+ nla_total_size_64bit(sizeof(struct rtnl_link_ifmap))
	       //+ nla_total_size(sizeof(struct rtnl_link_stats))
	       //+ nla_total_size_64bit(sizeof(struct rtnl_link_stats64))
	       //+ nla_total_size(MAX_ADDR_LEN) /* IFLA_ADDRESS */
	       //+ nla_total_size(MAX_ADDR_LEN) /* IFLA_BROADCAST */
	       //+ nla_total_size(4) /* IFLA_TXQLEN */
	       //+ nla_total_size(4) /* IFLA_WEIGHT */
	       //+ nla_total_size(4) /* IFLA_MTU */
	       //+ nla_total_size(4) /* IFLA_LINK */
	       //+ nla_total_size(4) /* IFLA_MASTER */
	       //+ nla_total_size(1) /* IFLA_CARRIER */
	       //+ nla_total_size(4) /* IFLA_PROMISCUITY */
	       //+ nla_total_size(4) /* IFLA_ALLMULTI */
	       //+ nla_total_size(4) /* IFLA_NUM_TX_QUEUES */
	       //+ nla_total_size(4) /* IFLA_NUM_RX_QUEUES */
	       //+ nla_total_size(4) /* IFLA_GSO_MAX_SEGS */
	       //+ nla_total_size(4) /* IFLA_GSO_MAX_SIZE */
	       //+ nla_total_size(4) /* IFLA_GRO_MAX_SIZE */
	       //+ nla_total_size(4) /* IFLA_GSO_IPV4_MAX_SIZE */
	       //+ nla_total_size(4) /* IFLA_GRO_IPV4_MAX_SIZE */
	       //+ nla_total_size(4) /* IFLA_TSO_MAX_SIZE */
	       //+ nla_total_size(4) /* IFLA_TSO_MAX_SEGS */
	       //+ nla_total_size(1) /* IFLA_OPERSTATE */
	       //+ nla_total_size(1) /* IFLA_LINKMODE */
	       //+ nla_total_size(4) /* IFLA_CARRIER_CHANGES */
	       //+ nla_total_size(4) /* IFLA_LINK_NETNSID */
	       //+ nla_total_size(4) /* IFLA_GROUP */
	       //+ nla_total_size(ext_filter_mask
		//	        & RTEXT_FILTER_VF ? 4 : 0) /* IFLA_NUM_VF */
	       //+ rtnl_vfinfo_size(dev, ext_filter_mask) /* IFLA_VFINFO_LIST */
	       //+ rtnl_port_size(dev, ext_filter_mask) /* IFLA_VF_PORTS + IFLA_PORT_SELF */
	       //+ rtnl_link_get_size(dev) /* IFLA_LINKINFO */
	       //+ rtnl_link_get_af_size(dev, ext_filter_mask) /* IFLA_AF_SPEC */
	       //+ nla_total_size(MAX_PHYS_ITEM_ID_LEN) /* IFLA_PHYS_PORT_ID */
	       //+ nla_total_size(MAX_PHYS_ITEM_ID_LEN) /* IFLA_PHYS_SWITCH_ID */
	       //+ nla_total_size(IFNAMSIZ) /* IFLA_PHYS_PORT_NAME */
	       //+ rtnl_xdp_size() /* IFLA_XDP */
	       //+ nla_total_size(4)  /* IFLA_EVENT */
	       //+ nla_total_size(4)  /* IFLA_NEW_NETNSID */
	       //+ nla_total_size(4)  /* IFLA_NEW_IFINDEX */
	       //+ rtnl_proto_down_size(dev)  /* proto down */
	       //+ nla_total_size(4)  /* IFLA_TARGET_NETNSID */
	       //+ nla_total_size(4)  /* IFLA_CARRIER_UP_COUNT */
	       //+ nla_total_size(4)  /* IFLA_CARRIER_DOWN_COUNT */
	       //+ nla_total_size(4)  /* IFLA_MIN_MTU */
	       //+ nla_total_size(4)  /* IFLA_MAX_MTU */
	       //+ rtnl_prop_list_size(dev)
	       //+ nla_total_size(MAX_ADDR_LEN) /* IFLA_PERM_ADDRESS */
	       //+ rtnl_devlink_port_size(dev)
	       //+ 0;
/*
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0b0: type: IFLA_CARRIER (33) size 5
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0b8: type: IFLA_QDISC (6) size 12
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0c4: type: IFLA_CARRIER_CHANGES (35) size 8
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0cc: type: IFLA_CARRIER_UP_COUNT (47) size 8
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0d4: type: IFLA_CARRIER_DOWN_COUNT (48) size 8
[info]  netlink.c:552 FillAttr  - 0x7f06596ff0dc: type: IFLA_PROTO_DOWN (39) size 5
IFLA_PROTO_DOWN - это флаг, который используется в документации ядра Linux для указания причины отключения протокола.
[info]  netlink.c:552 FillAttr  - 0x7f06596ff250: type: IFLA_XDP (43) size 12
Например, если вы используете iproute2, вы можете использовать команду “ip link set <interface> xdp <command>” для установки программы XDP для интерфейса
XDP - это расширенный путь передачи данных (eXpress Data Path), который используется для отправки и получения сетевых пакетов на высоких скоростях,
 обходя большую часть сетевого стека операционной системы https://developers.redhat.com/blog/2021/04/01/get-started-with-xdp

struct link_util gtp_link_util = {
	.id		= "gtp",
	.maxattr	= IFLA_GTP_MAX,
	.parse_opt	= gtp_parse_opt,
	.print_opt	= gtp_print_opt,
	.print_help	= gtp_print_help,
};

struct link_util sit_link_util = {
	.id = "sit",
	.maxattr = IFLA_IPTUN_MAX,
	.parse_opt = iptunnel_parse_opt,
	.print_opt = iptunnel_print_opt,
	.print_help = iptunnel_print_help,
};

struct link_util tun_link_util = {
	.id = "tun",
	.maxattr = IFLA_TUN_MAX,
	.print_opt = tun_print_opt,
};


static struct link_util *linkutil_list;

struct link_util *get_link_kind(const char *id)
{
	void *dlh;
	char buf[256];
	struct link_util *l;

	for (l = linkutil_list; l; l = l->next)
		if (strcmp(l->id, id) == 0)
			return l;

	snprintf(buf, sizeof(buf), "%s/link_%s.so", get_ip_lib_dir(), id);
	dlh = dlopen(buf, RTLD_LAZY);
	if (dlh == NULL) {
		// look in current binary, only open once
		dlh = BODY;
		if (dlh == NULL) {
			dlh = BODY = dlopen(NULL, RTLD_LAZY);
			if (dlh == NULL)
				return NULL;
		}
	}

	snprintf(buf, sizeof(buf), "%s_link_util", id);
	l = dlsym(dlh, buf);
	if (l == NULL)
		return NULL;

	l->next = linkutil_list;
	linkutil_list = l;
	return l;
}

struct link_util {
	struct link_util	*next;
	const char		*id;
	int			maxattr;
	int			(*parse_opt)(struct link_util *, int, char **,
					     struct nlmsghdr *);
	void			(*print_opt)(struct link_util *, FILE *,
					     struct rtattr *[]);
	void			(*print_xstats)(struct link_util *, FILE *,
						struct rtattr *);
	void			(*print_help)(struct link_util *, int, char **,
					      FILE *);
	int			(*parse_ifla_xstats)(struct link_util *,
						     int, char **);
	int			(*print_ifla_xstats)(struct nlmsghdr *, void *);
};

struct link_util *get_link_kind(const char *kind);
*/
						}

						
						LOG_INFO("Parse lr->tb[IFLA_LINKINFO] ... ok\n");
						//IFLA_INFO_UNSPEC,
						//IFLA_INFO_KIND,
						//IFLA_INFO_DATA,
						//IFLA_INFO_XSTATS,
						//IFLA_INFO_SLAVE_KIND,
						//IFLA_INFO_SLAVE_DATA,
					}
				}


				if( lr->tb[IFLA_AF_SPEC] ) {
					struct rtattr *rta = (struct rtattr *)RTA_DATA(lr->tb[IFLA_AF_SPEC]);
					int len = RTA_PAYLOAD(lr->tb[IFLA_AF_SPEC]);

					while( RTA_OK(rta, len) ) {
						LOG_INFO("type: %u (%s) size %u\n", rta->rta_type, familyname(rta->rta_type), rta->rta_len);
						if( rta->rta_type == AF_INET ) {
							int32_t * data = (int32_t *)RTA_DATA(rta);
							uint32_t size = RTA_PAYLOAD(rta);
							const uint32_t maxsize = sizeof(net_if->osdep.ipv4conf)-sizeof(int32_t);
							size = size <= maxsize ? size:maxsize;
							memmove(&net_if->osdep.ipv4conf, data, size);
							net_if->osdep.ipv4conf.totalFields = size/sizeof(uint32_t)+1;
							net_if->LogIpv4Conf();
						}

						if( rta->rta_type == AF_INET6 ) {
							struct rtattr *tb[IFLA_INET6_MAX + 1];
							if( FillAttr((struct rtattr*)RTA_DATA(rta),
								tb,
								IFLA_INET6_MAX,
								(short unsigned int)(~NLA_F_NESTED),
								RTA_PAYLOAD(rta),
								iflainet6type) ) {

								if( tb[IFLA_INET6_CONF] && RTA_PAYLOAD(tb[IFLA_INET6_CONF]) >= sizeof(uint32_t) ) {
									int32_t * data = (int32_t *)RTA_DATA(tb[IFLA_INET6_CONF]);
									uint32_t size = RTA_PAYLOAD(tb[IFLA_INET6_CONF]);
									const uint32_t maxsize = sizeof(net_if->osdep.ipv6conf)-sizeof(int32_t);
									size = size <= maxsize ? size:maxsize;
									LOG_INFO("IFLA_INET6_CONF: RTA_PAYLOAD(tb[IFLA_INET6_CONF]) size %d fields %d, move size %d, move fields %d, totalFields %d\n", 
										RTA_PAYLOAD(tb[IFLA_INET6_CONF]), RTA_PAYLOAD(tb[IFLA_INET6_CONF])/sizeof(int32_t),
										size, size/sizeof(int32_t), (size/sizeof(int32_t))+1);
									memmove(&net_if->osdep.ipv6conf.forwarding, data, size);
									net_if->osdep.ipv6conf.totalFields = (size/sizeof(int32_t))+1;

									// accept_ra_rt_table field https://android.googlesource.com/kernel/msm/+/35b2ab13ba6bf6d7c1f70c6ae09fa264d6ae37bb%5E!/
									if( net_if->HasAcceptRaRtTable() && net_if->osdep.ipv6conf.totalFields > (DEVCONF_ACCEPT_RA_RT_TABLE+1) ) {
										net_if->osdep.accept_ra_rt_table = data[DEVCONF_ACCEPT_RA_RT_TABLE];
										memmove(&net_if->osdep.ipv6conf.forwarding + DEVCONF_ACCEPT_RA_RT_TABLE,
										&net_if->osdep.ipv6conf.forwarding + DEVCONF_ACCEPT_RA_RT_TABLE + 1,
										size-DEVCONF_ACCEPT_RA_RT_TABLE*sizeof(int32_t));
										net_if->osdep.ipv6conf.totalFields--;
									}
										
									net_if->LogIpv6Conf();
									
								}
								if( tb[IFLA_INET6_FLAGS] && RTA_PAYLOAD(tb[IFLA_INET6_FLAGS]) >= sizeof(uint32_t) ) {
									net_if->osdep.inet6flags = *(uint32_t *)RTA_DATA(tb[IFLA_INET6_FLAGS]);
									LOG_INFO("IFLA_INET6_FLAGS:         0x%08X (%s)\n", net_if->osdep.inet6flags, inet6flags(net_if->osdep.inet6flags));
								}
								if( tb[IFLA_INET6_CACHEINFO] && RTA_PAYLOAD(tb[IFLA_INET6_CACHEINFO]) >= sizeof(struct ifla_cacheinfo) ) {
									//static_assert( sizeof(struct ifla_cacheinfo) == sizeof(net_if->osdep.inet6cacheinfo) );
									memmove(&net_if->osdep.inet6cacheinfo, RTA_DATA(tb[IFLA_INET6_CACHEINFO]), sizeof(net_if->osdep.inet6cacheinfo));
									LOG_INFO("IFLA_INET6_CACHEINFO:     max_reasm_len:  %s\n", size_to_str(net_if->osdep.inet6cacheinfo.max_reasm_len) );
									LOG_INFO("IFLA_INET6_CACHEINFO:     tstamp:         %.2fs\n", (double)net_if->osdep.inet6cacheinfo.tstamp/100.0);
									LOG_INFO("IFLA_INET6_CACHEINFO:     reachable_time: %s\n", msec_to_str(net_if->osdep.inet6cacheinfo.reachable_time));
									LOG_INFO("IFLA_INET6_CACHEINFO:     retrans_time:   %s\n", msec_to_str(net_if->osdep.inet6cacheinfo.retrans_time));
								}
								if( tb[IFLA_INET6_STATS] && RTA_PAYLOAD(tb[IFLA_INET6_STATS]) >= sizeof(uint64_t) ) {
									uint64_t * mib = (uint64_t *)RTA_DATA(tb[IFLA_INET6_STATS]);
									uint64_t size = RTA_PAYLOAD(tb[IFLA_INET6_STATS]);
									size = size <= mib[IPSTATS_MIB_NUM]*sizeof(uint64_t) ? size:mib[IPSTATS_MIB_NUM]*sizeof(uint64_t);
									size = size <= sizeof(net_if->osdep.inet6stat) ? size:sizeof(net_if->osdep.inet6stat);
									if( size != sizeof(net_if->osdep.inet6stat) )
										memset(&net_if->osdep.inet6stat, 0, sizeof(net_if->osdep.inet6stat));
									memmove(&net_if->osdep.inet6stat, mib, size);
									net_if->osdep.inet6stat.totalStatsFields = size/sizeof(uint64_t);
									if( mib[IPSTATS_MIB_NUM] != net_if->osdep.inet6stat.totalStatsFields )
										LOG_WARN("IPSTATS_MIB_NUM: %u != %u totalStatsFields\n", mib[IPSTATS_MIB_NUM], net_if->osdep.inet6stat.totalStatsFields);

									if( net_if->osdep.inet6stat.totalStatsFields == IPSTATS_MIB_OUTBCASTOCTETS ) {
										auto st = &net_if->osdep.inet6stat;
										LOG_WARN("Reoder stats");
										st->Ip6InHdrErrors = mib[2];
										st->Ip6InTooBigErrors = mib[3];
										st->Ip6InNoRoutes = mib[4];
										st->Ip6InAddrErrors = mib[5];
										st->Ip6InUnknownProtos = mib[6];
										st->Ip6InTruncatedPkts = mib[7];
										st->Ip6InDiscards = mib[8];
										st->Ip6InDelivers = mib[9];
										st->Ip6OutForwDatagrams = mib[10];
										st->Ip6OutRequests = mib[11];
										st->Ip6OutDiscards = mib[12];
										st->Ip6OutNoRoutes = mib[13];
										st->Ip6ReasmTimeout = mib[14];
										st->Ip6ReasmReqds = mib[15];
										st->Ip6ReasmOKs = mib[16];
										st->Ip6ReasmFails = mib[17];
										st->Ip6FragOKs = mib[18];
										st->Ip6FragFails = mib[19];
										st->Ip6FragCreates = mib[20];
										st->Ip6InMcastPkts = mib[21];
										st->Ip6OutMcastPkts = mib[22];
										st->Ip6InBcastPkts = mib[23];
										st->Ip6OutBcastPkts = mib[24];
										st->Ip6InOctets = mib[25];
										st->Ip6OutOctets = mib[26];
									}
									net_if->LogInet6Stats();
								}

								if( tb[IFLA_INET6_ICMP6STATS] && RTA_PAYLOAD(tb[IFLA_INET6_ICMP6STATS]) >= sizeof(uint64_t) ) {
									uint64_t * mib = (uint64_t *)RTA_DATA(tb[IFLA_INET6_ICMP6STATS]);
									uint64_t size = RTA_PAYLOAD(tb[IFLA_INET6_ICMP6STATS]);
									size = size <= mib[IFLA_INET6_ICMP6STATS]*sizeof(uint64_t) ? size:mib[IFLA_INET6_ICMP6STATS]*sizeof(uint64_t);
									size = size <= sizeof(net_if->osdep.icmp6stat) ? size:sizeof(net_if->osdep.icmp6stat);
									if( size != sizeof(net_if->osdep.icmp6stat) )
										memset(&net_if->osdep.icmp6stat, 0, sizeof(net_if->osdep.icmp6stat));
									memmove(&net_if->osdep.icmp6stat, mib, size);
									net_if->osdep.icmp6stat.totalStatsFields = size/sizeof(uint64_t);
									if( mib[ICMP6_MIB_NUM] != net_if->osdep.icmp6stat.totalStatsFields )
										LOG_WARN("ICMP6_MIB_NUM: %u != %u totalStatsFields\n", mib[ICMP6_MIB_NUM], net_if->osdep.icmp6stat.totalStatsFields);
									net_if->LogIcmp6Stats();
								}

								if( tb[IFLA_INET6_TOKEN] && RTA_PAYLOAD(tb[IFLA_INET6_TOKEN]) >= sizeof(struct in6_addr) ) {
									struct sockaddr_in6 sa6;
									sa6.sin6_family = AF_INET6;
									sa6.sin6_addr = *(struct in6_addr *)RTA_DATA(tb[IFLA_INET6_TOKEN]);
									net_if->osdep.ipv6token = get_sockaddr_str((const struct sockaddr *)&sa6, false);
									LOG_INFO("IFLA_INET6_TOKEN:         %S\n", net_if->osdep.ipv6token.c_str());
								}

								if( tb[IFLA_INET6_ADDR_GEN_MODE] && RTA_PAYLOAD(tb[IFLA_INET6_ADDR_GEN_MODE]) >= sizeof(uint8_t) ) {
									net_if->osdep.ipv6genmodeflags = *(uint8_t *)RTA_DATA(tb[IFLA_INET6_ADDR_GEN_MODE]);
									LOG_INFO("IFLA_INET6_ADDR_GEN_MODE: 0x%02X (%s)\n", net_if->osdep.ipv6genmodeflags, ipv6genmodeflags(net_if->osdep.ipv6genmodeflags));
								}

								LOG_INFO("tb[IFLA_INET6...] ... ok\n");
							}
						}

						rta = RTA_NEXT(rta, len);
					}


				}

				if( lr->tb[IFLA_PARENT_DEV_NAME] && RTA_PAYLOAD(lr->tb[IFLA_PARENT_DEV_NAME]) >= sizeof(uint8_t) ) {
					net_if->osdep.valid.parentdev_name = 1;
					net_if->osdep.parentdev_name = towstr((const char *)RTA_DATA(lr->tb[IFLA_PARENT_DEV_NAME]));
					LOG_INFO("IFLA_PARENT_DEV_NAME:     %S\n", net_if->osdep.parentdev_name.c_str());
				}
				if( lr->tb[IFLA_PARENT_DEV_BUS_NAME] && RTA_PAYLOAD(lr->tb[IFLA_PARENT_DEV_BUS_NAME]) >= sizeof(uint8_t) ) {
					net_if->osdep.valid.parentdev_busname = 1;
					net_if->osdep.parentdev_busname = towstr((const char *)RTA_DATA(lr->tb[IFLA_PARENT_DEV_BUS_NAME]));
					LOG_INFO("IFLA_PARENT_DEV_BUS_NAME: %S\n", net_if->osdep.parentdev_busname.c_str());
				}
				if( lr->tb[IFLA_MAP] && RTA_PAYLOAD(lr->tb[IFLA_MAP]) >= sizeof(struct rtnl_link_ifmap) ) {
					memmove(&net_if->osdep.ifmap, RTA_DATA(lr->tb[IFLA_MAP]), sizeof(net_if->osdep.ifmap));	
					struct rtnl_link_ifmap * ifmap = (struct rtnl_link_ifmap *)RTA_DATA(lr->tb[IFLA_MAP]);
					LOG_INFO("IFLA_MAP:                 mem_start: 0x%p\n", ifmap->mem_start);
					LOG_INFO("IFLA_MAP:                 mem_end:   0x%p\n", ifmap->mem_end);
					LOG_INFO("IFLA_MAP:                 base_addr: 0x%p\n", ifmap->base_addr);
					LOG_INFO("IFLA_MAP:                 irq:       %d\n", ifmap->irq);
					LOG_INFO("IFLA_MAP:                 dma:       %d\n", ifmap->dma);
					LOG_INFO("IFLA_MAP:                 port:      %d\n", ifmap->port);
				}

				if( lr->tb[IFLA_MASTER] && RTA_PAYLOAD(lr->tb[IFLA_MASTER]) >= sizeof(uint32_t) ) {
					net_if->osdep.valid.master = 1;
					net_if->osdep.master = RTA_UINT32_T(lr->tb[IFLA_MASTER]);
					LOG_INFO("IFLA_MASTER:              %u\n", net_if->osdep.master);
				}

				net_if->LogHardInfo();

				net_if->mac[mac.mac] = mac;
			}
			lr++;
		}

		lr = 0;

		const AddrRecord * ar =  GetAddr(netlink, AF_UNSPEC);
		if( !ar )
			break;

		while( ar->ifam ) {

			LOG_INFO("-------------------------------------\n");
			LOG_INFO("nlh.nlmsg_type: %d (%s)\n", ar->nlm.nlmsg_type, nlmsgtype(ar->nlm.nlmsg_type));
			LOG_INFO("ifa_family:    %u (%s)\n", ar->ifam->ifa_family, familyname(ar->ifam->ifa_family));
			LOG_INFO("ifa_prefixlen: %u\n", ar->ifam->ifa_prefixlen);
			LOG_INFO("ifa_flags:     0x%08X (%s)\n", ar->ifam->ifa_flags, ifaddrflags(ar->ifam->ifa_flags)); // IFA_F_* flags
			LOG_INFO("ifa_scope:     %u (%s)\n", ar->ifam->ifa_scope, rtscopetype(ar->ifam->ifa_scope)); // Address scope
			LOG_INFO("ifa_index:     %u\n", ar->ifam->ifa_index); // Link index

			NetInterface * net_if = FindByIndex(ar->ifam->ifa_index);

			if( net_if ) {
				LOG_INFO("interface:     %S\n", net_if->name.c_str());
				IpAddressInfo ip;

				ip.family = ar->ifam->ifa_family;
				ip.flags = ar->ifam->ifa_flags;
				ip.scope = ar->ifam->ifa_scope;
				ip.prefixlen = ar->ifam->ifa_prefixlen;
				ip.rt_priority = 0;
				ip.netnsid = 0;
				memset(&ip.cacheinfo, 0, sizeof(ip.cacheinfo));

				uint32_t addrlen = 0;
				const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
				char s[maxlen+1] = {0};
			
			        switch( ip.family ) {
				case AF_INET:
					addrlen = sizeof(uint32_t);
					ip.netmask = towstr(IpBitsToMask(ip.prefixlen, s, maxlen));
					break;
				case  AF_INET6:
					addrlen = sizeof(struct in6_addr);
					if( snprintf(s, maxlen, "%u", ip.prefixlen) > 0 )
						ip.netmask = towstr(s);
					break;
				case AF_MCTP:
					addrlen = sizeof(uint8_t);
					break;
				default:
					LOG_ERROR("unsupported family %u\n", ip.family);
					continue;
				};
				if( ar->tb[IFA_FLAGS] && RTA_PAYLOAD(ar->tb[IFA_FLAGS]) >= sizeof(uint32_t) ) {
					ip.flags = RTA_UINT32_T(ar->tb[IFA_FLAGS]);
					LOG_INFO("IFA_FLAGS:          0x%08X (%s)\n", ip.flags, ifaddrflags(ip.flags));
				}
				if( ar->tb[IFA_ADDRESS] && RTA_PAYLOAD(ar->tb[IFA_ADDRESS]) >= addrlen ) {
					if( inet_ntop(ip.family, RTA_DATA(ar->tb[IFA_ADDRESS]), s, maxlen) )
						ip.ip = towstr(s);
					LOG_INFO("IFA_ADDRESS:        %S\n", ip.ip.c_str());
				}
				if( ar->tb[IFA_LOCAL] && RTA_PAYLOAD(ar->tb[IFA_LOCAL]) >= addrlen ) {
					if( inet_ntop(ip.family, RTA_DATA(ar->tb[IFA_LOCAL]), s, maxlen) )
						ip.local = towstr(s);
					LOG_INFO("IFA_LOCAL:          %S\n", ip.local.c_str());
				}
				if( ar->tb[IFA_BROADCAST] && RTA_PAYLOAD(ar->tb[IFA_BROADCAST]) >= addrlen ) {
					if( inet_ntop(ip.family, RTA_DATA(ar->tb[IFA_BROADCAST]), s, maxlen) )
						ip.broadcast = towstr(s);
					LOG_INFO("IFA_BROADCAST:      %S\n", ip.broadcast.c_str());
				}
				if( ar->tb[IFA_LABEL] && RTA_PAYLOAD(ar->tb[IFA_LABEL]) >= sizeof(uint8_t) ) {
					ip.label = towstr((const char *)RTA_DATA(ar->tb[IFA_LABEL]));
					LOG_INFO("IFA_LABEL:          %S\n", ip.label.c_str());
				}
				if( ar->tb[IFA_RT_PRIORITY] && RTA_PAYLOAD(ar->tb[IFA_RT_PRIORITY]) >= sizeof(uint32_t) ) {
					ip.rt_priority = RTA_UINT32_T(ar->tb[IFA_RT_PRIORITY]);
					LOG_INFO("IFA_RT_PRIORITY:    %d\n", ip.rt_priority);
				}
				if( ar->tb[IFA_TARGET_NETNSID] && RTA_PAYLOAD(ar->tb[IFA_TARGET_NETNSID]) >= sizeof(uint32_t) ) {
					ip.netnsid = RTA_INT32_T(ar->tb[IFA_TARGET_NETNSID]);
					LOG_INFO("IFA_TARGET_NETNSID: %d\n", ip.netnsid);
				}
				if( ar->tb[IFA_PROTO] && RTA_PAYLOAD(ar->tb[IFA_PROTO]) >= sizeof(uint8_t) ) {
					ip.proto = RTA_UINT8_T(ar->tb[IFA_PROTO]);
					LOG_INFO("IFA_PROTO:          %d\n", ip.proto);
/* ifa_proto */
//#define IFAPROT_UNSPEC		0
//#define IFAPROT_KERNEL_LO	1	/* loopback */
//#define IFAPROT_KERNEL_RA	2	/* set by kernel from router announcement */
//#define IFAPROT_KERNEL_LL	3	/* link-local set by kernel */


				}
				if( ar->tb[IFA_CACHEINFO] && RTA_PAYLOAD(ar->tb[IFA_CACHEINFO]) >= sizeof(struct ifa_cacheinfo) ) {
					//static_assert( sizeof(ip.cacheinfo) == sizeof(struct ifa_cacheinfo) );
					memmove(&ip.cacheinfo, RTA_DATA(ar->tb[IFA_CACHEINFO]), sizeof(ip.cacheinfo));
					LOG_INFO("IFA_CACHEINFO:      ifa_prefered: %s\n", ip.cacheinfo.ifa_prefered == 0xFFFFFFFFU ? "forever":msec_to_str(ip.cacheinfo.ifa_prefered*1000));
					LOG_INFO("IFA_CACHEINFO:      ifa_valid:    %s\n", ip.cacheinfo.ifa_valid == 0xFFFFFFFFU ? "forever":msec_to_str(ip.cacheinfo.ifa_valid*1000));
					LOG_INFO("IFA_CACHEINFO:      cstamp:       %s (%d/100 sec)\n", msec_to_str(ip.cacheinfo.cstamp*10), ip.cacheinfo.cstamp);
					LOG_INFO("IFA_CACHEINFO:      tstamp:       %s (%d/100 sec)\n", msec_to_str(ip.cacheinfo.tstamp*10), ip.cacheinfo.tstamp);
				}

			        switch( ip.family ) {
				case AF_INET:
					net_if->ip[ip.ip] = ip;
					break;
				case  AF_INET6:
					net_if->ip6[ip.ip] = ip;
					break;
				};
			}
			ar++;
		}


		if( ifs.size() )
			res = true;

	} while(0);


	CloseNetlink(netlink);

	return res;
	//return false;
}
#endif

bool NetInterfaces::Update(void)
{
	LOG_INFO("\n");
	Clear();
	return
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	UpdateByNetlink() ||
#endif
	UpdateByProcNet();
}

void NetInterfaces::Log(void) {
	for( const auto& [name, net_if] : ifs ) {
		net_if->Log();
	}
}

NetInterfaces::NetInterfaces()
{
}

NetInterfaces::~NetInterfaces()
{
	Clear();
	LOG_INFO("\n");
}

extern int RootExec(const char * cmd);

bool NetInterfaces::TcpDumpStart(const wchar_t * iface, const wchar_t * file, bool promisc)
{
	LOG_INFO("iface: %S file: %S promisc: %s\n", iface, file, promisc ? "true":"false");
	if( ifs.find(iface) == ifs.end() )
		return false;
	ifs.find(iface)->second->SetPromisc(promisc);
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, TCPDUMP_CMD, iface, file, iface, file) > 0 )
		if( RootExec(buffer) == 0 ) {
			tcpdump.push_back(iface);
			return true;
		}
	return false;
}

void NetInterfaces::TcpDumpStop(const wchar_t * iface)
{
	LOG_INFO("\n");
	for( auto it = tcpdump.begin(); it != tcpdump.end();) {
		if( *it == iface ) {
			char buffer[MAX_CMD_LEN];
			if( snprintf(buffer, MAX_CMD_LEN, TCPDUMPKILL_CMD, iface) > 0 )
				RootExec(buffer);
			it = tcpdump.erase(it);
		} else
			++it;
	}
	return;
}

#ifdef MAIN_NETIF
int RootExec(const char * cmd)
{
	std::string _cmd("sudo /bin/sh -c \'");
	_cmd += cmd;
	_cmd += "'";
	LOG_INFO("Try exec \"%s\"\n", _cmd.c_str());
	return system(_cmd.c_str());
}

int main(int argc, char * argv[])
{
	{
		NetInterfaces net = NetInterfaces();
		net.Update();
		net.Log();
		for( const auto& [name, net_if] : net ) {
			LOG_INFO("%S:\n", net_if->name.c_str());
			//net_if->SetInterfaceName(L"eth0");
			//net_if->SetMac(L"88:88:88:88:88:88");
			net.Log();
			break;
		}
		net.Clear();
	}
	{
		NetInterface net_if = NetInterface(L"lo");
	}

	system("mv /tmp/far2l.netcfg.log .");
	return 0;
}

#endif //MAIN_NETIF
