#include "netif.h"
#include "netifset.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/netutils.h>
#include <common/netlink.h>
#include <common/sizestr.h>

#define LOG_SOURCE_FILE "netif.cpp"
#ifndef MAIN_NETIF
extern const char * LOG_FILE;
#else
#define LOG_FILE ""
#endif

extern "C" {
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)

#include <linux/sysctl.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/snmp.h>
#include <unistd.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#else // __APPLE__

#include <sys/sysctl.h>
#include <net/if_var.h>
#include <net/if_dl.h>

#endif

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 0x10000
#endif //IFF_LOWER_UP

#ifndef IFF_DORMANT
#define IFF_DORMANT 0x20000
#endif //IFF_DORMANT

#ifndef IFF_ECHO
#define IFF_ECHO 0x40000
#endif //IFF_ECHO

#if !defined(__APPLE__) && !defined(__FreeBSD__)
bool NetInterface::HasAcceptRaRtTable(void)
{
	return (bool)has_accept_ra_rt_table();
}
#endif

NetInterface::NetInterface(std::wstring name):
size(sizeof(NetInterface)),
name(name),
type(ARPHRD_ETHER),
ifa_flags(0),
mtu(0),
recv_bytes(0ll),
send_bytes(0ll),
recv_packets(0ll),
send_packets(0ll),
recv_errors(0ll),
send_errors(0ll),
multicast(0ll),
collisions(0ll),
tcpdumpOn(false)
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	memset(&osdep.inet6stat, 0, sizeof(osdep.inet6stat));
	memset(&osdep.icmp6stat, 0, sizeof(osdep.icmp6stat));
	memset(&osdep.inet6cacheinfo, 0, sizeof(osdep.inet6cacheinfo));
	memset(&osdep.stat64, 0, sizeof(osdep.stat64));
	memset(&osdep.ifmap, 0, sizeof(osdep.ifmap));
	memset(&osdep.ipv4conf, 0, sizeof(osdep.ipv4conf));
	memset(&osdep.ipv6conf, 0, sizeof(osdep.ipv6conf));
	memset(&osdep.valid, 0, sizeof(osdep.valid));

	osdep.gso_max_segs = 0;
	osdep.gso_max_size = 0;
	osdep.gso_ipv4_max_size = 0;
	osdep.gro_max_size = 0;
	osdep.gro_ipv4_max_size = 0;
	osdep.tso_max_segs = 0;
	osdep.tso_max_size = 0;
	osdep.txqueuelen = 0;
	osdep.numtxqueues = 0;
	osdep.numrxqueues = 0;

	osdep.minmtu = 0;
	osdep.maxmtu = 0;
	osdep.group = 0;

#endif
}

NetInterface::~NetInterface()
{
}

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

void NetInterface::LogStats(void)
{

	LOG_INFO("recv_bytes:   %s\n", size_to_str(recv_bytes));
	LOG_INFO("send_bytes:   %s\n", size_to_str(send_bytes));
	LOG_INFO("recv_packets: " LLFMT "\n", recv_packets);
	LOG_INFO("send_packets: " LLFMT "\n", send_packets);
	LOG_INFO("recv_errors:  " LLFMT "\n", recv_errors);
	LOG_INFO("send_errors:  " LLFMT "\n", send_errors);
	LOG_INFO("multicast:    " LLFMT "\n", multicast);
	LOG_INFO("collisions:   " LLFMT "\n",collisions);

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	uint64_t * st = (uint64_t *)&osdep.stat64;
	const uint64_t cnt = sizeof(osdep.stat64)/sizeof(uint64_t);
	static const char * field[] = {
		"rx_packets",
		"tx_packets", 
		"rx_bytes",
		"tx_bytes",
		"rx_errors",
		"tx_errors",
		"rx_dropped",
		"tx_dropped",
		"multicast",
		"collisions",
		"rx_length_errors",
		"rx_over_errors",
		"rx_crc_errors",
		"rx_frame_errors",
		"rx_fifo_errors",
		"rx_missed_errors",
		"tx_aborted_errors",
		"tx_carrier_errors",
		"tx_fifo_errors",
		"tx_heartbeat_errors",
		"tx_window_errors",
		"rx_compressed",
		"tx_compressed",
		"rx_nohandler",
		"rx_otherhost_dropped"
	};

	for( uint32_t index = 0; index < cnt; index++ )
	{
		switch( index ) {
			case 2:
			case 3:
			LOG_INFO("%-18s\t%s\n", field[index], size_to_str(st[index]));
			break;
		default:
			LOG_INFO("%-18s\t" LLFMT "\n", field[index], st[index]);
			break;
		};
	}

	LogInet6Stats();
	LogIcmp6Stats();
#else
	// TODO: macos
#endif

}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
void NetInterface::LogIpv6Conf(void)
{
	int32_t * st = (int32_t *)&osdep.ipv6conf;
	const uint32_t cnt = osdep.ipv6conf.totalFields;
	static const char * field[] = {
		"totalFields",
		"forwarding",                       // DEVCONF_FORWARDING = 0,
		"hop_limit",                        // DEVCONF_HOPLIMIT,
		"mtu",                              // DEVCONF_MTU6,
		"accept_ra",                        // DEVCONF_ACCEPT_RA,
		"accept_redirects",                 // DEVCONF_ACCEPT_REDIRECTS,
		"autoconf",                         // DEVCONF_AUTOCONF,
		"dad_transmits",                    // DEVCONF_DAD_TRANSMITS,
		"router_solicitations",             // DEVCONF_RTR_SOLICITS,
		"router_solicitation_interval",     // DEVCONF_RTR_SOLICIT_INTERVAL,
		"router_solicitation_delay",        // DEVCONF_RTR_SOLICIT_DELAY,
		"use_tempaddr",                     // DEVCONF_USE_TEMPADDR,
		"temp_valid_lft",                   // DEVCONF_TEMP_VALID_LFT,
		"temp_prefered_lft",                // DEVCONF_TEMP_PREFERED_LFT,
		"regen_max_retry",                  // DEVCONF_REGEN_MAX_RETRY,
		"max_desync_factor",                // DEVCONF_MAX_DESYNC_FACTOR,
		"max_addresses",                    // DEVCONF_MAX_ADDRESSES,
		"force_mld_version",                // DEVCONF_FORCE_MLD_VERSION,
		"accept_ra_defrtr",                 // DEVCONF_ACCEPT_RA_DEFRTR,
		"accept_ra_pinfo",                  // DEVCONF_ACCEPT_RA_PINFO,
		"accept_ra_rtr_pref",               // DEVCONF_ACCEPT_RA_RTR_PREF,
		"router_probe_interval",            // DEVCONF_RTR_PROBE_INTERVAL,
		"accept_ra_rt_info_max_plen",       // DEVCONF_ACCEPT_RA_RT_INFO_MAX_PLEN,
		"proxy_ndp",                        // DEVCONF_PROXY_NDP,
		"optimistic_dad",                   // DEVCONF_OPTIMISTIC_DAD,
		"accept_source_route",              // DEVCONF_ACCEPT_SOURCE_ROUTE,
		"mc_forwarding",                    // DEVCONF_MC_FORWARDING,
		"disable_ipv6",                     // DEVCONF_DISABLE_IPV6,
		"accept_dad",                       // DEVCONF_ACCEPT_DAD,
		"force_tllao",                      // DEVCONF_FORCE_TLLAO,
		"ndisc_notify",                     // DEVCONF_NDISC_NOTIFY,
		"mldv1_unsolicited_report_interval",// DEVCONF_MLDV1_UNSOLICITED_REPORT_INTERVAL,
		"mldv2_unsolicited_report_interval",// DEVCONF_MLDV2_UNSOLICITED_REPORT_INTERVAL,
		"suppress_frag_ndisc",              // DEVCONF_SUPPRESS_FRAG_NDISC,
		"accept_ra_from_local",             // DEVCONF_ACCEPT_RA_FROM_LOCAL,
		"use_optimistic",                   // DEVCONF_USE_OPTIMISTIC,
		"accept_ra_mtu",                    // DEVCONF_ACCEPT_RA_MTU,
		"stable_secret",                    // DEVCONF_STABLE_SECRET,
		"use_oif_addrs_only",               // DEVCONF_USE_OIF_ADDRS_ONLY,
		"accept_ra_min_hop_limit",          // DEVCONF_ACCEPT_RA_MIN_HOP_LIMIT,
		"ignore_routes_with_linkdown",      // DEVCONF_IGNORE_ROUTES_WITH_LINKDOWN,
		"drop_unicast_in_l2_multicast",     // DEVCONF_DROP_UNICAST_IN_L2_MULTICAST,
		"drop_unsolicited_na",              // DEVCONF_DROP_UNSOLICITED_NA,
		"keep_addr_on_down",                // DEVCONF_KEEP_ADDR_ON_DOWN,
		"router_solicitation_max_interval", // DEVCONF_RTR_SOLICIT_MAX_INTERVAL,
		"seg6_enabled",                     // DEVCONF_SEG6_ENABLED,
		"seg6_require_hmac",                // DEVCONF_SEG6_REQUIRE_HMAC,
		"enhanced_dad",                     // DEVCONF_ENHANCED_DAD,
		"addr_gen_mode",                    // DEVCONF_ADDR_GEN_MODE,
		"disable_policy",                   // DEVCONF_DISABLE_POLICY,
		"accept_ra_rt_info_min_plen",       // DEVCONF_ACCEPT_RA_RT_INFO_MIN_PLEN,
		"ndisc_tclass",                     // DEVCONF_NDISC_TCLASS,
		"rpl_seg_enabled",                  // DEVCONF_RPL_SEG_ENABLED,
		"ra_defrtr_metric",                 // DEVCONF_RA_DEFRTR_METRIC,
		"ioam6_enabled",                    // DEVCONF_IOAM6_ENABLED,
		"ioam6_id",                         // DEVCONF_IOAM6_ID,
		"ioam6_id_wide",                    // DEVCONF_IOAM6_ID_WIDE,
		"ndisc_evict_nocarrier",            // DEVCONF_NDISC_EVICT_NOCARRIER,
		"accept_untracked_na"               // DEVCONF_ACCEPT_UNTRACKED_NA
	};
	assert( sizeof(osdep.ipv6conf) >= cnt*sizeof(int32_t) );

	for( uint32_t index = 0; index < cnt; index++ )
	{
		switch( index ) {
			case DEVCONF_RTR_SOLICIT_INTERVAL+1:
			case DEVCONF_RTR_SOLICIT_DELAY+1:
			case DEVCONF_RTR_SOLICIT_MAX_INTERVAL+1:
			case DEVCONF_TEMP_VALID_LFT+1:
			case DEVCONF_TEMP_PREFERED_LFT+1:
			case DEVCONF_RTR_PROBE_INTERVAL+1:
			case DEVCONF_MLDV1_UNSOLICITED_REPORT_INTERVAL+1:
			case DEVCONF_MLDV2_UNSOLICITED_REPORT_INTERVAL+1:
			case DEVCONF_MAX_DESYNC_FACTOR+1:

			if( (int32_t)st[index] < 0)
				LOG_INFO("%-35s %d\n", field[index], st[index]);
			else
				LOG_INFO("%-35s %s (%d ms)\n", field[index], msec_to_str((uint64_t)st[index]), st[index]);
			break;
		default:
			LOG_INFO("%-35s %d\n", field[index], st[index]);
			break;
		};
	}

	if( has_accept_ra_rt_table() )
		LOG_INFO("%-35s %d\n", "accept_ra_rt_table", osdep.accept_ra_rt_table);

	if( osdep.inet6cacheinfo.tstamp ) {
		LOG_INFO("inet6cacheinfo:\n");
		LOG_INFO("    max_reasm_len:  %s\n", size_to_str(osdep.inet6cacheinfo.max_reasm_len) );
		LOG_INFO("    tstamp:         %.2fs\n", (double)osdep.inet6cacheinfo.tstamp/100.0);
		LOG_INFO("    reachable_time: %s\n", msec_to_str(osdep.inet6cacheinfo.reachable_time));
		LOG_INFO("    retrans_time:   %s\n", msec_to_str(osdep.inet6cacheinfo.retrans_time));
	}
}

void NetInterface::LogIpv4Conf(void)
{
	int32_t * st = (int32_t *)&osdep.ipv4conf;
	const uint32_t cnt = osdep.ipv4conf.totalFields;
	static const char * field[] = {
		"totalFields",
		"forwarding",                         // IPV4_DEVCONF_FORWARDING:
		"mc_forwarding",                      // IPV4_DEVCONF_MC_FORWARDING:
		"proxy_arp",                          // IPV4_DEVCONF_PROXY_ARP:
		"accept_redirects",                   // IPV4_DEVCONF_ACCEPT_REDIRECTS:
		"secure_redirects",                   // IPV4_DEVCONF_SECURE_REDIRECTS:
		"send_redirects",                     // IPV4_DEVCONF_SEND_REDIRECTS:
		"shared_media",                       // IPV4_DEVCONF_SHARED_MEDIA:
		"rp_filter",                          // IPV4_DEVCONF_RP_FILTER:
		"accept_source_route",                // IPV4_DEVCONF_ACCEPT_SOURCE_ROUTE:
		"bootp_relay",                        // IPV4_DEVCONF_BOOTP_RELAY:
		"log_martians",                       // IPV4_DEVCONF_LOG_MARTIANS:
		"tag",                                // IPV4_DEVCONF_TAG:
		"arp_filter",                         // IPV4_DEVCONF_ARPFILTER:
		"medium_id",                          // IPV4_DEVCONF_MEDIUM_ID:
		"disable_xfrm",                       // IPV4_DEVCONF_NOXFRM:
		"disable_policy",                     // IPV4_DEVCONF_NOPOLICY:
		"force_igmp_version",                 // IPV4_DEVCONF_FORCE_IGMP_VERSION:
		"arp_announce",                       // IPV4_DEVCONF_ARP_ANNOUNCE:
		"arp_ignore",                         // IPV4_DEVCONF_ARP_IGNORE:
		"promote_secondaries",                // IPV4_DEVCONF_PROMOTE_SECONDARIES:
		"arp_accept",                         // IPV4_DEVCONF_ARP_ACCEPT:
		"arp_notify",                         // IPV4_DEVCONF_ARP_NOTIFY:
		"accept_local",                       // IPV4_DEVCONF_ACCEPT_LOCAL:
		"src_valid_mark",                     // IPV4_DEVCONF_SRC_VMARK:
		"proxy_arp_pvlan",                    // IPV4_DEVCONF_PROXY_ARP_PVLAN:
		"route_localnet",                     // IPV4_DEVCONF_ROUTE_LOCALNET:
		"igmpv2_unsolicited_report_interval", // IPV4_DEVCONF_IGMPV2_UNSOLICITED_REPORT_INTERVAL:
		"igmpv3_unsolicited_report_interval", // IPV4_DEVCONF_IGMPV3_UNSOLICITED_REPORT_INTERVAL:
		"ignore_routes_with_linkdown",        // IPV4_DEVCONF_IGNORE_ROUTES_WITH_LINKDOWN:
		"drop_unicast_in_l2_multicast",       // IPV4_DEVCONF_DROP_UNICAST_IN_L2_MULTICAST:
		"drop_gratuitous_arp",                // IPV4_DEVCONF_DROP_GRATUITOUS_ARP:
		"bc_forwarding",                      // IPV4_DEVCONF_BC_FORWARDING:
		"arp_evict_nocarrier"                 // IPV4_DEVCONF_ARP_EVICT_NOCARRIER:
	};

	assert( sizeof(osdep.ipv4conf) >= cnt*sizeof(int32_t) );

	for( uint32_t index = 0; index < cnt; index++ )
	{
		switch( index ) {
			case IPV4_DEVCONF_IGMPV2_UNSOLICITED_REPORT_INTERVAL:
			case IPV4_DEVCONF_IGMPV3_UNSOLICITED_REPORT_INTERVAL:
			LOG_INFO("%-35s %s (%d ms)\n", field[index], msec_to_str((uint64_t)st[index]), st[index]);
			break;
		default:
			LOG_INFO("%-35s %d\n", field[index], st[index]);
			break;
		};
	}
}

void NetInterface::LogHardInfo(void)
{
	if( osdep.ifmap.mem_start || osdep.ifmap.mem_end || osdep.ifmap.base_addr || osdep.ifmap.port ) {
		LOG_INFO("mem_start: 0x%p\n", osdep.ifmap.mem_start);
		LOG_INFO("mem_end:   0x%p\n", osdep.ifmap.mem_end);
		LOG_INFO("base_addr: 0x%p\n", osdep.ifmap.base_addr);
		LOG_INFO("irq:       %d\n", osdep.ifmap.irq);
		LOG_INFO("dma:       %d\n", osdep.ifmap.dma);
		LOG_INFO("port:      %d\n", osdep.ifmap.port);
	}
	if( !osdep.parentdev_name.empty() || !osdep.parentdev_busname.empty() ) {
		LOG_INFO("parent dev name:         %S\n", osdep.parentdev_name.c_str());
		LOG_INFO("parent dev bus name:     %S\n", osdep.parentdev_busname.c_str());
	}

	LOG_INFO("gso_max_segs:      %d\n", osdep.gso_max_segs);
	LOG_INFO("gso_max_size:      %d\n", osdep.gso_max_size);
	LOG_INFO("gso_ipv4_max_size: %d\n", osdep.gso_ipv4_max_size);
	LOG_INFO("gro_max_size:      %d\n", osdep.gro_max_size);
	LOG_INFO("gro_ipv4_max_size: %d\n", osdep.gro_ipv4_max_size);
	LOG_INFO("tso_max_segs:      %d\n", osdep.tso_max_segs);
	LOG_INFO("tso_max_size:      %d\n", osdep.tso_max_size);
	LOG_INFO("txqueuelen:        %d\n", osdep.txqueuelen);
	LOG_INFO("numtxqueues:       %d\n", osdep.numtxqueues);
	LOG_INFO("numrxqueues:       %d\n", osdep.numrxqueues);
}

void NetInterface::LogIcmp6Stats(void)
{
	uint64_t * st = (uint64_t *)&osdep.icmp6stat;
	uint64_t cnt = osdep.icmp6stat.totalStatsFields;
	static const char * field[] = {
		"totalStatsFields", //         ICMP6_MIB_NUM = 0,
		"Icmp6InMsgs", //	ICMP6_MIB_INMSGS,			/* InMsgs */
		"Icmp6InErrors", //	ICMP6_MIB_INERRORS,			/* InErrors */
		"Icmp6OutMsgs", //	ICMP6_MIB_OUTMSGS,			/* OutMsgs */
		"Icmp6OutErrors", //	ICMP6_MIB_OUTERRORS,			/* OutErrors */
		"Icmp6InCsumErrors", //	ICMP6_MIB_CSUMERRORS,			/* InCsumErrors */
		"Icmp6OutRateLimitHost", //	ICMP6_MIB_RATELIMITHOST,		/* OutRateLimitHost */
	};

	assert( sizeof(osdep.icmp6stat) >= cnt*sizeof(uint64_t) );

	for( uint32_t index = 0; index < cnt; index++ )
	{
		LOG_INFO("%-18s\t" LLFMT "\n", field[index], st[index]);
	}
}

void NetInterface::LogInet6Stats(void)
{
	uint64_t * st = (uint64_t *)&osdep.inet6stat;
	uint64_t cnt = osdep.inet6stat.totalStatsFields;

	static const char * field[] = {
		"totalStatsFields", //         IPSTATS_MIB_NUM = 0,
		"Ip6InReceives", //            IPSTATS_MIB_INPKTS,			
		"Ip6InOctets", //              IPSTATS_MIB_INOCTETS,			
		"Ip6InDelivers", //            IPSTATS_MIB_INDELIVERS,			
		"Ip6OutForwDatagrams", //      IPSTATS_MIB_OUTFORWDATAGRAMS,		
		"Ip6OutRequests", //           IPSTATS_MIB_OUTPKTS,			
		"Ip6OutOctets", //             IPSTATS_MIB_OUTOCTETS,			
		"Ip6InHdrErrors", //           IPSTATS_MIB_INHDRERRORS,		
		"Ip6InTooBigErrors", //        IPSTATS_MIB_INTOOBIGERRORS,		
		"Ip6InNoRoutes", //            IPSTATS_MIB_INNOROUTES,			
		"Ip6InAddrErrors", //          IPSTATS_MIB_INADDRERRORS,		
		"Ip6InUnknownProtos", //       IPSTATS_MIB_INUNKNOWNPROTOS,		
		"Ip6InTruncatedPkts", //       IPSTATS_MIB_INTRUNCATEDPKTS,		
		"Ip6InDiscards", //            IPSTATS_MIB_INDISCARDS,			
		"Ip6OutDiscards", //           IPSTATS_MIB_OUTDISCARDS,		
		"Ip6OutNoRoutes", //           IPSTATS_MIB_OUTNOROUTES,		
		"Ip6ReasmTimeout", //          IPSTATS_MIB_REASMTIMEOUT,		
		"Ip6ReasmReqds", //            IPSTATS_MIB_REASMREQDS,			
		"Ip6ReasmOKs", //              IPSTATS_MIB_REASMOKS,			
		"Ip6ReasmFails", //            IPSTATS_MIB_REASMFAILS,			
		"Ip6FragOKs", //               IPSTATS_MIB_FRAGOKS,			
		"Ip6FragFails", //             IPSTATS_MIB_FRAGFAILS,			
		"Ip6FragCreates", //           IPSTATS_MIB_FRAGCREATES,		
		"Ip6InMcastPkts", //           IPSTATS_MIB_INMCASTPKTS,		
		"Ip6OutMcastPkts", //          IPSTATS_MIB_OUTMCASTPKTS,		
		"Ip6InBcastPkts", //           IPSTATS_MIB_INBCASTPKTS,		
		"Ip6OutBcastPkts", //          IPSTATS_MIB_OUTBCASTPKTS,		
		"Ip6InMcastOctets", //         IPSTATS_MIB_INMCASTOCTETS,		
		"Ip6OutMcastOctets", //        IPSTATS_MIB_OUTMCASTOCTETS,		
		"Ip6InBcastOctets", //         IPSTATS_MIB_INBCASTOCTETS,		
		"Ip6OutBcastOctets", //        IPSTATS_MIB_OUTBCASTOCTETS,		
		"Ip6InCsumErrors", //          IPSTATS_MIB_CSUMERRORS,			
		"Ip6InNoECTPkts", //           IPSTATS_MIB_NOECTPKTS,			
		"Ip6InECT1Pkts", //            IPSTATS_MIB_ECT1PKTS,			
		"Ip6InECT0Pkts", //            IPSTATS_MIB_ECT0PKTS,			
		"Ip6InCEPkts", //              IPSTATS_MIB_CEPKTS,			
		"Ip6ReasmOverlaps", //         IPSTATS_MIB_REASM_OVERLAPS,		
	};

	assert( sizeof(osdep.inet6stat) >= cnt*sizeof(uint64_t) );

	for( uint32_t index = 0; index < cnt; index++ )
	{
		switch( index ) {
		case IPSTATS_MIB_INOCTETS:
		case IPSTATS_MIB_OUTOCTETS:
		case IPSTATS_MIB_INMCASTOCTETS:	
		case IPSTATS_MIB_OUTMCASTOCTETS:
		case IPSTATS_MIB_INBCASTOCTETS:	
		case IPSTATS_MIB_OUTBCASTOCTETS:
			LOG_INFO("%-18s\t%s\n", field[index], size_to_str(st[index]));
			break;
		default:
			LOG_INFO("%-18s\t" LLFMT "\n", field[index], st[index]);
			break;
		};
	}
}
#endif

void NetInterface::Log(void)
{
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( !osdep.alias.empty() )
		LOG_INFO("%S (ifindex = %u), alias: %S type: %s\n", name.c_str(), ifindex, osdep.alias.c_str(), arphdrname(type));
	else 
		LOG_INFO("%S (ifindex = %u), type: %s\n", name.c_str(), ifindex, arphdrname(type));
	#else
	LOG_INFO("%S (ifindex = %u):\n", name.c_str(), ifindex);
	#endif


	for( const auto& [ip, ipinfo] : ip ) {
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
		LOG_INFO("   flags=0x%08X(%s) ip: %S/%u netmask: %S %s %S\n", ipinfo.flags, ifaddrflags(ipinfo.flags), ip.c_str(), ipinfo.prefixlen, ipinfo.netmask.c_str(), \
			ifa_flags & IFF_BROADCAST ? "broadcast:":ifa_flags & IFF_POINTOPOINT ? "point-to-point:":"", \
			ifa_flags & IFF_BROADCAST ? ipinfo.broadcast.c_str():ifa_flags & IFF_POINTOPOINT ? ipinfo.point_to_point.c_str():L"");
	#else
		LOG_INFO("   flags=0x%08X ip: %S/%u netmask: %S %s %S\n", ipinfo.flags, ip.c_str(), ipinfo.prefixlen, ipinfo.netmask.c_str(), \
			ifa_flags & IFF_BROADCAST ? "broadcast:":ifa_flags & IFF_POINTOPOINT ? "point-to-point:":"", \
			ifa_flags & IFF_BROADCAST ? ipinfo.broadcast.c_str():ifa_flags & IFF_POINTOPOINT ? ipinfo.point_to_point.c_str():L"");
	#endif
	}
	for( const auto& [ip, ipinfo] : ip6 ) {
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
		LOG_INFO("   flags=0x%08X(%s) ip6: %S/%u netmask: %S %s %S\n", ipinfo.flags, ifaddrflags(ipinfo.flags), ip.c_str(), ipinfo.prefixlen, ipinfo.netmask.c_str(), \
			ifa_flags & IFF_BROADCAST ? "broadcast:":ifa_flags & IFF_POINTOPOINT ? "point-to-point:":"", \
			ifa_flags & IFF_BROADCAST ? ipinfo.broadcast.c_str():ifa_flags & IFF_POINTOPOINT ? ipinfo.point_to_point.c_str():L"");
	#else
		LOG_INFO("   flags=0x%08X ip6: %S/%u netmask: %S %s %S\n", ipinfo.flags, ip.c_str(), ipinfo.prefixlen, ipinfo.netmask.c_str(), \
			ifa_flags & IFF_BROADCAST ? "broadcast:":ifa_flags & IFF_POINTOPOINT ? "point-to-point:":"", \
			ifa_flags & IFF_BROADCAST ? ipinfo.broadcast.c_str():ifa_flags & IFF_POINTOPOINT ? ipinfo.point_to_point.c_str():L"");
	#endif
	}
	for( const auto& [mac, macinfo] : mac ) {
		LOG_INFO("   flags=0x%08X (%s) mac: %S %s %S\n", macinfo.flags, ifflagsname(macinfo.flags), mac.c_str(), \
			ifa_flags & IFF_BROADCAST ? "broadcast:":"", \
			ifa_flags & IFF_BROADCAST ? macinfo.broadcast.c_str():L"");
	}

	LOG_INFO("   permanent mac: %S\n", permanent_mac.c_str());
	LOG_INFO("   mtu:    %u\n", mtu);

	LogStats();
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	LOG_INFO("   minmtu: %d\n", osdep.minmtu);
	LOG_INFO("   maxmtu: %d\n", osdep.maxmtu);
	LOG_INFO("   group:  %d\n", osdep.group);
	LOG_INFO("   qdisc:  %S\n", osdep.qdisc.c_str());
	LogIpv4Conf();
	LogIpv6Conf();
	LogHardInfo();
	#endif
}

bool NetInterface::UpdateStats(void)
{
	std::string iface(name.begin(), name.end());
	bool res = false;

	#if !defined(__APPLE__) && !defined(__FreeBSD__)

	FILE * netdev = fopen("/proc/net/dev", "r");
	if( !netdev ) {
		LOG_ERROR("can`t open \"/proc/net/dev\" ... error (%s)\n", errorname(errno));
		return false;
	}

	uint64_t unused;
	char line[512];
	char ifname[MAX_INTERFACE_NAME_LEN + 1];
	if( fgets(line, sizeof(line), netdev) != 0 ) {
		while( fgets(line, sizeof(line), netdev) != 0 && \
			fscanf(netdev, "%s", ifname) > 0 ) {
			if( strlen(ifname) == (iface.size() + 1) && \
			    strncmp(iface.c_str(), ifname, iface.size()) == 0 ) {
				res = (fscanf(netdev, 
#if INTPTR_MAX == INT32_MAX
						"%llu %llu %llu %llu %llu %llu %llu %llu "
						"%llu %llu %llu %llu %llu %llu %llu %llu\n",
#elif INTPTR_MAX == INT64_MAX
						"%lu %lu %lu %lu %lu %lu %lu %lu "
						"%lu %lu %lu %lu %lu %lu %lu %lu\n",
#else
    #error "Environment not 32 or 64-bit."
#endif
						   &recv_bytes, &recv_packets,
						   &recv_errors, &unused, &unused,
						   &unused, &unused, &multicast,
						   &send_bytes, &send_packets,
						   &send_errors, &unused,
						   &unused, &collisions,
						   &unused,
						   &unused) == 16);
				break;
			}
		}
	}

	fclose(netdev);

	if( !res )
		LOG_ERROR("stats for %S not found\n", iface.c_str());
	else
		LOG_INFO("%16s: stats update ... success\n", iface.c_str());

	// add mtu mac permanent etc

	int isocket = socket(AF_INET, SOCK_DGRAM, 0);
	if( isocket < 0 ) {
		LOG_ERROR("socket(AF_INET, SOCK_STREAM, 0) ... error (%s)\n", errorname(errno));
		return false;
	}

	struct ifreq paifr;
	memset(&paifr, 0, sizeof(paifr));
	strncpy(paifr.ifr_name, iface.c_str(), iface.size());
	if( ioctl(isocket, SIOCGIFMTU, &paifr) != -1 )
		mtu = paifr.ifr_mtu;
	else
		LOG_ERROR("ioctl(SIOCGIFMTU) ... error (%s)\n", errorname(errno));

	struct ethtool_perm_addr * epa = (struct ethtool_perm_addr*)malloc(sizeof(struct ethtool_perm_addr) + IFHWADDRLEN);
	if( epa ) {
		epa->cmd = ETHTOOL_GPERMADDR;
		epa->size = IFHWADDRLEN;
		paifr.ifr_data = (caddr_t)epa;
		if( ioctl(isocket, SIOCETHTOOL, &paifr) >= 0 ) {
			char s[INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN] = {0};
			if( snprintf(s, sizeof(s), "%02x:%02x:%02x:%02x:%02x:%02x", \
					epa->data[0], epa->data[1], epa->data[2], \
					epa->data[3],epa->data[4],epa->data[5]) > 0 ) {
				std::string _name(s);
				permanent_mac = std::wstring(_name.begin(), _name.end());
			}
		} else
			LOG_ERROR("ioctl(SIOCETHTOOL) ... error (%s)\n", errorname(errno));

		free(epa);
	}

	close(isocket);

	#else

	int mib[6] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
	char * buf = 0;
	size_t size = 0;

	if( sysctl(mib, sizeof(mib)/sizeof(mib[0]), 0, &size, 0, 0) < 0 ) {
		LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_IFLIST2) ... error (%s)\n", errorname(errno));
		return false;
	}

	buf = (char *)malloc(size);
	if( !buf ) {
		LOG_ERROR("can`t allocate %u bytes\n", size);
		return false;
	}
	do {
		if( sysctl(mib, sizeof(mib)/sizeof(mib[0]), buf, &size, 0, 0) < 0 ) {
			LOG_ERROR("sysctl(CTL_NET, PF_ROUTE, NET_RT_IFLIST2) ... error (%s)\n", errorname(errno));
			break;
		}
		char *lim, *next;
		lim = buf + size;
		for (next = buf; next < lim; ) {
			struct if_msghdr * ifm = (struct if_msghdr *)next;
			next += ifm->ifm_msglen;
			if( ifm->ifm_type == RTM_IFINFO2 ) {
				struct if_msghdr2 *if2m = (struct if_msghdr2 *)ifm;
				struct sockaddr_dl *sdl = (struct sockaddr_dl *)(if2m + 1);
				if( sdl->sdl_nlen == iface.size() && strncmp(iface.c_str(), sdl->sdl_data, iface.size()) == 0 ) {
					LOG_INFO("%16s: interface ... found\n", iface.c_str());
					ifa_flags = if2m->ifm_flags;
					send_packets = if2m->ifm_data.ifi_opackets;
					recv_packets = if2m->ifm_data.ifi_ipackets;
					send_bytes = if2m->ifm_data.ifi_obytes;
					recv_bytes = if2m->ifm_data.ifi_ibytes;
					send_errors =if2m->ifm_data.ifi_oerrors;
					recv_errors = if2m->ifm_data.ifi_ierrors;
					collisions = if2m->ifm_data.ifi_collisions;
					multicast = if2m->ifm_data.ifi_imcasts + if2m->ifm_data.ifi_omcasts;
					mtu = if2m->ifm_data.ifi_mtu;
				}

			}
		}

	} while(0);

	free(buf);
	
	#endif

	return res;
}

bool NetInterface::IsUp() { return ifa_flags & IFF_UP; };
bool NetInterface::IsBroadcast() { return ifa_flags & IFF_BROADCAST; };
bool NetInterface::IsDebug() { return ifa_flags & IFF_DEBUG; };
//bool NetInterface::SetDebug(bool on) { return ifa_flags & IFF_DEBUG; };
bool NetInterface::IsLoopback() { return ifa_flags & IFF_LOOPBACK; };
bool NetInterface::IsPointToPoint() { return ifa_flags & IFF_POINTOPOINT; };
bool NetInterface::IsNotrailers() { return ifa_flags & IFF_NOTRAILERS; };
//bool NetInterface::SetNotrailers() { return ifa_flags & IFF_NOTRAILERS; };
bool NetInterface::IsRunning() { return ifa_flags & IFF_RUNNING; };
bool NetInterface::IsNoARP() { return ifa_flags & IFF_NOARP; };
bool NetInterface::IsPromisc() { return ifa_flags & IFF_PROMISC; };
bool NetInterface::IsMulticast() { return ifa_flags & IFF_MULTICAST; };
bool NetInterface::IsAllmulticast() { return ifa_flags & IFF_ALLMULTI; };

#if !defined(__APPLE__) && !defined(__FreeBSD__)	
bool NetInterface::IsMaster() { return ifa_flags & IFF_MASTER; };
bool NetInterface::IsSlave() { return ifa_flags & IFF_SLAVE; };
#endif

bool NetInterface::IsCarrierOn()
{
	#if !defined(__APPLE__) && !defined(__FreeBSD__)	
	return (ifa_flags & IFF_RUNNING) && (ifa_flags & IFF_LOWER_UP);
	#else
	return (ifa_flags & IFF_RUNNING);
	#endif
}
bool NetInterface::IsDormant() { return ifa_flags & IFF_DORMANT; };
bool NetInterface::IsEcho() { return ifa_flags & IFF_ECHO; };
//bool NetInterface::SetEcho(bool on) { return ifa_flags & IFF_ECHO; };

extern int RootExec(const char * cmd);

bool NetInterface::SetInterfaceName(const wchar_t * newname)
{
	if( name == newname )
		return false;

	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEIFNAME_CMD, name.c_str(), name.c_str(), newname, newname) > 0 )
		if( RootExec(buffer) == 0 ) {
			name = newname; // we need update ifname for next commands
			return true;
		}
	return false;
}

bool NetInterface::SetMac(const wchar_t * newmac)
{
	if( mac.find(newmac) != mac.end() )
		return false;
	char buffer[MAX_CMD_LEN];

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEMAC_CMD, name.c_str(), name.c_str(), newmac, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEMAC_CMD, name.c_str(), newmac, name.c_str(), name.c_str()) > 0 )
#endif
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetUp(bool on)
{
	if( (IsUp() && on) || (!IsUp() && !on) )
		return false;

	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, on ? SETUP_CMD:SETDOWN_CMD, name.c_str()) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetMulticast(bool on)
{
	if( (IsMulticast() && on) || (!IsMulticast() && !on) )
		return false;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, on ? SETMULTICASTON_CMD:SETMULTICASTOFF_CMD, name.c_str()) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetAllmulticast(bool on)
{
	if( (IsAllmulticast() && on) || (!IsAllmulticast() && !on) )
		return false;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, on ? SETALLMULTICASTON_CMD:SETALLMULTICASTOFF_CMD, name.c_str()) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetNoARP(bool on)
{
	if( (IsNoARP() && on) || (!IsNoARP() && !on) )
		return false;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, on ? SETARPON_CMD:SETARPOFF_CMD, name.c_str()) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetPromisc(bool on)
{
	if( (IsPromisc() && on) || (!IsPromisc() && !on) )
		return false;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, on ? SETPROMISCON_CMD:SETPROMISCOFF_CMD, name.c_str()) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::SetMtu(uint32_t newmtu)
{
	if( mtu == newmtu )
		return false;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, SETMTU_CMD, name.c_str(), newmtu) > 0 )
		if( RootExec(buffer) == 0 )
			return true;
	return false;
}

bool NetInterface::DeleteIp(const wchar_t * delip)
{
	auto it = ip.find(delip);
	if( it == ip.end() )
		return false;

	std::string _mask(it->second.netmask.begin(), it->second.netmask.end());
	int maskbits = IpMaskToBits(_mask.c_str());

	char buffer[MAX_CMD_LEN];
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, DELETEIP_CMD, delip, maskbits, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, DELETEIP_CMD, name.c_str(), delip, maskbits) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			ip.erase(it);
			return true;
		}
	return false;
}

bool NetInterface::DeleteIp6(const wchar_t * delip)
{
	auto it = ip6.find(delip);
	if( it == ip6.end() )
		return false;
	char buffer[MAX_CMD_LEN];

	std::string _mask(it->second.netmask.begin(), it->second.netmask.end());
	int maskbits = Ip6MaskToBits(_mask.c_str());

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, DELETEIP6_CMD, delip, maskbits, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, DELETEIP6_CMD, name.c_str(), delip, maskbits) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			ip6.erase(it);
			return true;
		}
	return false;
}

bool NetInterface::AddIp(const wchar_t * newip, const wchar_t * mask, const wchar_t * bcip)
{
	if( ip.find(newip) != ip.end() )
		return false;

	if( *bcip == 0 || *bcip == 0x20 )
		bcip = L"255.255.255.255";

	if( *mask == 0 || *mask == 0x20 )
		mask = L"255.255.255.0";

	std::string _mask(mask, mask+wcslen(mask));
	int maskbits = IpMaskToBits(_mask.c_str());

	char buffer[MAX_CMD_LEN];
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, ADDIP_CMD, newip, maskbits, bcip, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, ADDIP_CMD, name.c_str(), newip, maskbits, bcip) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			// we need update ip and mask for next commands
			IpAddressInfo _ip;
			_ip.flags = IFF_BROADCAST;
			_ip.ip = newip;
			_ip.netmask = mask;
			ip[newip] = _ip;
			return true;
		}
	return false;
}

bool NetInterface::AddIp6(const wchar_t * newip, const wchar_t * mask)
{
	if( ip6.find(newip) != ip6.end() )
		return false;
	char buffer[MAX_CMD_LEN];

	std::string _mask(mask, mask+wcslen(mask));
	int maskbits = Ip6MaskToBits(_mask.c_str());

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, ADDIP6_CMD, newip, maskbits, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, ADDIP6_CMD, name.c_str(), newip, maskbits) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			// we need update ip and mask for next commands
			IpAddressInfo _ip;
			_ip.flags = 0;
			_ip.ip = newip;
			_ip.netmask = mask;
			ip6[newip] = _ip;
			return true;
		}
	return false;
}


bool NetInterface::ChangeIp(const wchar_t * oldip, const wchar_t * newip, const wchar_t * mask, const wchar_t * bcip)
{
	auto it = ip.find(oldip);
	if( it == ip.end() )
		return false;

	if( *mask == 0 || *mask == 0x20 )
		mask = L"255.255.255.0";

	std::string _mask(mask, mask+wcslen(mask));
	int maskbits = IpMaskToBits(_mask.c_str());
	_mask = std::string(it->second.netmask.begin(), it->second.netmask.end());
	int oldmaskbits = IpMaskToBits(_mask.c_str());

	if( *bcip == 0 || *bcip == 0x20 )
		bcip = L"255.255.255.255";

	if( it->second.ip == newip && maskbits == oldmaskbits && it->second.broadcast == bcip )
		return false;

	char buffer[MAX_CMD_LEN];
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEIP_CMD, oldip, oldmaskbits, name.c_str(), newip, maskbits, bcip, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEIP_CMD, name.c_str(), oldip, oldmaskbits, name.c_str(), newip, maskbits, bcip) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			// we need update ip and mask for next commands
			IpAddressInfo _ip;
			_ip.flags = it->second.flags;
			_ip.ip = newip;
			std::string _ipmask(IpBitsToMask((uint32_t)maskbits, buffer, sizeof(buffer)));
			_ip.netmask = std::wstring(_ipmask.begin(),_ipmask.end());
			ip.erase(it);
			ip[newip] = _ip;
			return true;
		}
	return false;
}

bool NetInterface::ChangeIp6(const wchar_t * oldip, const wchar_t * newip, const wchar_t * mask)
{
	auto it = ip6.find(oldip);
	if( it == ip6.end() )
		return false;

	std::string _mask(mask, mask+wcslen(mask));
	int maskbits = Ip6MaskToBits(_mask.c_str());
	_mask = std::string(it->second.netmask.begin(), it->second.netmask.end());
	int oldmaskbits = Ip6MaskToBits(_mask.c_str());

	if( it->second.ip == newip && maskbits == oldmaskbits )
		return false;

	char buffer[MAX_CMD_LEN];
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEIP6_CMD, oldip, oldmaskbits, name.c_str(), newip, maskbits, name.c_str()) > 0 )
#else
	if( snprintf(buffer, MAX_CMD_LEN, CHANGEIP6_CMD, name.c_str(), oldip, oldmaskbits, name.c_str(), newip, maskbits) > 0 )
#endif
		if( RootExec(buffer) == 0 ) {
			// we need update ip and mask for next commands
			IpAddressInfo _ip;
			_ip.flags = it->second.flags;
			_ip.ip = newip;
			_ip.netmask = mask;
			// delete old ip
			ip6.erase(it);
			ip6[newip] = _ip;
			return true;
		}
	return false;
}

bool NetInterface::TcpDumpStart(const wchar_t * file, bool promisc)
{
	LOG_INFO("file: %S promisc: %s\n", file, promisc ? "true":"false");
	SetPromisc(promisc);
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, TCPDUMP_CMD, name.c_str(), file, name.c_str(), file) > 0 )
		tcpdumpOn = (RootExec(buffer) == 0);
	LOG_INFO("tcpdumpOn %s\n", tcpdumpOn ? "true":"false");
	return tcpdumpOn;
}

void NetInterface::TcpDumpStop(void)
{
	LOG_INFO("\n");
	if( !tcpdumpOn )
		return;
	char buffer[MAX_CMD_LEN];
	if( snprintf(buffer, MAX_CMD_LEN, TCPDUMPKILL_CMD, name.c_str()) > 0 )
		tcpdumpOn = !(RootExec(buffer) == 0);
	return;
}

