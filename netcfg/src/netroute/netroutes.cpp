#include "netroutes.h"
#include "netrouteset.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/netutils.h>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <net/if.h>
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)

#define PATH_ROUTE	  "/proc/net/route"
#define PATH_ROUTE6	  "/proc/net/ipv6_route"
#define PATH_ARP	  "/proc/net/arp"

// permanent  net.ipv4.ip_forward and net.ipv6.conf.all.forwarding in /etc/sysctl.conf
#define PATH_IPV4_FORWARD "/proc/sys/net/ipv4/ip_forward"
#define PATH_IPV6_FORWARD "/proc/sys/net/ipv6/conf/all/forwarding"

#else

#define ROUNDUP(a) \
       ((a) > 0 ? (1 + (((a) - 1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))

#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/ethernet.h>

#endif

#define LOG_SOURCE_FILE "netroutes.cpp"
#ifndef MAIN_NETROUTES
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

NetRoutes::NetRoutes():
ipv4_forwarding(false),
ipv6_forwarding(false)
{
	LOG_INFO("\n");
}

NetRoutes::~NetRoutes()
{
	Clear();
	LOG_INFO("\n");
}

#define MAX_INTERFACE_NAME_LEN 16

#if defined(__APPLE__) || defined(__FreeBSD__)
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
	case AF_LINK:
	{
		struct sockaddr_dl * sdl = (struct sockaddr_dl *)sa;
		if (sdl->sdl_nlen == 0 && sdl->sdl_alen == 0 &&
		    sdl->sdl_slen == 0) {
			//static_assert( IFNAMSIZ < sizeof(s) );

			/*LOG_INFO("1. sdl_index: %d\n", sdl->sdl_index);
			LOG_INFO("1. sdl_type:  0x%02x (%s)\n", sdl->sdl_type, ifttoname(sdl->sdl_type));
			LOG_INFO("1. sdl_nlen:  %d\n", sdl->sdl_nlen);
			LOG_INFO("1. sdl_alen   %u\n", sdl->sdl_alen);
			LOG_INFO("1. sdl_slen   %u\n", sdl->sdl_slen);*/

			addr = if_indextoname(sdl->sdl_index, s);
		} else if( sdl->sdl_type == IFT_ETHER || sdl->sdl_alen == ETHER_ADDR_LEN ) {

			/*LOG_INFO("2. sdl_index: %d\n", sdl->sdl_index);
			LOG_INFO("2. sdl_type:  0x%02x (%s)\n", sdl->sdl_type, ifttoname(sdl->sdl_type));
			LOG_INFO("2. sdl_nlen:  %d\n", sdl->sdl_nlen);
			LOG_INFO("2. sdl_alen   %u\n", sdl->sdl_alen);
			LOG_INFO("2. sdl_slen   %u\n", sdl->sdl_slen);*/

			unsigned char * mac = (unsigned char *)sdl->sdl_data + sdl->sdl_nlen;
			//static_assert( sizeof("FF:FF:FF:FF:FF:FF") < sizeof(s) );
			if( snprintf(s, maxlen, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) > 0 )
				addr = s;
		} else if( (sdl->sdl_alen + sdl->sdl_nlen/2) == ETHER_ADDR_LEN ) {

			LOG_INFO("2.5 sdl_index: %d\n", sdl->sdl_index);
			LOG_INFO("2.5 sdl_type:  0x%02x (%s)\n", sdl->sdl_type, ifttoname(sdl->sdl_type));
			LOG_INFO("2.5 sdl_nlen:  %d\n", sdl->sdl_nlen);
			LOG_INFO("2.5 sdl_alen   %u\n", sdl->sdl_alen);
			LOG_INFO("2.5 sdl_slen   %u\n", sdl->sdl_slen);

			//static_assert( ETHER_ADDR_LEN*3 < sizeof(s) );
			memmove(s, sdl->sdl_data, sdl->sdl_nlen);
			char * ptr = &s[sdl->sdl_nlen];
			for(int i = 0; i < (ETHER_ADDR_LEN - sdl->sdl_nlen/2); i++ )
				ptr += snprintf(ptr, maxlen-(ptr-s), ":%02x", *((unsigned char *)sdl->sdl_data + sdl->sdl_nlen + i));
			addr = s;
		} else {

			LOG_INFO("3. sdl_index: %d\n", sdl->sdl_index);
			LOG_INFO("3. sdl_type:  0x%02x (%s)\n", sdl->sdl_type, ifttoname(sdl->sdl_type));
			LOG_INFO("3. sdl_nlen:  %d\n", sdl->sdl_nlen);
			LOG_INFO("3. sdl_alen   %u\n", sdl->sdl_alen);
			LOG_INFO("3. sdl_slen   %u\n", sdl->sdl_slen);

			addr = link_ntoa(sdl);
		}
		break;
	}
	default:
		LOG_ERROR("unsupported sa_family: %u\n", sa->sa_family);
		break;
	}
	return std::wstring(addr.begin(), addr.end());
}

const char * GetRtmAddrs(uint8_t rtm_addrs)
{
	static char buf[256] = {0};
	memset(buf, 0, sizeof(buf));
	if( rtm_addrs & RTA_DST )
		strcat(buf, "DST");
	if( rtm_addrs & RTA_GATEWAY )
		strcat(buf, " GATEWAY");
	if( rtm_addrs & RTA_NETMASK )
		strcat(buf, " NETMASK");
	if( rtm_addrs & RTA_GENMASK )
		strcat(buf, " GENMASK");
	if( rtm_addrs & RTA_IFP )
		strcat(buf, " IFP");
	if( rtm_addrs & RTA_IFA )
		strcat(buf, " IFA");
	if( rtm_addrs & RTA_AUTHOR )
		strcat(buf, " AUTHOR");
	if( rtm_addrs & RTA_BRD )
		strcat(buf, " BRD");
	return buf;
}

const char * GetRtmInits(uint32_t rtm_inits)
{
	static char buf[256] = {0};
	memset(buf, 0, sizeof(buf));
	if( rtm_inits & RTV_MTU )
		strcat(buf, "MTU");
	if( rtm_inits & RTV_HOPCOUNT )
		strcat(buf, " HOPCOUNT");
	if( rtm_inits & RTV_EXPIRE )
		strcat(buf, " EXPIRE");
	if( rtm_inits & RTV_RPIPE )
		strcat(buf, " RPIPE");
	if( rtm_inits & RTV_SPIPE )
		strcat(buf, " SPIPE");
	if( rtm_inits & RTV_SSTHRESH )
		strcat(buf, " SSTHRESH");
	if( rtm_inits & RTV_RTT )
		strcat(buf, " RTT");
	if( rtm_inits & RTV_RTTVAR )
		strcat(buf, " RTTVAR");
	return buf;
}

#ifndef RTF_GLOBAL
#define RTF_GLOBAL      0x40000000      /* route to destination of the global internet */
#endif

const char * GetRtmFlags(uint32_t rtm_flags)
{
	static char buf[4096] = {0};
	memset(buf, 0, sizeof(buf));
	if( rtm_flags & RTF_UP )
		strcat(buf, "RTF_UP route usable");
	if( rtm_flags & RTF_GATEWAY )
		strcat(buf, ", RTF_GATEWAY destination is a gateway");
	if( rtm_flags & RTF_HOST )
		strcat(buf, ", RTF_HOST host entry (net otherwise)");
	if( rtm_flags & RTF_REJECT )
		strcat(buf, ", RTF_REJECT host or net unreachable");
	if( rtm_flags & RTF_DYNAMIC )
		strcat(buf, ", RTF_DYNAMIC created dynamically (by redirect)");
	if( rtm_flags & RTF_MODIFIED )
		strcat(buf, ", RTF_DYNAMIC modified dynamically (by redirect)");
	if( rtm_flags & RTF_DONE )
		strcat(buf, ", RTF_DONE message confirmed");
	if( rtm_flags & RTF_DELCLONE )
		strcat(buf, ", RTF_DELCLONE delete cloned route");
	if( rtm_flags & RTF_CLONING )
		strcat(buf, ", RTF_CLONING generate new routes on use");
	if( rtm_flags & RTF_XRESOLVE )
		strcat(buf, ", RTF_XRESOLVE external daemon resolves name");
	if( rtm_flags & RTF_LLINFO )
		strcat(buf, ", RTF_LLINFO generated by ARP or NDP");
	if( rtm_flags & RTF_LLDATA )
		strcat(buf, ", RTF_LLDATA used by apps to add/del L2 entries");
	if( rtm_flags & RTF_STATIC )
		strcat(buf, ", RTF_STATIC manually added");
	if( rtm_flags & RTF_BLACKHOLE )
		strcat(buf, ", RTF_BLACKHOLE just discard pkts (during updates)");
	if( rtm_flags & RTF_NOIFREF )
		strcat(buf, ", RTF_NOIFREF not eligible for RTF_IFREF");
	if( rtm_flags & RTF_PROTO2 )
		strcat(buf, ", RTF_PROTO2 protocol specific routing flag");
	if( rtm_flags & RTF_PROTO1 )
		strcat(buf, ", RTF_PROTO1 protocol specific routing flag");
	if( rtm_flags & RTF_PRCLONING )
		strcat(buf, ", RTF_PRCLONING protocol requires cloning");
	if( rtm_flags & RTF_WASCLONED )
		strcat(buf, ", RTF_WASCLONED route generated through cloning");
	if( rtm_flags & RTF_PROTO3 )
		strcat(buf, ", RTF_PROTO3 (RTPRF_OURS set on routes we manage) protocol specific routing flag");
	if( rtm_flags & 0x80000 )
		strcat(buf, ", 0x80000 unused");
	if( rtm_flags & RTF_PINNED )
		strcat(buf, ", RTF_PINNED future use");
	if( rtm_flags & RTF_LOCAL )
		strcat(buf, ", RTF_LOCAL route represents a local address");
	if( rtm_flags & RTF_BROADCAST )
		strcat(buf, ", RTF_BROADCAST route represents a bcast address");
	if( rtm_flags & RTF_MULTICAST )
		strcat(buf, ", RTF_MULTICAST route represents a mcast address");
	if( rtm_flags & RTF_IFSCOPE )
		strcat(buf, ", RTF_IFSCOPE has valid interface scope");
	if( rtm_flags & RTF_CONDEMNED )
		strcat(buf, ", RTF_CONDEMNED defunct; no longer modifiable");
	if( rtm_flags & RTF_IFREF )
		strcat(buf, ", RTF_IFREF route holds a ref to interface");
	if( rtm_flags & RTF_PROXY )
		strcat(buf, ", RTF_PROXY proxying, no interface scope");
	if( rtm_flags & RTF_ROUTER )
		strcat(buf, ", RTF_ROUTER host is a router");
	if( rtm_flags & RTF_DEAD )
		strcat(buf, ", RTF_DEAD route entry is being freed");
	if( rtm_flags & RTF_GLOBAL )
		strcat(buf, ", RTF_GLOBAL route to destination of the global internet");
	return buf;
}

static std::string tostr(const wchar_t * name)
{
	std::wstring _s(name);
	return std::string(_s.begin(), _s.end());
}

#endif

// towstr can use only for ASCII symbols
static std::wstring towstr(const char * name)
{
	std::string _s(name);
	return std::wstring(_s.begin(), _s.end());
}

static std::wstring destIpandMask(uint8_t family, void * adr, uint8_t maskbits)
{
	char addr_mask[sizeof("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:255.255.255.255/128")] = {0};
	if( inet_ntop(family, adr, addr_mask, sizeof(addr_mask)) ) {
		size_t len = strlen(addr_mask);

		assert( maskbits <= 128 );
		assert( (sizeof(addr_mask) - len) >= sizeof("/128") );

		if( snprintf(addr_mask+len, sizeof(addr_mask)-len, "/%u", maskbits) > 0 )
			return towstr(addr_mask);

	}
	return std::wstring();
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
bool NetRoutes::UpdateByProcNet(void)
{
	char ifname[MAX_INTERFACE_NAME_LEN] = {0};

	FILE *routes = fopen(PATH_ROUTE, "r");
	if( !routes ){
		LOG_ERROR("can`t open \"/proc/net/route\" ... error (%s)\n", errorname(errno));
		return false;
	}

	// clear before start
	Clear();

	do {
		// skip header
		{
			char line[512];
			if( fgets(line, sizeof(line), routes) == 0 )
			break;
		}

		uint32_t destination, gateway, flags, mask, unused;
		int32_t metric;

		while ( fscanf(routes, "%s %x %x %x %d %d %d %x %d %d %d\n", ifname, &destination, &gateway, \
			&flags, &unused, &unused, &metric, &mask, &unused, &unused, &unused) == 11 ) {

			int32_t bits = 0;
			IpRouteInfo ipr;
			ipr.ifnameIndex = (uint32_t)-1;

			if( ifname[0] == '*' ) {
				if( flags & RTF_REJECT )
					ipr.iface = towstr("unreachable");
				else
					ipr.iface = towstr("blackhole");
			} else
				ipr.iface = towstr(ifname);

			char addr_mask[sizeof("255.255.255.255/32")] = {0};
			if( inet_ntop(AF_INET, &destination, addr_mask, sizeof(addr_mask)) ) {
				while( mask ) {
					bits += mask & 1;
					mask >>= 1;
				}
				assert( bits <= 32 );
				size_t len = strlen(addr_mask);
				assert( (sizeof(addr_mask) - len) >= sizeof("/32") );
				if( snprintf(addr_mask+len, sizeof(addr_mask)-len, "/%u", bits) > 0 )
					ipr.destIpandMask = towstr(addr_mask);
			}

			if( inet_ntop(AF_INET, &gateway, addr_mask, sizeof(addr_mask)) )
				ipr.gateway = towstr(addr_mask);

			ipr.dstprefixlen = (uint32_t)bits;
			ipr.sa_family = AF_INET;
			ipr.flags = flags;
			ipr.osdep.metric = metric;

			if( bits == 0 && metric >= 0  ) {
				ipr.valid.gateway = true;
				inet.push_front(ipr);
			} else
				inet.push_back(ipr);
		}

	} while(0);

	fclose(routes);	

	// Ipv6

	routes = fopen(PATH_ROUTE6, "r");
	if( !routes ){
		LOG_ERROR("can`t open \"/proc/net/ipv6_route\" ... error (%s)\n", errorname(errno));
		return false;
	}

	do {
		uint32_t flags, unused, bits;
		int32_t metric;
		char addr6p[8][5], naddr6p[8][5];
		char unused_ch[5];

		while( fscanf(routes, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %15s\n",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				addr6p[4], addr6p[5], addr6p[6], addr6p[7],
				&bits,
				unused_ch, unused_ch, unused_ch, unused_ch,
				unused_ch, unused_ch, unused_ch, unused_ch,
				&unused,
				naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
				naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
				&metric, &unused, &unused, &flags, ifname) == 31 ) {

			IpRouteInfo ipr;
			ipr.iface = towstr(ifname);

			char addr_mask[sizeof("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:255.255.255.255/128")] = {0};
			struct in6_addr addr;

			if( snprintf(addr_mask, sizeof(addr_mask), "%s:%s:%s:%s:%s:%s:%s:%s",
				naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
				naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7]) > 0 ) {
				inet_pton(AF_INET6, addr_mask, &addr);
				if( inet_ntop(AF_INET6, &addr, addr_mask, sizeof(addr_mask)) )
					ipr.gateway = towstr(addr_mask);
			}

			size_t len = snprintf(addr_mask, sizeof(addr_mask), "%s:%s:%s:%s:%s:%s:%s:%s",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
			if( len > 0 ) {
				inet_pton(AF_INET6, addr_mask, &addr);
				if( inet_ntop(AF_INET6, &addr, addr_mask, sizeof(addr_mask)) ) {
					len = strlen(addr_mask);

					assert( bits <= 128 );
					assert( (sizeof(addr_mask) - len) >= sizeof("/128") );

					if( snprintf(addr_mask+len, sizeof(addr_mask)-len, "/%u", bits) > 0)
						ipr.destIpandMask = towstr(addr_mask);
				}
			}

			ipr.dstprefixlen = bits;
			ipr.sa_family = AF_INET6;
			ipr.flags = flags;
			ipr.osdep.metric = metric;

			if( bits == 0 && metric >= 0  ) {
				ipr.valid.gateway = true;
				inet6.push_front(ipr);
			} else
				inet6.push_back(ipr);
		}

	} while(0);

	fclose(routes);	

	routes = fopen(PATH_ARP, "r");
	if( !routes ){
		LOG_ERROR("can`t open \"/proc/net/arp\" ... error (%s)\n", errorname(errno));
		return false;
	}

	do {
		char line[512];

		if( fgets(line, sizeof(line), routes) == 0 )
			break;

		const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;

		//static_assert( sizeof("ff:ff:ff:ff:ff:ff") < maxlen );
		//static_assert( 15 < MAX_INTERFACE_NAME_LEN );

		char ip[maxlen] = {0};
		char mac[maxlen] = {0};
		char mask[maxlen] = {0};
		uint32_t type, flags;
		int num;
		
		while( fgets(line, sizeof(line), routes) != 0 &&
			(num = sscanf(line, "%s 0x%x 0x%x %17s %17s %15s\n",
			     ip, &type, &flags, mac, mask, ifname)) >= 5 ) {
			if( num == 5 ) {
				/*
				 * This happens for incomplete ARP entries for which there is
				 * no hardware address in the line.
				 */
				num = sscanf(line, "%s 0x%x 0x%x %17s %15s\n",
				     ip, &type, &flags, mask, ifname);
				mac[0] = 0;
			}

			ArpRouteInfo ari;
			ari.iface = towstr(ifname);
			ari.ip = towstr(ip);
			ari.mac = towstr(mac);
			ari.sa_family = AF_PACKET;
			ari.flags = flags;
			ari.hwType = type;
			ari.ifnameIndex = -1;

			arp.push_back(ari);
		}

	} while(0);

	fclose(routes);	

	return true;
}

void NetRoutes::SetNameByIndex(const char *ifname, uint32_t ifnameIndex)
{
	for( auto & route : inet ) {
		if( route.valid.ifnameIndex && route.ifnameIndex == ifnameIndex ) {
			route.iface = towstr(ifname);
			route.valid.iface = 1;
		}

		if( route.valid.rtnexthop )
			for( auto & item : route.osdep.nhs ) {
				if( item.ifindex == ifnameIndex ) {
					item.iface = std::move(towstr(ifname));
					item.valid.iface = 1;
				}
			}

	}
	for( auto & route : inet6 ) {
		if( route.valid.ifnameIndex && route.ifnameIndex == ifnameIndex ) {
			route.iface = towstr(ifname);
			route.valid.iface = 1;
		}

		if( route.valid.rtnexthop )
			for( auto & item : route.osdep.nhs ) {
				if( item.ifindex == ifnameIndex ) {
					item.iface = std::move(towstr(ifname));
					item.valid.iface = 1;
				}
			}
	}

	for( auto & route : arp ) {
		if( route.valid.ifnameIndex && route.ifnameIndex == ifnameIndex ) {
			route.iface = towstr(ifname);
			route.valid.iface = 1;
		}
	}
	return;
}

bool NetRoutes::UpdateNeigbours(const NeighborRecord * nb)
{
	if( !nb )
		return false;

	while( nb->ndm ) {
		LOG_INFO("---------------- NeighborRecord ---------------------\n");
		LOG_INFO("nlh.nlmsg_type: %d (%s)\n", nb->nlm.nlmsg_type, nlmsgtype(nb->nlm.nlmsg_type));
		LOG_INFO("ndm_family:  %d (%s)\n", nb->ndm->ndm_family, familyname(nb->ndm->ndm_family));
		LOG_INFO("ndm_ifindex: %d\n", nb->ndm->ndm_ifindex);
		LOG_INFO("ndm_state:   0x%04X (%s)\n", nb->ndm->ndm_state, ndmsgstate(nb->ndm->ndm_state));
		LOG_INFO("ndm_flags:   0x%02X (%s)\n", nb->ndm->ndm_flags, ndmsgflagsname(nb->ndm->ndm_flags));
		LOG_INFO("ndm_type:    0x%02X (%s)\n", nb->ndm->ndm_type, rttype(nb->ndm->ndm_type));

		ArpRouteInfo ari;
		ari.sa_family = nb->ndm->ndm_family;
		ari.ifnameIndex = nb->ndm->ndm_ifindex;
		ari.flags = nb->ndm->ndm_flags;
		ari.type = nb->ndm->ndm_type;
		ari.state = nb->ndm->ndm_state;
		ari.valid.state = 1;
		ari.valid.type = 1;
		ari.valid.flags = 1;
		ari.valid.ifnameIndex = (ari.ifnameIndex != (uint32_t)-1);

		uint32_t addrlen = 0;
		uint32_t family = nb->ndm->ndm_family;
		const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
		char s[maxlen+1] = {0};

		switch( ari.sa_family ) {
		case AF_INET:
		case RTNL_FAMILY_IPMR:
			addrlen = sizeof(uint32_t);
			family = AF_INET;
			break;
		case AF_INET6:
		case RTNL_FAMILY_IP6MR:
			addrlen = sizeof(struct in6_addr);
			family = AF_INET6;
			break;
		case AF_BRIDGE:
			if( nb->tb[NDA_DST] ) {
				if( RTA_PAYLOAD(nb->tb[NDA_DST]) == sizeof(struct in6_addr) ) {
					addrlen = sizeof(struct in6_addr);
					family = AF_INET6;
				} else {
					addrlen = sizeof(uint32_t);
					family = AF_INET;
				}
			}
			break;
		//case AF_MCTP:
		//	addrlen = sizeof(uint8_t);
		//	break;
		default:
			LOG_ERROR("unsupported family %u\n", ari.sa_family);
			continue;
		};

		if( nb->tb[NDA_DST] && RTA_PAYLOAD(nb->tb[NDA_DST]) >= addrlen ) {
			if( inet_ntop(family, RTA_DATA(nb->tb[NDA_DST]), s, maxlen) ) {
				ari.ip = towstr(s);
				ari.valid.ip = 1;
			}
			LOG_INFO("NDA_DST:    %S\n", ari.ip.c_str());
		}

		if( nb->tb[NDA_LLADDR] && RTA_PAYLOAD(nb->tb[NDA_LLADDR]) >= ETH_ALEN ) {
			unsigned char * mac = (unsigned char *)RTA_DATA(nb->tb[NDA_LLADDR]);
			if( snprintf(s, maxlen, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) > 0 ) {
				ari.mac = towstr(s);
				ari.valid.mac = 1;
			}
			LOG_INFO("NDA_LLADDR: %S\n", ari.mac.c_str());
		}

		if( nb->tb[NDA_PROBES] && RTA_PAYLOAD(nb->tb[NDA_PROBES]) >= sizeof(uint32_t) ) {
			ari.probes = RTA_UINT32_T(nb->tb[NDA_PROBES]);
			ari.valid.probes = 1;
			LOG_INFO("NDA_PROBES: %d\n", ari.probes);
		}

		if( nb->tb[NDA_CACHEINFO] && RTA_PAYLOAD(nb->tb[NDA_CACHEINFO]) >= sizeof(arproute_cacheinfo) ) {
			ari.ci = *(arproute_cacheinfo *)RTA_DATA(nb->tb[NDA_CACHEINFO]);
			ari.valid.ci = 1;
			LOG_INFO("NDA_CACHEINFO: ndm_confirmed %d\n", ari.ci.ndm_confirmed);
			LOG_INFO("NDA_CACHEINFO: ndm_used      %d\n", ari.ci.ndm_used);
			LOG_INFO("NDA_CACHEINFO: ndm_updated   %d\n", ari.ci.ndm_updated);
			LOG_INFO("NDA_CACHEINFO: ndm_refcnt    %d\n", ari.ci.ndm_refcnt);
			ari.LogCacheInfo();
		}

		if( nb->tb[NDA_VLAN] && RTA_PAYLOAD(nb->tb[NDA_VLAN]) >= sizeof(uint16_t) ) {
			ari.vlan = RTA_UINT16_T(nb->tb[NDA_VLAN]);
			ari.valid.vlan = 1;
			LOG_INFO("NDA_VLAN:   %d\n", ari.vlan);
		}

		if( nb->tb[NDA_PORT] && RTA_PAYLOAD(nb->tb[NDA_PORT]) >= sizeof(uint16_t) ) {
			ari.port = RTA_UINT16_T(nb->tb[NDA_PORT]);
			ari.valid.port = 1;
			LOG_INFO("NDA_PORT:   %d\n", ari.port);
		}

		if( nb->tb[NDA_PROTOCOL] && RTA_PAYLOAD(nb->tb[NDA_PROTOCOL]) >= sizeof(uint8_t) ) {
			ari.protocol = RTA_UINT8_T(nb->tb[NDA_PROTOCOL]);
			ari.valid.protocol = 1;
			LOG_INFO("NDA_PROTOCOL: %d (%s)\n", ari.protocol, rtprotocoltype(ari.protocol));
		}

		if( nb->tb[NDA_IFINDEX] && RTA_PAYLOAD(nb->tb[NDA_IFINDEX]) >= sizeof(uint32_t) ) {
			ari.ifnameIndex = RTA_UINT32_T(nb->tb[NDA_IFINDEX]);
			ari.valid.ifnameIndex = 1;
			LOG_INFO("NDA_IFINDEX: %d\n", ari.ifnameIndex);
		}

		if( nb->tb[NDA_NH_ID] && RTA_PAYLOAD(nb->tb[NDA_NH_ID]) >= sizeof(uint32_t) ) {
			ari.nh_id = RTA_UINT32_T(nb->tb[NDA_NH_ID]);
			ari.valid.nh_id = 1;
			LOG_INFO("NDA_NH_ID: %d\n", ari.nh_id);
		}

		if( nb->tb[NDA_FLAGS_EXT] && RTA_PAYLOAD(nb->tb[NDA_FLAGS_EXT]) >= sizeof(uint32_t) ) {
			ari.flags_ext = RTA_UINT32_T(nb->tb[NDA_FLAGS_EXT]);
			ari.valid.flags_ext = 1;
			LOG_INFO("NDA_FLAGS_EXT: %d\n", ari.flags_ext, ndmsgflags_extname(ari.flags_ext));
		}

		if( nb->tb[NDA_VNI] && RTA_PAYLOAD(nb->tb[NDA_VNI]) >= sizeof(uint32_t) ) {
			ari.vni = RTA_UINT32_T(nb->tb[NDA_VNI]);
			ari.valid.vni = 1;
			LOG_INFO("NDA_VNI:       %d\n", ari.vni);
		}

		if( nb->tb[NDA_MASTER] && RTA_PAYLOAD(nb->tb[NDA_MASTER]) >= sizeof(uint32_t) ) {
			ari.master = RTA_UINT32_T(nb->tb[NDA_MASTER]);
			ari.valid.master = 1;
			LOG_INFO("NDA_MASTER:    %d\n", ari.master);
		}

		//TODO: [NDA_FDB_EXT_ATTRS]	= { .type = NLA_NESTED }, NFEA_ACTIVITY_NOTIFY (FDB_NOTIFY_BIT)
		arp.push_back(ari);
		nb++;
	}
	return true;
}

bool NetRoutes::UpdateByNetlink(void * netlink, unsigned char af_family)
{
	const RouteRecord * rr = GetRoutes(netlink, af_family);

	if( rr )
	while( rr->rt ) {

		char ifname[MAX_INTERFACE_NAME_LEN] = {0};

		LOG_INFO("-------------------------------------\n");
		LOG_INFO("nlh.nlmsg_type: %d (%s)\n", rr->nlm.nlmsg_type, nlmsgtype(rr->nlm.nlmsg_type));
		LOG_INFO("rtm_family:   %u (%s)\n", rr->rt->rtm_family, familyname(rr->rt->rtm_family));
		LOG_INFO("rtm_dst_len:  %u\n", rr->rt->rtm_dst_len );
		LOG_INFO("rtm_src_len:  %u\n", rr->rt->rtm_src_len );
		LOG_INFO("rtm_tos:      %u\n", rr->rt->rtm_tos );
		LOG_INFO("rtm_table:    %u (%s)\n", rr->rt->rtm_table, rtruletable(rr->rt->rtm_table) );
		LOG_INFO("rtm_protocol: %u (%s)\n", rr->rt->rtm_protocol, rtprotocoltype(rr->rt->rtm_protocol));
		LOG_INFO("rtm_scope:    %u (%s)\n", rr->rt->rtm_scope, rtscopetype(rr->rt->rtm_scope)); // Address scope
		LOG_INFO("rtm_type:     %u (%s)\n", rr->rt->rtm_type, rttype(rr->rt->rtm_type) );
		LOG_INFO("rtm_flags:    0x%08X\n", rr->rt->rtm_flags );

		IpRouteInfo ipr;
		ipr.sa_family = rr->rt->rtm_family;
		ipr.valid.flags = 1;
		ipr.flags = rr->rt->rtm_flags;
		ipr.valid.dstprefixlen = 1;
		ipr.dstprefixlen = rr->rt->rtm_dst_len;
		ipr.osdep.table = rr->rt->rtm_table;
		ipr.valid.table = (bool)ipr.osdep.table;

		ipr.valid.protocol = 1;
		ipr.osdep.protocol = rr->rt->rtm_protocol;
		ipr.valid.scope = 1;
		ipr.osdep.scope = rr->rt->rtm_scope;
		ipr.valid.type = 1;
		ipr.osdep.type = rr->rt->rtm_type;
		ipr.valid.tos = 1;
		ipr.osdep.tos = rr->rt->rtm_tos;

		uint32_t addrlen = 0;
		const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
		char s[maxlen+1] = {0};

		switch( ipr.sa_family ) {
		case AF_INET:
			addrlen = sizeof(uint32_t);
			break;
		case AF_INET6:
			addrlen = sizeof(struct in6_addr);
			break;
		//case AF_MCTP:
		//	addrlen = sizeof(uint8_t);
		//	break;
		default:
			LOG_ERROR("unsupported family %u\n", ipr.sa_family);
			continue;
		};

		if( rr->tb[RTA_OIF] && RTA_PAYLOAD(rr->tb[RTA_OIF]) >= sizeof(uint32_t) ) {
			ipr.ifnameIndex = RTA_UINT32_T(rr->tb[RTA_OIF]);
			ipr.valid.ifnameIndex = 1;
			if_indextoname(ipr.ifnameIndex, ifname);
			ipr.iface = towstr(ifname);
			ipr.valid.iface = !ipr.iface.empty();
			LOG_INFO("RTA_OIF: %s (%u)\n", ifname, ipr.ifnameIndex);
		}

		if( rr->tb[RTA_TABLE] && RTA_PAYLOAD(rr->tb[RTA_TABLE]) >= sizeof(uint32_t) ) {
			ipr.osdep.table = RTA_UINT32_T(rr->tb[RTA_TABLE]);
			ipr.valid.table = 1;
			LOG_INFO("RTA_TABLE %u (%s)\n", ipr.osdep.table, rtruletable(ipr.osdep.table));
		}
		if( rr->tb[RTA_GATEWAY] && RTA_PAYLOAD(rr->tb[RTA_GATEWAY]) >= addrlen ) {
			if( inet_ntop(ipr.sa_family, RTA_DATA(rr->tb[RTA_GATEWAY]), s, maxlen) ) {
				ipr.gateway = towstr(s);
				ipr.valid.gateway = 1;
			}
			LOG_INFO("RTA_GATEWAY: %S\n", ipr.gateway.c_str());
		}
		if( rr->tb[RTA_DST] && RTA_PAYLOAD(rr->tb[RTA_DST]) >= addrlen ) {
			ipr.destIpandMask = destIpandMask(ipr.sa_family, RTA_DATA(rr->tb[RTA_DST]), ipr.dstprefixlen);
			ipr.valid.destIpandMask = 1;
			LOG_INFO("RTA_DST: %S\n", ipr.destIpandMask.c_str());
		}
		if( rr->tb[RTA_PRIORITY] && RTA_PAYLOAD(rr->tb[RTA_PRIORITY]) >= sizeof(int32_t) ) {
			ipr.osdep.metric = RTA_INT32_T(rr->tb[RTA_PRIORITY]);
			ipr.valid.metric = 1;
			LOG_INFO("RTA_PRIORITY: %d\n", ipr.osdep.metric);
		}
		if( rr->tb[RTA_PREFSRC] && RTA_PAYLOAD(rr->tb[RTA_PREFSRC]) >= addrlen ) {
			if( inet_ntop(ipr.sa_family, RTA_DATA(rr->tb[RTA_PREFSRC]), s, maxlen) ) {
				ipr.prefsrc = towstr(s);
				ipr.valid.prefsrc = 1;
			}
			LOG_INFO("RTA_PREFSRC: %S\n", ipr.prefsrc.c_str());
		}
		if( rr->tb[RTA_PREF] && RTA_PAYLOAD(rr->tb[RTA_PREF]) >= sizeof(uint8_t) ) {
			ipr.osdep.icmp6pref = RTA_UINT8_T(rr->tb[RTA_PREF]);
			ipr.valid.icmp6pref = 1;
			LOG_INFO("RTA_PREF: %u (%s)\n", ipr.osdep.icmp6pref, rticmp6pref(ipr.osdep.icmp6pref));
		}

		if( rr->tb[RTA_CACHEINFO] && RTA_PAYLOAD(rr->tb[RTA_CACHEINFO]) >= sizeof(struct rta_cacheinfo) ) {
			const size_t size = sizeof(ipr.osdep.rtcache) <= sizeof(struct rta_cacheinfo) ? \
				sizeof(ipr.osdep.rtcache):sizeof(struct rta_cacheinfo);
			memmove(&ipr.osdep.rtcache, RTA_DATA(rr->tb[RTA_CACHEINFO]), size);
			ipr.valid.rtcache = 1;
			LOG_INFO("RTA_CACHEINFO: size %u\n", RTA_PAYLOAD(rr->tb[RTA_CACHEINFO]));
			ipr.LogRtCache();
		}

		if( rr->tb[RTA_METRICS] && RTA_PAYLOAD(rr->tb[RTA_METRICS]) >= sizeof(struct rtattr) ) {
			struct rtattr * mrta[RTAX_MAX+1];
			if( FillAttr((struct rtattr*)RTA_DATA(rr->tb[RTA_METRICS]),
						mrta,
						RTAX_MAX,
						(short unsigned int)(~NLA_F_NESTED),
						RTA_PAYLOAD(rr->tb[RTA_METRICS]),
						rtaxtype) ) {
				//static_assert( (sizeof(ipr.osdep.rtmetrics)/sizeof(uint32_t)) == (RTAX_MAX+1) );
				for(uint32_t index = 0; index < sizeof(ipr.osdep.rtmetrics)/sizeof(uint32_t); index++ ) {
					if( mrta[index] && RTA_PAYLOAD(mrta[index]) >= sizeof(uint32_t) ) {
						((uint32_t *)&ipr.osdep.rtmetrics)[index] = RTA_UINT32_T(mrta[index]);
						LOG_INFO("%s: %d\n", rtaxtype(index), ((uint32_t *)&ipr.osdep.rtmetrics)[index]);
					}
				}
				ipr.valid.rtmetrics = 1;
				ipr.valid.hoplimit = 1;
				ipr.hoplimit = ipr.osdep.rtmetrics.hoplimit;
			}
			LOG_INFO("RTA_METRICS: ... ok\n");
		}

		if( rr->tb[RTA_SRC] && RTA_PAYLOAD(rr->tb[RTA_SRC]) >= addrlen ) {
			if( inet_ntop(ipr.sa_family, RTA_DATA(rr->tb[RTA_SRC]), s, maxlen) ) {
				ipr.osdep.fromsrcIpandMask = (towstr(s) + L"/" + std::to_wstring(rr->rt->rtm_src_len));
				ipr.valid.fromsrcIpandMask = 1;
			}
			LOG_INFO("RTA_SRC: %S\n", ipr.osdep.fromsrcIpandMask.c_str());
		}

		if( rr->tb[RTA_VIA] && RTA_PAYLOAD(rr->tb[RTA_VIA]) >= sizeof(struct rtvia) ) {
			const struct rtvia *via = (const struct rtvia *)RTA_DATA(rr->tb[RTA_VIA]);
			ipr.osdep.rtvia_family = via->rtvia_family;
			if( inet_ntop(ipr.osdep.rtvia_family, &via->rtvia_addr, s, maxlen) ) {
				ipr.osdep.rtvia_addr = towstr(s);
				ipr.valid.rtvia = 1;
			}
			LOG_INFO("RTA_VIA:     %S\n", ipr.osdep.rtvia_addr.c_str());
		}

		if( rr->tb[RTA_ENCAP_TYPE] && rr->tb[RTA_ENCAP] && RTA_PAYLOAD(rr->tb[RTA_ENCAP_TYPE]) >= sizeof(uint16_t) ) {
			ipr.osdep.enc.type = RTA_UINT16_T(rr->tb[RTA_ENCAP_TYPE]);
			if( FillEncap(&ipr.osdep.enc, rr->tb[RTA_ENCAP] ) )
				ipr.valid.encap = 1;
		}

		if( rr->tb[RTA_MULTIPATH] && RTA_PAYLOAD(rr->tb[RTA_MULTIPATH]) >= sizeof(struct rtnexthop) ) {
			const struct rtnexthop *nh = (struct rtnexthop *)RTA_DATA(rr->tb[RTA_MULTIPATH]);
			int len = RTA_PAYLOAD(rr->tb[RTA_MULTIPATH]);

			ipr.valid.rtnexthop = 1;

			while( len >= static_cast<int>(sizeof(*nh)) ) {
				LOG_INFO("RTA_MULTIPATH: rtnh_len     %d\n", nh->rtnh_len);
				LOG_INFO("RTA_MULTIPATH: rtnh_flags   0x%02X (%s)\n", nh->rtnh_flags, rtnhflagsname(nh->rtnh_flags));
				LOG_INFO("RTA_MULTIPATH: rtnh_hops    %d\n", nh->rtnh_hops);
				LOG_INFO("RTA_MULTIPATH: rtnh_ifindex %d\n", nh->rtnh_ifindex);

				NextHope netHope;
				netHope.valid.nexthope = 1;
				netHope.flags = nh->rtnh_flags;
				netHope.weight = nh->rtnh_hops;
				netHope.ifindex = nh->rtnh_ifindex;

				if (nh->rtnh_len > len) {
					ipr.osdep.nhs.push_back(netHope);
					break;
				}

				struct rtattr * rta[RTA_MAX + 1];

				if( nh->rtnh_len > sizeof(*nh) && FillAttr((struct rtattr*)RTNH_DATA(nh),
						rta,
						RTA_MAX,
						(short unsigned int)(~NLA_F_NESTED),
						nh->rtnh_len - sizeof(*nh),
						rtatype) ) {

					if( rta[RTA_ENCAP_TYPE] && rta[RTA_ENCAP] && RTA_PAYLOAD(rta[RTA_ENCAP_TYPE]) >= sizeof(uint16_t) ) {
						netHope.enc.type = RTA_UINT16_T(rta[RTA_ENCAP_TYPE]);
						if( FillEncap(&netHope.enc, rta[RTA_ENCAP] ) )
							netHope.valid.encap = 1;
					}

					if( rta[RTA_GATEWAY] && RTA_PAYLOAD(rta[RTA_GATEWAY]) >= addrlen ) {
						if( inet_ntop(ipr.sa_family, RTA_DATA(rta[RTA_GATEWAY]), s, maxlen) ) {
							netHope.gateway = towstr(s);
							netHope.valid.gateway = 1;
						}
						LOG_INFO("RTA_GATEWAY: %S\n", netHope.gateway.c_str());
					}

					if( rta[RTA_VIA] && RTA_PAYLOAD(rta[RTA_VIA]) >= sizeof(struct rtvia) ) {
						const struct rtvia *via = (const struct rtvia *)RTA_DATA(rta[RTA_VIA]);
						netHope.rtvia_family = via->rtvia_family;
						if( inet_ntop(netHope.rtvia_family, &via->rtvia_addr, s, maxlen) ) {
							netHope.rtvia_addr = towstr(s);
							netHope.valid.rtvia = 1;
						}
						LOG_INFO("RTA_VIA:     %S\n", netHope.rtvia_addr.c_str());
					}

					if( rta[RTA_FLOW] && RTA_PAYLOAD(rta[RTA_FLOW]) >= sizeof(uint32_t) ) {
						uint32_t flow = RTA_UINT32_T(rta[RTA_FLOW]);
						netHope.flowto = flow & 0xFFFF;
						netHope.flowfrom = flow >> 16;
						netHope.valid.flowto = 1;
						netHope.valid.flowfrom = (netHope.flowfrom != 0);
						LOG_INFO("RTA_FLOW:    %d/%d\n", netHope.flowfrom, netHope.flowto);
					}

					// TODO: RTA_NEWDST
				}

				ipr.osdep.nhs.push_back(netHope);
				len -= NLMSG_ALIGN(nh->rtnh_len);
				nh = RTNH_NEXT(nh);
			}
			LOG_INFO("RTA_MULTIPATH ... ok\n");
		}

		if( rr->tb[RTA_NH_ID] && RTA_PAYLOAD(rr->tb[RTA_NH_ID]) >= sizeof(uint32_t) ) {
			ipr.osdep.nhid = RTA_UINT32_T(rr->tb[RTA_NH_ID]);
			ipr.valid.nhid = 1;
			LOG_INFO("RTA_NH_ID:    %u\n", ipr.osdep.nhid);
		}

// TODO:

/*const struct nla_policy rtm_ipv4_policy[RTA_MAX + 1] = {
	[RTA_UNSPEC]		= { .strict_start_type = RTA_DPORT + 1 },
	[RTA_DST]		= { .type = NLA_U32 },
	[RTA_SRC]		= { .type = NLA_U32 },
	[RTA_IIF]		= { .type = NLA_U32 },
	[RTA_OIF]		= { .type = NLA_U32 },
	[RTA_GATEWAY]		= { .type = NLA_U32 },
	[RTA_PRIORITY]		= { .type = NLA_U32 },
	[RTA_PREFSRC]		= { .type = NLA_U32 },
	[RTA_METRICS]		= { .type = NLA_NESTED },
	[RTA_MULTIPATH]		= { .len = sizeof(struct rtnexthop) },
	[RTA_FLOW]		= { .type = NLA_U32 },
	[RTA_ENCAP_TYPE]	= { .type = NLA_U16 },
	[RTA_ENCAP]		= { .type = NLA_NESTED },
	[RTA_UID]		= { .type = NLA_U32 },
	[RTA_MARK]		= { .type = NLA_U32 },
	[RTA_TABLE]		= { .type = NLA_U32 },
	[RTA_IP_PROTO]		= { .type = NLA_U8 },
	[RTA_SPORT]		= { .type = NLA_U16 },
	[RTA_DPORT]		= { .type = NLA_U16 },
	[RTA_NH_ID]		= { .type = NLA_U32 },
};

static const struct nla_policy rtm_ipv6_policy[RTA_MAX+1] = {
	[RTA_UNSPEC]		= { .strict_start_type = RTA_DPORT + 1 },
	[RTA_GATEWAY]           = { .len = sizeof(struct in6_addr) },
	[RTA_PREFSRC]		= { .len = sizeof(struct in6_addr) },
	[RTA_OIF]               = { .type = NLA_U32 },
	[RTA_IIF]		= { .type = NLA_U32 },
	[RTA_PRIORITY]          = { .type = NLA_U32 },
	[RTA_METRICS]           = { .type = NLA_NESTED },
	[RTA_MULTIPATH]		= { .len = sizeof(struct rtnexthop) },
	[RTA_PREF]              = { .type = NLA_U8 },
	[RTA_ENCAP_TYPE]	= { .type = NLA_U16 },
	[RTA_ENCAP]		= { .type = NLA_NESTED },
	[RTA_EXPIRES]		= { .type = NLA_U32 },
	[RTA_UID]		= { .type = NLA_U32 },
	[RTA_MARK]		= { .type = NLA_U32 },
	[RTA_TABLE]		= { .type = NLA_U32 },
	[RTA_IP_PROTO]		= { .type = NLA_U8 },
	[RTA_SPORT]		= { .type = NLA_U16 },
	[RTA_DPORT]		= { .type = NLA_U16 },
	[RTA_NH_ID]		= { .type = NLA_U32 },
};
*/
		if( ipr.valid.gateway || ipr.valid.rtvia ) {
			LOG_INFO("push GATEWAY\n");
			if( ipr.sa_family == AF_INET )
				inet.push_front(ipr);
			else if( ipr.sa_family == AF_INET6 )
				inet6.push_front(ipr);
		} else  {
			LOG_INFO("push NORMAL\n");
			if( ipr.sa_family == AF_INET )
				inet.push_back(ipr);
			else if( ipr.sa_family == AF_INET6 )
				inet6.push_back(ipr);
		}

		rr++;
	}

	rr = 0;

	const RuleRecord * r = GetRules(netlink, AF_UNSPEC);

	if( r )
	while( r->frh ) {

		LOG_INFO("---------------- RuleRecord ---------------------\n");
		LOG_INFO("nlh.nlmsg_type: %d (%s)\n", r->nlm.nlmsg_type, nlmsgtype(r->nlm.nlmsg_type));
		LOG_INFO("family:  %d (%s)\n", r->frh->family, familyname(r->frh->family));
		LOG_INFO("dst_len: %d\n", r->frh->dst_len);
		LOG_INFO("src_len: %d\n", r->frh->src_len);
		LOG_INFO("tos:     0x%02X\n", r->frh->tos);
		LOG_INFO("table:   %d (%s)\n", r->frh->table, rtruletable(r->frh->table));
		LOG_INFO("res1:    %d\n", r->frh->res1);
		LOG_INFO("res2:    %d\n", r->frh->res2);
		LOG_INFO("action:  %d (%s)\n", r->frh->action, fractionrule(r->frh->action));
		LOG_INFO("flags:   0x%08X (%s)\n", r->frh->flags, fibruleflagsname(r->frh->flags));

		RuleRouteInfo rri;
		rri.family = r->frh->family;
		rri.fwmark = 0;
		rri.fwmask = 0;
		rri.valid = {0};
		rri.tos = r->frh->tos;
		rri.flags = r->frh->flags;
		rri.type = r->nlm.nlmsg_type;
		rri.priority = 0;
		rri.table = r->frh->table;
		rri.action = r->frh->action;

		uint32_t addrlen = 0;
		uint32_t family = r->frh->family;
		const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
		char s[maxlen+1] = {0};

		switch( rri.family ) {
		case AF_INET:
		case RTNL_FAMILY_IPMR:
			addrlen = sizeof(uint32_t);
			family = AF_INET;
			break;
		case AF_INET6:
		case RTNL_FAMILY_IP6MR:
			addrlen = sizeof(struct in6_addr);
			family = AF_INET6;
			break;
		//case AF_MCTP:
		//	addrlen = sizeof(uint8_t);
		//	break;
		default:
			LOG_ERROR("unsupported family %u\n", rri.family);
			continue;
		};

		if( r->tb[FRA_PRIORITY] && RTA_PAYLOAD(r->tb[FRA_PRIORITY]) >= sizeof(uint32_t) ) {
			rri.priority = RTA_UINT32_T(r->tb[FRA_PRIORITY]);
			rri.valid.priority = 1;
			LOG_INFO("FRA_PRIORITY:           %d\n", rri.priority);
		}

		if( r->tb[FRA_SRC] && RTA_PAYLOAD(r->tb[FRA_SRC]) >= addrlen ) {
			rri.fromIpandMask = destIpandMask(family, RTA_DATA(r->tb[FRA_SRC]), r->frh->src_len);
			LOG_INFO("FRA_SRC:                %S\n", rri.fromIpandMask.c_str());
		} else if( r->frh->src_len ) {
			rri.fromIpandMask = L"0/";
			rri.fromIpandMask += std::to_wstring(r->frh->src_len);
		} else
			rri.fromIpandMask = L"all";

		rri.valid.fromIpandMask = 1;


		if( r->tb[FRA_DST] && RTA_PAYLOAD(r->tb[FRA_DST]) >= addrlen ) {
			rri.toIpandMask = destIpandMask(family, RTA_DATA(r->tb[FRA_DST]), r->frh->dst_len);
			LOG_INFO("FRA_DST:                %S\n", rri.toIpandMask.c_str());
			rri.valid.toIpandMask = 1;
		} else if( r->frh->dst_len ) {
			rri.toIpandMask = L"to 0/" + std::to_wstring(r->frh->dst_len);
			rri.valid.toIpandMask = 1;
		}

		if( r->tb[FRA_FWMARK] && RTA_PAYLOAD(r->tb[FRA_FWMARK]) >= sizeof(uint32_t) ) {
			rri.fwmark = RTA_UINT32_T(r->tb[FRA_FWMARK]);
			rri.valid.fwmark = 1;
			LOG_INFO("FRA_FWMARK:             0x%08X\n", rri.fwmark);
		}
		if( r->tb[FRA_FWMASK] && RTA_PAYLOAD(r->tb[FRA_FWMASK]) >= sizeof(uint32_t) ) {
			rri.fwmask = RTA_UINT32_T(r->tb[FRA_FWMASK]);
			rri.valid.fwmask = (rri.fwmask != 0xFFFFFFFF);
			LOG_INFO("FRA_FWMARK/FRA_FWMASK:  0x%08X/0x%08X\n", rri.fwmark, rri.fwmask);
		}

		if( r->tb[FRA_IIFNAME] && RTA_PAYLOAD(r->tb[FRA_IIFNAME]) >= sizeof(uint8_t) ) {
			rri.iiface = towstr((const char *)RTA_DATA(r->tb[FRA_IIFNAME]));
			rri.valid.iiface = 1;
			LOG_INFO("FRA_IIFNAME:            %S\n", rri.iiface.c_str());
		}
		if( r->tb[FRA_OIFNAME] && RTA_PAYLOAD(r->tb[FRA_OIFNAME]) >= sizeof(uint8_t) ) {
			rri.oiface = towstr((const char *)RTA_DATA(r->tb[FRA_OIFNAME]));
			rri.valid.oiface = 1;
			LOG_INFO("FRA_OIFNAME:            %S\n", rri.oiface.c_str());
		}

		// on android #define FRA_UID_START FRA_PAD and #define FRA_UID_END FRA_L3MDEV
		if( r->tb[FRA_L3MDEV] && !r->tb[FRA_PAD] && RTA_PAYLOAD(r->tb[FRA_L3MDEV]) >= sizeof(uint8_t) ) {
			rri.l3mdev = RTA_UINT8_T(r->tb[FRA_L3MDEV]);
			rri.valid.l3mdev = 1;
			LOG_INFO("FRA_L3MDEV:             %d\n", rri.l3mdev);
		}

		if( r->tb[FRA_UID_START] && r->tb[FRA_UID_END] && \
				RTA_PAYLOAD(r->tb[FRA_UID_START]) >= sizeof(uint32_t) &&
				RTA_PAYLOAD(r->tb[FRA_UID_END]) >= sizeof(uint32_t) ) {
			rri.uid_range.start = RTA_UINT32_T(r->tb[FRA_UID_START]);
			rri.uid_range.end = RTA_UINT32_T(r->tb[FRA_UID_END]);
			assert( !rri.valid.l3mdev );
			rri.valid.uid_range = 1;
			LOG_INFO("FRA_UID_START:          %d\n", rri.uid_range.start);
			LOG_INFO("FRA_UID_END:            %d\n", rri.uid_range.end);
		}

		if( r->tb[FRA_UID_RANGE] && RTA_PAYLOAD(r->tb[FRA_UID_RANGE]) >= sizeof(struct uid_range) ) {
			rri.uid_range = *(struct uid_range *)RTA_DATA(r->tb[FRA_UID_RANGE]);
			assert( !rri.valid.uid_range );
			rri.valid.uid_range = 1;
			LOG_INFO("FRA_UID_RANGE:          %d-%d\n", rri.uid_range.start, rri.uid_range.end);
		}

		if( r->tb[FRA_IP_PROTO] && RTA_PAYLOAD(r->tb[FRA_IP_PROTO]) >= sizeof(uint8_t) ) {
			rri.ip_protocol = RTA_UINT8_T(r->tb[FRA_IP_PROTO]);
			rri.valid.ip_protocol = 1;
			LOG_INFO("FRA_IP_PROTO:           %d (%s)\n", rri.ip_protocol, iprotocolname(rri.ip_protocol));
		}

		if( r->tb[FRA_SPORT_RANGE] && RTA_PAYLOAD(r->tb[FRA_SPORT_RANGE]) >= sizeof(struct port_range) ) {
			rri.sport_range = *(struct port_range *)RTA_DATA(r->tb[FRA_SPORT_RANGE]);
			rri.valid.sport_range = 1;
			LOG_INFO("FRA_SPORT_RANGE:        %u-%u\n", rri.sport_range.start, rri.sport_range.end);
		}
		if( r->tb[FRA_DPORT_RANGE] && RTA_PAYLOAD(r->tb[FRA_DPORT_RANGE]) >= sizeof(struct port_range) ) {
			rri.dport_range = *(struct port_range *)RTA_DATA(r->tb[FRA_DPORT_RANGE]);
			rri.valid.dport_range = 1;
			LOG_INFO("FRA_DPORT_RANGE:        %u-%u\n", rri.dport_range.start, rri.dport_range.end);
		}

		if( r->tb[FRA_TUN_ID] && RTA_PAYLOAD(r->tb[FRA_TUN_ID]) >= sizeof(uint64_t) ) {
			rri.tun_id = ntohll(RTA_UINT64_T(r->tb[FRA_TUN_ID]));
			rri.valid.tun_id = 1;
			LOG_INFO("FRA_TUN_ID:             %lld\n", rri.tun_id);
		}
		if( r->tb[FRA_TABLE] && RTA_PAYLOAD(r->tb[FRA_TABLE]) >= sizeof(uint32_t) ) {
			rri.table = RTA_UINT32_T(r->tb[FRA_TABLE]);
			rri.valid.table = 1;
			LOG_INFO("FRA_TABLE:              %d\n", rri.table);
		}

		if( r->tb[FRA_SUPPRESS_PREFIXLEN] && RTA_PAYLOAD(r->tb[FRA_SUPPRESS_PREFIXLEN]) >= sizeof(uint32_t) ) {
			rri.suppress_prefixlength = RTA_UINT32_T(r->tb[FRA_SUPPRESS_PREFIXLEN]);
			rri.valid.suppress_prefixlength = (rri.suppress_prefixlength != (uint32_t)-1);
			LOG_INFO("FRA_SUPPRESS_PREFIXLEN: %d\n", rri.suppress_prefixlength);
		}
		if( r->tb[FRA_SUPPRESS_IFGROUP] && RTA_PAYLOAD(r->tb[FRA_SUPPRESS_IFGROUP]) >= sizeof(uint32_t) ) {
			rri.suppress_ifgroup = RTA_UINT32_T(r->tb[FRA_SUPPRESS_IFGROUP]);
			rri.valid.suppress_ifgroup = (rri.suppress_ifgroup != (uint32_t)-1);
			LOG_INFO("FRA_SUPPRESS_IFGROUP:   %d\n", rri.suppress_ifgroup);
		}

		if( r->tb[FRA_FLOW] && RTA_PAYLOAD(r->tb[FRA_FLOW]) >= sizeof(uint32_t) ) {
			uint32_t flow = RTA_UINT32_T(r->tb[FRA_FLOW]);
			rri.flowto = flow & 0xFFFF;
			rri.flowfrom = flow >> 16;
			rri.valid.flowto = 1;
			rri.valid.flowfrom = (rri.flowfrom != 0);
			LOG_INFO("FRA_FLOW:               %d/%d\n", rri.flowfrom, rri.flowto);
		}
		if( r->tb[FRA_GOTO] && RTA_PAYLOAD(r->tb[FRA_GOTO]) >= sizeof(uint32_t) ) {
			rri.goto_priority = RTA_UINT32_T(r->tb[FRA_GOTO]);
			rri.valid.goto_priority = 1;
			LOG_INFO("FRA_GOTO:               %d\n", rri.goto_priority);
		}

		if( r->frh->action == RTN_NAT && r->tb[RTA_GATEWAY] && RTA_PAYLOAD(r->tb[RTA_GATEWAY]) ) {
			if( inet_ntop(family, RTA_DATA(r->tb[RTA_GATEWAY]), s, maxlen) ) {
				rri.gateway = towstr(s);
				rri.valid.gateway = 1;
			}
			LOG_INFO("RTA_GATEWAY: %S\n", rri.gateway.c_str());
		}

		if( r->tb[FRA_PROTOCOL] && RTA_PAYLOAD(r->tb[FRA_PROTOCOL]) >= sizeof(uint8_t) ) {
			rri.protocol = RTA_UINT8_T(r->tb[FRA_PROTOCOL]);
			rri.valid.protocol = 1;
			LOG_INFO("FRA_PROTOCOL:           %d (%s)\n", rri.protocol, rtprotocoltype(rri.protocol));
		}

		rri.ToRuleString();
		rri.Log();
		switch( rri.family ) {
		case AF_INET:
			rule.push_back(rri);
			break;
		case AF_INET6:
			rule6.push_back(rri);
			break;
		case RTNL_FAMILY_IPMR:
			mcrule.push_back(rri);
			break;
		case RTNL_FAMILY_IP6MR:
			mcrule6.push_back(rri);
			break;
		};
		r++;
	}

	r = 0;

	if( !UpdateNeigbours(GetNeighbors(netlink, AF_UNSPEC, 0)) )
		return false;

	UpdateNeigbours(GetNeighbors(netlink, AF_UNSPEC, NTF_PROXY));

	const LinkRecord * lr = GetLinks(netlink);
	if( !lr )
		return false;

	while( lr->ifm ) {
		LOG_INFO("-------------------------------------\n");
		LOG_INFO("rtm_family:   %u (%s)\n", lr->ifm->ifi_family, familyname(lr->ifm->ifi_family));
		LOG_INFO("ifi_type:     %u\n", lr->ifm->ifi_type); // ARPHRD_
		LOG_INFO("ifi_index:    %u\n", lr->ifm->ifi_index); // Link index
		LOG_INFO("ifi_flags:    0x%08X\n", lr->ifm->ifi_flags); // IFF_* flags
		LOG_INFO("ifi_change:   0x%08X\n", lr->ifm->ifi_change); // IFF_* change mask

		if( lr->tb[IFLA_IFNAME] && RTA_PAYLOAD(lr->tb[IFLA_IFNAME]) >= sizeof(uint8_t) ) {
			LOG_INFO("set index: %u name: %s\n", lr->ifm->ifi_index, (const char *)RTA_DATA(lr->tb[IFLA_IFNAME]));
			ifs[lr->ifm->ifi_index] = towstr((const char *)RTA_DATA(lr->tb[IFLA_IFNAME]));
			SetNameByIndex((const char *)RTA_DATA(lr->tb[IFLA_IFNAME]), lr->ifm->ifi_index);
		}

		lr++;
	}

	return true;
}

bool NetRoutes::UpdateByNetlink(void)
{
	void * netlink = OpenNetlink();
	if(!netlink)
		return false;

	// clear before start
	Clear();

	//bool res = UpdateByNetlink(netlink, AF_UNSPEC); //UpdateByNetlink(netlink, AF_INET) && UpdateByNetlink(netlink, AF_INET6);
	//bool res = UpdateByNetlink(netlink, AF_PACKET);
	bool res = UpdateByNetlink(netlink, AF_UNSPEC);

	CloseNetlink(netlink);

	return res;
}
#endif

bool NetRoutes::Update(void)
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)

	if( !UpdateByNetlink() && !UpdateByProcNet() )
		return false;

	LOG_INFO("inet.size() %u\n", inet.size());
	LOG_INFO("inet6.size() %u\n", inet6.size());
	LOG_INFO("arp.size() %u\n", arp.size());

	uint32_t fw;

	FILE * routes = fopen(PATH_IPV4_FORWARD, "r");
	if( !routes ){
		LOG_ERROR("can`t open \"" PATH_IPV4_FORWARD "\" ... error (%s)\n", errorname(errno));
		return false;
	}

	if( fscanf(routes, "%u\n", &fw) == 1 )
		ipv4_forwarding = (bool)fw;
	else
		LOG_ERROR("can`t get ipv4_forwarding\n");

	fclose(routes);	

	routes = fopen(PATH_IPV6_FORWARD, "r");
	if( !routes ){
		LOG_ERROR("can`t open \"" PATH_IPV6_FORWARD "\" ... error (%s)\n", errorname(errno));
		return false;
	}

	if( fscanf(routes, "%u\n", &fw) == 1 )
		ipv6_forwarding = (bool)fw;
	else
		LOG_ERROR("can`t get ipv6_forwarding\n");

	fclose(routes);	

#else
	// clear before start
	Clear();

	UpdateInterfaces();

	char ifname[MAX_INTERFACE_NAME_LEN] = {0};
	int mib[6] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_DUMP2, 0};
	char * buf = 0;
	size_t size = 0;

	if( sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
		LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_DUMP2) ... error (%s)\n", errorname(errno));
		return false;
	}

	size += size/2;

	buf = (char *)malloc(size);
	if( !buf ) {
		LOG_ERROR("can`t allocate %u bytes\n", size);
		return false;
	}

	do {
		if( sysctl(mib, 6, buf, &size, NULL, 0) < 0) {
			LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_DUMP2) ... error (%s)\n", errorname(errno));
			break;
		}

		char *lim, *next;
		lim = buf + size;

		union {
			uint32_t dummy;
			struct	sockaddr sa;
			uint8_t	data[UINT8_MAX+1];
		} address = {0};

		for (next = buf; next < lim; ) {
			struct rt_msghdr2 * rtm = (struct rt_msghdr2 *)next;
			next += rtm->rtm_msglen;

			IpRouteInfo ipr;
			ipr.ifnameIndex = rtm->rtm_index;
			ipr.valid.ifnameIndex = ipr.ifnameIndex != (uint32_t)-1;

			if( if_indextoname(rtm->rtm_index, ifname) )
				ipr.iface = towstr(ifname);
			ipr.valid.iface = !ipr.iface.empty();
			ipr.flags = rtm->rtm_flags;
			ipr.valid.flags = 1;

			ipr.osdep.parentflags = rtm->rtm_parentflags;
			ipr.valid.parentflags = 1;

			ipr.hoplimit = rtm->rtm_rmx.rmx_hopcount;
			ipr.valid.hoplimit = ipr.hoplimit != 0;

			LOG_INFO("%s: rtm_msglen:       %d (%d)\n", ifname, rtm->rtm_msglen, sizeof(struct rt_msghdr2));
			LOG_INFO("%s: rtm_version:      %d\n", ifname, rtm->rtm_version);
			LOG_INFO("%s: rtm_type:         0x%02X (%s)\n", ifname, rtm->rtm_type, rtmtoname(rtm->rtm_type));
			LOG_INFO("%s: rtm_index:        %d\n", ifname, rtm->rtm_index);
			LOG_INFO("%s: rtm_flags:       \"%s\"(0x%08X) (%s)\n", ifname, RouteFlagsToString(rtm->rtm_flags, 0), rtm->rtm_flags, GetRtmFlags(rtm->rtm_flags));
			LOG_INFO("%s: rtm_addrs:       \"%s\"(0x%08X)\n", ifname, GetRtmAddrs(rtm->rtm_addrs), rtm->rtm_addrs);
			LOG_INFO("%s: rtm_refcnt:       %d\n", ifname, rtm->rtm_refcnt);
			LOG_INFO("%s: rtm_parentflags: \"%s\"(0x%08X)\n", ifname, RouteFlagsToString(rtm->rtm_parentflags, 0), rtm->rtm_parentflags);
			LOG_INFO("%s: rtm_use:          %d (packets sent using this route)\n", ifname, rtm->rtm_use);
			LOG_INFO("%s: rtm_inits:       \"%s\"(0x%08X)\n", ifname, GetRtmInits(rtm->rtm_inits), rtm->rtm_inits);
			LOG_INFO("%s: rmx_locks: 0x%08X\n", ifname, rtm->rtm_rmx.rmx_locks);
			LOG_INFO("%s: rmx_mtu: %d\n", ifname, rtm->rtm_rmx.rmx_mtu);
			LOG_INFO("%s: rmx_hopcount: %d\n", ifname, rtm->rtm_rmx.rmx_hopcount);
			time_t expire_time;
			if( (expire_time = rtm->rtm_rmx.rmx_expire - time((time_t *)0)) > 0 ) {
				LOG_INFO("%s: rmx_expire: %d (%d) (lifetime for route, e.g. redirect)\n", ifname, rtm->rtm_rmx.rmx_expire, (int)expire_time);
				ipr.osdep.expire = (uint32_t)expire_time;
				ipr.valid.expire = 1;
			} else {
				ipr.osdep.expire = 0;
				ipr.valid.expire = 0;
				LOG_INFO("%s: rmx_expire: %d (lifetime for route, e.g. redirect)\n", ifname, rtm->rtm_rmx.rmx_expire);
			}
			LOG_INFO("%s: rmx_recvpipe: %d (inbound delay-bandwidth product)\n", ifname, rtm->rtm_rmx.rmx_recvpipe);
			LOG_INFO("%s: rmx_sendpipe: %d (outbound delay-bandwidth product)\n", ifname, rtm->rtm_rmx.rmx_sendpipe);
			LOG_INFO("%s: rmx_ssthresh: %d (outbound gateway buffer limit)\n", ifname, rtm->rtm_rmx.rmx_ssthresh);
			LOG_INFO("%s: rmx_rtt: %d (estimated round trip time)\n", ifname, rtm->rtm_rmx.rmx_rtt);
			LOG_INFO("%s: rmx_rttvar: %d (estimated rtt variance)\n", ifname, rtm->rtm_rmx.rmx_rttvar);
			LOG_INFO("%s: rmx_pksent: %d (packets sent using this route)\n", ifname, rtm->rtm_rmx.rmx_pksent);
			LOG_INFO("%s: rmx_state: %d (route state)\n", ifname, (int)rtm->rtm_rmx.rmx_state);
			LOG_INFO("%s: rmx_filler[0]: %d rmx_filler[1]: %d rmx_filler[2]: %d (will be used for TCP's peer-MSS cache)\n", ifname, rtm->rtm_rmx.rmx_filler[0], rtm->rtm_rmx.rmx_filler[1], rtm->rtm_rmx.rmx_filler[2]);

			struct sockaddr * sa = (struct sockaddr *)(rtm + 1);
			sa_family_t sa_family = 0, src_sa_family = 0;
			
			for( int i = 0; i < RTAX_MAX; i++) {
				if( rtm->rtm_addrs & (1 << i) ) {

					memset(&address, 0, sizeof(address));
					//static_assert(sizeof(sa->sa_len) == sizeof(uint8_t));
					memmove(&address, sa, sa->sa_len);

					LOG_INFO("%u. %s: sa %p sa->sa_family %u (%s) sa->sa_len %u\n", i, ifname, sa, sa->sa_family, familyname(sa->sa_family), sa->sa_len);

					sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);

					if( i == RTAX_DST ) {
						ipr.destIpandMask = get_sockaddr_str((struct sockaddr *)&address, false);
						ipr.valid.destIpandMask = !ipr.destIpandMask.empty();
						LOG_INFO("%u. %s: destIp: %S\n", i, ifname, ipr.destIpandMask.c_str());
						src_sa_family = ((struct sockaddr *)&address)->sa_family;
						sa_family = src_sa_family;
						continue;
					}

					if( i == RTAX_GATEWAY ) {
						ipr.gateway = get_sockaddr_str((struct sockaddr *)&address, false);
						LOG_INFO("%u. %s: gateway: %S\n", i, ifname, ipr.gateway.c_str());
						ipr.valid.gateway = !ipr.gateway.empty();

						if( ((struct sockaddr *)&address)->sa_family == AF_LINK ) {
							struct sockaddr_dl * sdl = (struct sockaddr_dl *)&address;
							if( ipr.valid.gateway ) {
								if( sdl->sdl_alen == 0 && sdl->sdl_slen == 0 )
									ipr.osdep.gateway_type = IpRouteInfo::InterfaceGatewayType;
								else if( sdl->sdl_type == IFT_ETHER || sdl->sdl_alen == ETHER_ADDR_LEN || (sdl->sdl_alen + sdl->sdl_nlen/2) == ETHER_ADDR_LEN )
									ipr.osdep.gateway_type = IpRouteInfo::MacGatewayType;
								else
									ipr.osdep.gateway_type = IpRouteInfo::OtherGatewayType;
							}
							if( !(rtm->rtm_addrs & RTA_NETMASK) && (sdl->sdl_type == IFT_ETHER || sdl->sdl_alen == ETHER_ADDR_LEN) )
								sa_family = AF_LINK;
						}
						continue;
					}

					if( i == RTAX_NETMASK && sa_family != AF_LINK ) {
						if( ipr.valid.destIpandMask ) {
							((struct sockaddr *)&address)->sa_family = src_sa_family;
							ipr.destIpandMask += L"/";
							if( sa_family == AF_INET )
								ipr.destIpandMask += std::to_wstring(IpMaskToBits(tostr(get_sockaddr_str((struct sockaddr *)&address, true).c_str()).c_str()));
							else if( sa_family == AF_INET6 )
								ipr.destIpandMask += std::to_wstring(Ip6MaskToBits(tostr(get_sockaddr_str((struct sockaddr *)&address, true).c_str()).c_str()));
							else
								ipr.destIpandMask += get_sockaddr_str((struct sockaddr *)&address, true);
							ipr.valid.mask = 1;
							LOG_INFO("%u. %s: destIpandMask: %S\n", i, ifname, ipr.destIpandMask.c_str());
						}
						continue;
					}

					if( i == RTAX_IFA ) {
						ipr.prefsrc = get_sockaddr_str((struct sockaddr *)&address, false);
						ipr.valid.prefsrc = !ipr.prefsrc.empty();
						LOG_INFO("%u. %s: prefsrc: %S\n", i, ifname, ipr.prefsrc.c_str());
					}
				}
			}

			ipr.sa_family = src_sa_family;
			if( sa_family == AF_INET )
				inet.push_back(ipr);
			else if( sa_family == AF_INET6 )
				inet6.push_back(ipr);
			else if( sa_family == AF_LINK ) {
				ArpRouteInfo ari;
				ari.iface = ipr.iface;
				ari.valid.iface = ipr.valid.iface;
				ari.ip = ipr.destIpandMask;
				ari.valid.ip = ipr.valid.destIpandMask;
				ari.mac = ipr.gateway;
				ari.valid.mac = ipr.valid.gateway;
				ari.sa_family = ipr.sa_family;
				ari.flags = ipr.flags;
				ari.valid.flags = ipr.valid.flags;
				ari.ifnameIndex = ipr.ifnameIndex;
				ari.valid.ifnameIndex = ipr.valid.ifnameIndex;
				LOG_INFO("add arp ip %S mac %S\n", ari.ip.c_str(), ari.mac.c_str());
				arp.push_back(ari);
			} else {
				LOG_ERROR("UNKNOWN sa_family: %u\n", sa_family);
			}
		}

	} while(0);

	free(buf);

	int ipforwarding = 0;
	size = sizeof(ipforwarding);

	mib[0] = CTL_NET;
	mib[1] = PF_INET;
	mib[2] = IPPROTO_IP;
	mib[3] = IPCTL_FORWARDING;

	if( sysctl(mib, 4, &ipforwarding, &size, NULL, 0) < 0) {
		LOG_ERROR("sysctl(CTL_NET, PF_INET, IPPROTO_IP, IPCTL_FORWARDING) ... error (%s)\n", errorname(errno));
		return false;
	}
	ipv4_forwarding = bool(ipforwarding);

	mib[0] = CTL_NET;
	mib[1] = PF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_FORWARDING;

	if( sysctl(mib, 4, &ipforwarding, &size, NULL, 0) < 0) {
		LOG_ERROR("sysctl(CTL_NET, PF_INET, IPPROTO_IP, IPCTL_FORWARDING) ... error (%s)\n", errorname(errno));
		return false;
	}

	ipv6_forwarding = bool(ipforwarding);
#endif
	return true;
}

void NetRoutes::Log(void)
{
	LOG_INFO("inet.size() %u\n", inet.size());
	for( const auto & item : inet )
		item.Log();

	LOG_INFO("inet6.size() %u\n", inet6.size());
	for( const auto & item : inet6 )
		item.Log();

	LOG_INFO("arp.size() %u\n", arp.size());
	for( const auto & item : arp )
		item.Log();

	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	for( const auto & item : rule )
		item.Log();
	for( const auto & item : rule6 )
		item.Log();
	for( const auto & item : mcrule )
		item.Log();
	for( const auto & item : mcrule6 )
		item.Log();
	#endif

	LOG_INFO("ipv4_forwarding: %s\n", ipv4_forwarding ? "true":"false");
	LOG_INFO("ipv6_forwarding: %s\n", ipv6_forwarding ? "true":"false");

	return;
}

void NetRoutes::Clear(void)
{
	LOG_INFO("\n");

	ifs.clear();

	arp.clear();
	inet.clear();
	inet6.clear();
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	mcinet.clear();
	mcinet6.clear();
	rule.clear();
	rule6.clear();
	mcrule.clear();
	mcrule6.clear();
	#endif
}

bool NetRoutes::SetIpForwarding(bool on)
{
	LOG_INFO("on = %s\n", on ? "true":"false");
	if( ipv4_forwarding != on ) {
		char buffer[MAX_CMD_LEN];
		if( snprintf(buffer, MAX_CMD_LEN, SET_IP_FORWARDING, on ? 1:0) > 0 )
			ipv4_forwarding = (RootExec(buffer) == 0) ? on:ipv4_forwarding;
		return ipv4_forwarding == on;
	}
	return false;
}

bool NetRoutes::SetIp6Forwarding(bool on)
{
	LOG_INFO("on = %s\n", on ? "true":"false");
	if( ipv6_forwarding != on ) {
		char buffer[MAX_CMD_LEN];
		if( snprintf(buffer, MAX_CMD_LEN, SET_IP6_FORWARDING, on ? 1:0) > 0 )
			ipv6_forwarding = (RootExec(buffer) == 0) ? on:ipv6_forwarding;
		return ipv6_forwarding == on;
	}
	return false;
}


#if defined(__APPLE__) || defined(__FreeBSD__)
bool NetRoutes::UpdateInterfaces(void)
{
	LOG_INFO("\n");

	char ifname[MAX_INTERFACE_NAME_LEN] = {0};
	int mib[6] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
	char * buf = 0;
	size_t size = 0;

	if( sysctl(mib, 6, NULL, &size, NULL, 0) < 0) {
		LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_IFLIST2) ... error (%s)\n", errorname(errno));
		return false;
	}

	size += size/2;

	buf = (char *)malloc(size);
	if( !buf ) {
		LOG_ERROR("can`t allocate %u bytes\n", size);
		return false;
	}

	do {
		if( sysctl(mib, 6, buf, &size, NULL, 0) < 0) {
			LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_IFLIST2) ... error (%s)\n", errorname(errno));
			break;
		}

		char *lim, *next;
		lim = buf + size;

		for (next = buf; next < lim; ) {
			struct if_msghdr * ifm = (struct if_msghdr *)next;
			next += ifm->ifm_msglen;

			LOG_INFO("ifm_msglen:       %d (%d)\n", ifm->ifm_msglen, sizeof(struct rt_msghdr2));
			LOG_INFO("ifm_version:      %d\n", ifm->ifm_version);
			LOG_INFO("ifm_type:         0x%02X (%s)\n", ifm->ifm_type, rtmtoname(ifm->ifm_type));
			LOG_INFO("ifm_addrs:       \"%s\"(0x%08X)\n", GetRtmAddrs(ifm->ifm_addrs), ifm->ifm_addrs);
			LOG_INFO("ifm_flags:       \"%s\"(0x%08X) (%s)\n", RouteFlagsToString(ifm->ifm_flags, 0), ifm->ifm_flags, GetRtmFlags(ifm->ifm_flags));
			LOG_INFO("ifm_index:        %d\n", ifm->ifm_index);

			if( ifm->ifm_type != RTM_IFINFO2 || !(ifm->ifm_addrs & RTA_IFP) )
				continue;

			struct sockaddr_dl *sdl = (struct sockaddr_dl *)((struct if_msghdr2 *)ifm + 1);

			/* The interface name is not a zero-ended string */
			memcpy(ifname, sdl->sdl_data, MIN(sizeof(ifname) - 1, sdl->sdl_nlen));
			ifname[MIN(sizeof(ifname) - 1, sdl->sdl_nlen)] = 0;

			LOG_INFO("sdl_index: %d\n", sdl->sdl_index);
			LOG_INFO("sdl_type:  0x%02x (%s)\n", sdl->sdl_type, ifttoname(sdl->sdl_type));
			LOG_INFO("sdl_nlen:  %d\n", sdl->sdl_nlen);

			LOG_INFO("%d. ifname: %s\n", ifm->ifm_index, ifname);
			ifs[ifm->ifm_index] = towstr(ifname);
		}

	} while(0);

	free(buf);

	return false;
}
#endif

#ifdef MAIN_NETROUTES

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
	NetRoutes rt;

	rt.Update();
	rt.Log();

	//rt.inet.begin()->CreateIpRoute(L"192.168.2.0", L"255.255.255.0", L"192.168.2.1", L"eth0", 0);

	system("mv /tmp/far2l.netcfg.log .");
	return 0;
}
#endif //MAIN_NETROUTES
