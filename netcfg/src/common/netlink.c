#if !defined(__APPLE__) && !defined(__FreeBSD__)

// https://habr.com/ru/articles/121254/

#include "netlink.h"
#include "netutils.h"

#include "errname.h"
#include "log.h"

#define LOG_SOURCE_FILE "netlink.c"

#ifdef MAIN_COMMON_NETLINK
const char * LOG_FILE = "";
#else
extern const char * LOG_FILE;
#endif

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define NETLINK_SEND_BUFFER_SIZE (32768)
#define NETLINK_RECV_BUFFER_SIZE (1024 * 1024)
#define NETLINK_EXT_ASK TRUE

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%lld"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%ld"
#else
    #error "Environment not 32 or 64-bit."
#endif

typedef struct {
	struct nlmsghdr nlm;
	char * info;
	struct rtattr *tb[1];
} UniversalRecord;

typedef struct {
	int netlink_socket;
	size_t sndbufsize;
	size_t rcvbufsize;
	uint32_t sequence_number;
	int ext_ask;

	ssize_t rcvsize;
	size_t offset;
	int totalmsg;
	int currentMsg;

	size_t infosize;
	uint16_t max_tbl_items;
	const char * (*typeprint)( uint16_t type );

	union {
		char * ptr;
		UniversalRecord * ur;
		RouteRecord * rt_recs;
		LinkRecord * link_recs;
		AddrRecord * adr_recs;
		RuleRecord * rule_recs;
		NeighborRecord * neighbor_recs;
	} info;

	struct sockaddr_nl sa;
	union {
		char * buf;
		struct nlmsghdr * nlh;
	};
} netlink_ctx;

#ifndef FIELD_OFFSET
#define	FIELD_OFFSET(s, field) ((addr_t)&((s *)(0))->field)
#define	addr_t unsigned long
#endif

#ifndef NETLINK_EXT_ACK
#define NETLINK_EXT_ACK	11
#endif

const char * fibruleflagsname(uint32_t flags) // FIB_RULE_
{
	static char buf[sizeof("PERMANENT, INVERT, UNRESOLVED, IIF_DETACHED, OIF_DETACHED, FIND_SADDR")] = {0};
	buf[0] = '\0';
	flags = addflag(buf, "PERMANENT", flags, FIB_RULE_PERMANENT);
	flags = addflag(buf, "INVERT", flags, FIB_RULE_INVERT);
	flags = addflag(buf, "UNRESOLVED", flags, FIB_RULE_UNRESOLVED);
	flags = addflag(buf, "IIF_DETACHED", flags, FIB_RULE_DEV_DETACHED);
	flags = addflag(buf, "OIF_DETACHED", flags, FIB_RULE_OIF_DETACHED);
	flags = addflag(buf, "FIND_SADDR", flags, FIB_RULE_FIND_SADDR);
	return buf;
}


const char * ndmsgflagsname(uint8_t flags)
{
	static char buf[sizeof("use, self, master, proxy, extern_learn, offload, sticky, router")] = {0};
	buf[0] = '\0';
	flags = addflag(buf, "use", flags, NTF_USE);
	flags = addflag(buf, "self", flags, NTF_SELF);
	flags = addflag(buf, "master", flags, NTF_MASTER);
	flags = addflag(buf, "proxy", flags, NTF_PROXY);
	flags = addflag(buf, "extern_learn", flags, NTF_EXT_LEARNED);
	flags = addflag(buf, "offload", flags, NTF_OFFLOADED);
	flags = addflag(buf, "sticky", flags, NTF_STICKY);
	flags = addflag(buf, "router", flags, NTF_ROUTER);

	return buf;
}

const char * tunnelflagsname(uint16_t flags) 
{
	static char buf[sizeof("csum, routing, key, seq, strict, rec, version, no key, dont fragment, oam, crit opt, geneve opt, vxlan opt, nocache, erspan opt, gtp opt")] = {0};
	buf[0] = '\0';
	flags = addflag_space_separator(buf, "csum", flags, TUNNEL_CSUM);
	flags = addflag_space_separator(buf, "routing", flags, TUNNEL_ROUTING);
	flags = addflag_space_separator(buf, "key", flags, TUNNEL_KEY);
	flags = addflag_space_separator(buf, "seq", flags, TUNNEL_SEQ);
	flags = addflag_space_separator(buf, "strict", flags, TUNNEL_STRICT);
	flags = addflag_space_separator(buf, "rec", flags, TUNNEL_REC);
	flags = addflag_space_separator(buf, "version", flags, TUNNEL_VERSION);
	flags = addflag_space_separator(buf, "no key", flags, TUNNEL_NO_KEY);
	flags = addflag_space_separator(buf, "dont fragment", flags, TUNNEL_DONT_FRAGMENT);
	flags = addflag_space_separator(buf, "oam", flags, TUNNEL_OAM);
	flags = addflag_space_separator(buf, "crit_opts", flags, TUNNEL_CRIT_OPT);
	flags = addflag_space_separator(buf, "nocache", flags, TUNNEL_NOCACHE);
	flags = addflag_space_separator(buf, "gtp_opts", flags, TUNNEL_GTP_OPT);
	flags = addflag_space_separator(buf, "geneve_opts", flags, TUNNEL_GENEVE_OPT);
	flags = addflag_space_separator(buf, "vxlan_opts", flags, TUNNEL_VXLAN_OPT);
	flags = addflag_space_separator(buf, "erspan_opts", flags, TUNNEL_ERSPAN_OPT);
	return buf;
}


const char * iptunneloptstype( uint16_t type ) // LWTUNNEL_IP_OPTS_
{
	static const char * iptunnelopts_type[] = {
		"LWTUNNEL_IP_OPTS_UNSPEC",
		"LWTUNNEL_IP_OPTS_GENEVE",
		"LWTUNNEL_IP_OPTS_VXLAN",
		"LWTUNNEL_IP_OPTS_ERSPAN"
	};
	if( type < sizeof(iptunnelopts_type)/sizeof(iptunnelopts_type[0]) )
		return iptunnelopts_type[type];

	return "UNKNOWN";
}

const char * iptunneloptsgenevetype( uint16_t type ) // LWTUNNEL_IP_OPT_GENEVE_
{
	static const char * iptunneloptsgeneve_type[] = {
		"LWTUNNEL_IP_OPT_GENEVE_UNSPEC",
		"LWTUNNEL_IP_OPT_GENEVE_CLASS",
		"LWTUNNEL_IP_OPT_GENEVE_TYPE",
		"LWTUNNEL_IP_OPT_GENEVE_DATA"
	};
	if( type < sizeof(iptunneloptsgeneve_type)/sizeof(iptunneloptsgeneve_type[0]) )
		return iptunneloptsgeneve_type[type];

	return "UNKNOWN";
}

const char * iptunneloptsvxlantype( uint16_t type ) // LWTUNNEL_IP_OPT_VXLAN_
{
	static const char * iptunneloptsvxlan_type[] = {
		"LWTUNNEL_IP_OPT_VXLAN_UNSPEC",
		"LWTUNNEL_IP_OPT_VXLAN_GBP"
	};
	if( type < sizeof(iptunneloptsvxlan_type)/sizeof(iptunneloptsvxlan_type[0]) )
		return iptunneloptsvxlan_type[type];

	return "UNKNOWN";
}

const char * iptunneloptserspantype( uint16_t type ) // LWTUNNEL_IP_OPT_ERSPAN_
{
	static const char * iptunneloptserspan_type[] = {
		"LWTUNNEL_IP_OPT_ERSPAN_UNSPEC",
		"LWTUNNEL_IP_OPT_ERSPAN_VER",
		"LWTUNNEL_IP_OPT_ERSPAN_INDEX",
		"LWTUNNEL_IP_OPT_ERSPAN_DIR",
		"LWTUNNEL_IP_OPT_ERSPAN_HWID"
	};
	if( type < sizeof(iptunneloptserspan_type)/sizeof(iptunneloptserspan_type[0]) )
		return iptunneloptserspan_type[type];

	return "UNKNOWN";
}

const char * rtnhflagsname(uint8_t flags)
{
	static char buf[sizeof("dead, onlink, pervasive, offload, trap, notify, linkdown, unresolved, rt_offload, rt_offload_failed")] = {0};
	buf[0] = '\0';
	flags = addflag(buf, "dead", flags, RTNH_F_DEAD);
	flags = addflag(buf, "onlink", flags, RTNH_F_ONLINK);
	flags = addflag(buf, "pervasive", flags, RTNH_F_PERVASIVE);
	flags = addflag(buf, "offload", flags, RTNH_F_OFFLOAD);
	flags = addflag(buf, "trap", flags, RTNH_F_TRAP);
	flags = addflag(buf, "notify", flags, RTM_F_NOTIFY);
	flags = addflag(buf, "linkdown", flags, RTNH_F_LINKDOWN);
	flags = addflag(buf, "unresolved", flags, RTNH_F_UNRESOLVED);
	flags = addflag(buf, "rt_offload", flags, RTM_F_OFFLOAD);
	flags = addflag(buf, "rt_trap", flags, RTM_F_TRAP);
	flags = addflag(buf, "rt_offload_failed", flags, RTM_F_OFFLOAD_FAILED);

	return buf;
}

const char * ndmsgflags_extname(uint32_t flags)
{
	static char buf[sizeof("managed, locked")] = {0};
	buf[0] = '\0';
	flags = addflag(buf, "managed", flags, NTF_EXT_MANAGED);
	flags = addflag(buf, "locked", flags, NTF_EXT_LOCKED);
	return buf;
}

const char * ndmsgstate(uint16_t state)
{
	if( state & NUD_PERMANENT )
		return "permanent";
	else if( state & NUD_REACHABLE )
		return "reachable";
	else if( state & NUD_NOARP )
		return "noarp";
	else if( state == NUD_NONE )
		return "none";
	else if( state & NUD_STALE )
		return "stale";
	else if( state & NUD_INCOMPLETE )
		return "incomplete";
	else if( state & NUD_DELAY )
		return "delay";
	else if( state & NUD_PROBE )
		return "probe";
	else if( state & NUD_FAILED )
		return "failed";
	return "UNKNOWN";
}

const char * rticmp6pref(uint8_t pref)
{
	static char s[sizeof("256")] = {0};
	switch (pref) {
	case ICMPV6_ROUTER_PREF_LOW:
		return "low";
	case ICMPV6_ROUTER_PREF_MEDIUM:
		return "medium";
	case ICMPV6_ROUTER_PREF_HIGH:
		return "high";
	default:
		if( snprintf(s, sizeof(s), "%u", pref) > 0 )
			return s;
	}
	return "";
}

const char * rtruletable( uint32_t table )
{
	static char s[sizeof("4294967296")] = {0};
	switch(table) {
		case RT_TABLE_UNSPEC:
			return "all"; // "RT_TABLE_UNSPEC";
		case RT_TABLE_COMPAT:
			return "RT_TABLE_COMPAT";
		case RT_TABLE_DEFAULT:
			return "default";
		case RT_TABLE_MAIN:
			return "main";
		case RT_TABLE_LOCAL:
			return "local";
		default:
			if( snprintf(s, sizeof(s), "%u", table) > 0 )
				return s;
	};
	return "";
}

const char * fractionrule( uint8_t action )
{
	static char s[sizeof("256")] = {0};
	switch(action) {
		case FR_ACT_UNSPEC:
			return "unspec";//FR_ACT_UNSPEC;
		case FR_ACT_TO_TBL:
			return "table";//FR_ACT_TO_TBL (Pass to fixed table)";
		case FR_ACT_GOTO:
			return "goto"; //FR_ACT_GOTO (Jump to another rule)";
		case FR_ACT_NOP:
			return "nop"; //FR_ACT_NOP (No operation)";
		case FR_ACT_BLACKHOLE:
			return "blackhole";//FR_ACT_BLACKHOLE (Drop without notification)
		case FR_ACT_UNREACHABLE:
			return "unreachable";//FR_ACT_UNREACHABLE (Drop with ENETUNREACH)
		case FR_ACT_PROHIBIT:
			return "prohibit"; //FR_ACT_PROHIBIT (Drop with EACCES)
		case RTN_THROW:
			return "throw"; //RTN_THROW (Not in this table)
		case RTN_NAT:
			return "nat"; //RTN_NAT (Translate this address)
		case RTN_XRESOLVE:
			return "xresolve"; //RTN_XRESOLVE (Use external resolver)

		default:
			if( snprintf(s, sizeof(s), "%u", action) > 0 )
				return s;
	};
	return "";
}

const char * rtscopetype( uint8_t scope )
{
	static char s[sizeof("256")] = {0};
	switch(scope) {
		case RT_SCOPE_UNIVERSE:
			return "global"; //"universe";
		case RT_SCOPE_SITE:
			return "site";
		case RT_SCOPE_LINK:
			return "link";
		case RT_SCOPE_HOST:
			return "host";
		case RT_SCOPE_NOWHERE:
			return "nowhere";
	};
	if( snprintf(s, sizeof(s), "%u", scope) > 0 )
		return s;
	return "";
}

const char * rtprotocoltype( uint8_t type )
{
	static char s[sizeof("256")] = {0};
	switch(type) {

		case RTPROT_UNSPEC:
			return "unspec";
		case RTPROT_REDIRECT:		// 1	/* Route installed by ICMP redirects not used by current IPv4 */
			return "redirect";
		case RTPROT_KERNEL:		// 2	/* Route installed by kernel		*/
			return "kernel";
		case RTPROT_BOOT:		// 3	/* Route installed during boot		*/
			return "boot";
		case RTPROT_STATIC:		// 4	/* Route installed by administrator	*/
			return "static";
		case RTPROT_DHCP:		// 16	/* DHCP client */
			return "dhcp";
/* Values of protocol >= RTPROT_STATIC are not interpreted by kernel;
   they are just passed from user and back as is.
   It will be used by hypothetical multiple routing daemons.
   Note that protocol values should be standardized in order to
   avoid conflicts.
 */
		case RTPROT_GATED:		// 8	/* Apparently, GateD */
			return "gated";
		case RTPROT_RA:			// 9	/* RDISC/ND router advertisements */
			return "ra";
		case RTPROT_MRT:		// 10	/* Merit MRT */
			return "mrt";
		case RTPROT_ZEBRA:		// 11	/* Zebra */
			return "zebra";
		case RTPROT_BIRD:		// 12	/* BIRD */
			return "bird";
		case RTPROT_DNROUTED:		// 13	/* DECnet routing daemon */
			return "dnrouted";
		case RTPROT_XORP:		// 14	/* XORP */
			return "xorp";
		case RTPROT_NTK:		// 15	/* Netsukuku */
			return "ntk";
		case RTPROT_MROUTED:		// 17	/* Multicast daemon */
			return "RTPROT_MROUTED";
		case RTPROT_KEEPALIVED:		// 18	/* Keepalived daemon */
			return "keepalived";
		case RTPROT_BABEL:		// 42	/* Babel daemon */
			return "babel";
		case RTPROT_OPENR:		// 99	/* Open Routing (Open/R) Routes */
			return "RTPROT_OPENR";
		case RTPROT_BGP:		// 186	/* BGP Routes */
			return "bgp";
		case RTPROT_ISIS:		// 187	/* ISIS Routes */
			return "isis";
		case RTPROT_OSPF:		// 188	/* OSPF Routes */
			return "ospf";
		case RTPROT_RIP:		// 189	/* RIP Routes */
			return "rip";
		case RTPROT_EIGRP:		// 192	/* EIGRP Routes */
			return "eigrp";
	};

	if( snprintf(s, sizeof(s), "%u", type) > 0 )
		return s;

	return "";
}

const char *lwtunnelencaptype(uint16_t type)
{
	static char s[sizeof("65536")] = {0};
	switch (type) {
	case LWTUNNEL_ENCAP_NONE:
		return "";
	case LWTUNNEL_ENCAP_MPLS:
		return "mpls";
	case LWTUNNEL_ENCAP_IP:
		return "ip";
	case LWTUNNEL_ENCAP_IP6:
		return "ip6";
	case LWTUNNEL_ENCAP_ILA:
		return "ila";
	case LWTUNNEL_ENCAP_BPF:
		return "bpf";
	case LWTUNNEL_ENCAP_SEG6:
		return "seg6";
	case LWTUNNEL_ENCAP_SEG6_LOCAL:
		return "seg6local";
	case LWTUNNEL_ENCAP_RPL:
		return "rpl";
	case LWTUNNEL_ENCAP_IOAM6:
		return "ioam6";
	case LWTUNNEL_ENCAP_XFRM:
		return "xfrm";
	}
	if( snprintf(s, sizeof(s), "%u", type) > 0 )
		return s;
	return "";
}

#define CASE_NLMSGTYPE(type) \
	case type: return #type

const char * nlmsgtype(uint16_t type)
{
	switch( type ) {
	CASE_NLMSGTYPE(NLMSG_NOOP);
	CASE_NLMSGTYPE(NLMSG_ERROR);
	CASE_NLMSGTYPE(NLMSG_DONE);
	CASE_NLMSGTYPE(NLMSG_OVERRUN);
	CASE_NLMSGTYPE(RTM_NEWLINK);
	CASE_NLMSGTYPE(RTM_DELLINK);
	CASE_NLMSGTYPE(RTM_GETLINK);
	CASE_NLMSGTYPE(RTM_SETLINK);
	CASE_NLMSGTYPE(RTM_NEWADDR);
	CASE_NLMSGTYPE(RTM_DELADDR);
	CASE_NLMSGTYPE(RTM_GETADDR);
	CASE_NLMSGTYPE(RTM_NEWROUTE);
	CASE_NLMSGTYPE(RTM_DELROUTE);
	CASE_NLMSGTYPE(RTM_GETROUTE);
	CASE_NLMSGTYPE(RTM_NEWNEIGH);
	CASE_NLMSGTYPE(RTM_DELNEIGH);
	CASE_NLMSGTYPE(RTM_GETNEIGH);
	CASE_NLMSGTYPE(RTM_NEWRULE);
	CASE_NLMSGTYPE(RTM_DELRULE);
	CASE_NLMSGTYPE(RTM_GETRULE);
	CASE_NLMSGTYPE(RTM_NEWQDISC);
	CASE_NLMSGTYPE(RTM_DELQDISC);
	CASE_NLMSGTYPE(RTM_GETQDISC);
	CASE_NLMSGTYPE(RTM_NEWTCLASS);
	CASE_NLMSGTYPE(RTM_DELTCLASS);
	CASE_NLMSGTYPE(RTM_GETTCLASS);
	CASE_NLMSGTYPE(RTM_NEWTFILTER);
	CASE_NLMSGTYPE(RTM_DELTFILTER);
	CASE_NLMSGTYPE(RTM_GETTFILTER);
	CASE_NLMSGTYPE(RTM_NEWACTION);
	CASE_NLMSGTYPE(RTM_DELACTION);
	CASE_NLMSGTYPE(RTM_GETACTION);
	CASE_NLMSGTYPE(RTM_NEWPREFIX);
	CASE_NLMSGTYPE(RTM_GETMULTICAST);
	CASE_NLMSGTYPE(RTM_GETANYCAST);
	CASE_NLMSGTYPE(RTM_NEWNEIGHTBL);
	CASE_NLMSGTYPE(RTM_GETNEIGHTBL);
	CASE_NLMSGTYPE(RTM_SETNEIGHTBL);
	CASE_NLMSGTYPE(RTM_NEWNDUSEROPT);
	CASE_NLMSGTYPE(RTM_NEWADDRLABEL);
	CASE_NLMSGTYPE(RTM_DELADDRLABEL);
	CASE_NLMSGTYPE(RTM_GETADDRLABEL);
	CASE_NLMSGTYPE(RTM_GETDCB);
	CASE_NLMSGTYPE(RTM_SETDCB);
	CASE_NLMSGTYPE(RTM_NEWNETCONF);
	CASE_NLMSGTYPE(RTM_DELNETCONF);
	CASE_NLMSGTYPE(RTM_GETNETCONF);
	CASE_NLMSGTYPE(RTM_NEWMDB);
	CASE_NLMSGTYPE(RTM_DELMDB);
	CASE_NLMSGTYPE(RTM_GETMDB);
	CASE_NLMSGTYPE(RTM_NEWNSID);
	CASE_NLMSGTYPE(RTM_DELNSID);
	CASE_NLMSGTYPE(RTM_GETNSID);
	CASE_NLMSGTYPE(RTM_NEWSTATS);
	CASE_NLMSGTYPE(RTM_GETSTATS);
	CASE_NLMSGTYPE(RTM_SETSTATS);
	CASE_NLMSGTYPE(RTM_NEWCACHEREPORT);
	CASE_NLMSGTYPE(RTM_NEWCHAIN);
	CASE_NLMSGTYPE(RTM_DELCHAIN);
	CASE_NLMSGTYPE(RTM_GETCHAIN);
	CASE_NLMSGTYPE(RTM_NEWNEXTHOP);
	CASE_NLMSGTYPE(RTM_DELNEXTHOP);
	CASE_NLMSGTYPE(RTM_GETNEXTHOP);
	CASE_NLMSGTYPE(RTM_NEWLINKPROP);
	CASE_NLMSGTYPE(RTM_DELLINKPROP);
	CASE_NLMSGTYPE(RTM_GETLINKPROP);
	CASE_NLMSGTYPE(RTM_NEWVLAN);
	CASE_NLMSGTYPE(RTM_DELVLAN);
	CASE_NLMSGTYPE(RTM_GETVLAN);
	CASE_NLMSGTYPE(RTM_NEWNEXTHOPBUCKET);
	CASE_NLMSGTYPE(RTM_DELNEXTHOPBUCKET);
	CASE_NLMSGTYPE(RTM_GETNEXTHOPBUCKET);
	CASE_NLMSGTYPE(RTM_NEWTUNNEL);
	CASE_NLMSGTYPE(RTM_DELTUNNEL);
	CASE_NLMSGTYPE(RTM_GETTUNNEL);
	};
	return "UNKNOWN";
}

const char * rtaxtype( uint16_t type )
{
	static const char * rtax_type[] = {
		"RTAX_UNSPEC",
		"RTAX_LOCK",
		"RTAX_MTU",
		"RTAX_WINDOW",
		"RTAX_RTT",
		"RTAX_RTTVAR",
		"RTAX_SSTHRESH",
		"RTAX_CWND",
		"RTAX_ADVMSS",
		"RTAX_REORDERING",
		"RTAX_HOPLIMIT",
		"RTAX_INITCWND",
		"RTAX_FEATURES",
		"RTAX_RTO_MIN",
		"RTAX_INITRWND",
		"RTAX_QUICKACK",
		"RTAX_CC_ALGO",
		"RTAX_FASTOPEN_NO_COOKIE",
	};
	if( type < sizeof(rtax_type)/sizeof(rtax_type[0]) )
		return rtax_type[type];
	return "UNKNOWN";
}

const char * rtatype( uint16_t type )
{
	static const char * rta_type[] = {
		"RTA_UNSPEC",
		"RTA_DST",
		"RTA_SRC",
		"RTA_IIF",
		"RTA_OIF",
		"RTA_GATEWAY",
		"RTA_PRIORITY",
		"RTA_PREFSRC",
		"RTA_METRICS",
		"RTA_MULTIPATH",
		"RTA_PROTOINFO", /* no longer used */
		"RTA_FLOW",
		"RTA_CACHEINFO",
		"RTA_SESSION", /* no longer used */
		"RTA_MP_ALGO", /* no longer used */
		"RTA_TABLE",
		"RTA_MARK",
		"RTA_MFC_STATS",
		"RTA_VIA",
		"RTA_NEWDST",
		"RTA_PREF",
		"RTA_ENCAP_TYPE",
		"RTA_ENCAP",
		"RTA_EXPIRES",
		"RTA_PAD",
		"RTA_UID",
		"RTA_TTL_PROPAGATE",
		"RTA_IP_PROTO",
		"RTA_SPORT",
		"RTA_DPORT",
		"RTA_NH_ID"
	};
	if( type < sizeof(rta_type)/sizeof(rta_type[0]) )
		return rta_type[type];

	return "UNKNOWN";
}

const char * mplsiptunneltype( uint16_t type )
{
	static const char * mplsiptunnel_type[] = {
		"MPLS_IPTUNNEL_UNSPEC",
		"MPLS_IPTUNNEL_DST",
		"MPLS_IPTUNNEL_TTL"
	};
	if( type < sizeof(mplsiptunnel_type)/sizeof(mplsiptunnel_type[0]) )
		return mplsiptunnel_type[type];

	return "UNKNOWN";
}


const char * iptunneltype( uint16_t type )
{
	static const char * iptunnel_type[] = {
		"LWTUNNEL_IP_UNSPEC",
		"LWTUNNEL_IP_ID",
		"LWTUNNEL_IP_DST",
		"LWTUNNEL_IP_SRC",
		"LWTUNNEL_IP_TTL",
		"LWTUNNEL_IP_TOS",
		"LWTUNNEL_IP_FLAGS",
		"LWTUNNEL_IP_PAD",
		"LWTUNNEL_IP_OPTS"
	};
	if( type < sizeof(iptunnel_type)/sizeof(iptunnel_type[0]) )
		return iptunnel_type[type];

	return "UNKNOWN";
}

const char * ip6tunneltype( uint16_t type )
{
	static const char * ip6tunnel_type[] = {
		"LWTUNNEL_IP6_UNSPEC",
		"LWTUNNEL_IP6_ID",
		"LWTUNNEL_IP6_DST",
		"LWTUNNEL_IP6_SRC",
		"LWTUNNEL_IP6_HOPLIMIT",
		"LWTUNNEL_IP6_TC",
		"LWTUNNEL_IP6_FLAGS",
		"LWTUNNEL_IP6_PAD",
		"LWTUNNEL_IP6_OPTS"
	};
	if( type < sizeof(ip6tunnel_type)/sizeof(ip6tunnel_type[0]) )
		return ip6tunnel_type[type];

	return "UNKNOWN";
}


/* Reference: RFC 5462, RFC 3032
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                Label                  | TC  |S|       TTL     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *	Label:  Label Value, 20 bits
 *	TC:     Traffic Class field, 3 bits
 *	S:      Bottom of Stack, 1 bit
 *	TTL:    Time to Live, 8 bits
 */
// MPLS (Multi Protocol Label Switching) - это технология маршрутизации по меткам
// https://ru.wikipedia.org/wiki/MPLS
void mpls_ntop(const struct mpls_label *addr, char * buf, size_t destlen)
{
	char *dest = buf;
	int count = 0;

	while (1) {
		uint32_t entry = ntohl(addr[count++].entry);
		uint32_t label = (entry & MPLS_LS_LABEL_MASK) >> MPLS_LS_LABEL_SHIFT;
		int len = snprintf(dest, destlen, "%u", label);

		if (len >= destlen)
			break;

		if (entry & MPLS_LS_S_MASK)
			return;

		dest += len;
		destlen -= len;
		if( destlen ) {
			*dest = '/';
			dest++;
			destlen--;
		}
	}
	errno = -E2BIG;
	return;
}



const char * ifatype( uint16_t type )
{
	static const char * ifa_type[] = {
		"IFA_UNSPEC",
		"IFA_ADDRESS",
		"IFA_LOCAL",
		"IFA_LABEL",
		"IFA_BROADCAST",
		"IFA_ANYCAST",
		"IFA_CACHEINFO",
		"IFA_MULTICAST",
		"IFA_FLAGS",
		"IFA_RT_PRIORITY",
		"IFA_TARGET_NETNSID",
		"IFA_PROTO"
	};
	if( type < sizeof(ifa_type)/sizeof(ifa_type[0]) )
		return ifa_type[type];
	return "UNKNOWN";
}

const char * iflatype( uint16_t type )
{
	static const char * ifla_type[] = {
		"IFLA_UNSPEC",
		"IFLA_ADDRESS",
		"IFLA_BROADCAST",
		"IFLA_IFNAME",
		"IFLA_MTU",
		"IFLA_LINK",
		"IFLA_QDISC",
		"IFLA_STATS",
		"IFLA_COST",
		"IFLA_PRIORITY",
		"IFLA_MASTER",
		"IFLA_WIRELESS",		/* Wireless Extension event - see wireless.h */
		"IFLA_PROTINFO",		/* Protocol specific information for a link */
		"IFLA_TXQLEN",
		"IFLA_MAP",
		"IFLA_WEIGHT",
		"IFLA_OPERSTATE",
		"IFLA_LINKMODE",
		"IFLA_LINKINFO",  		/* IFLA_INFO_ iflainfotype () */
		"IFLA_NET_NS_PID",
		"IFLA_IFALIAS",
		"IFLA_NUM_VF",			/* Number of VFs if device is SR-IOV PF */
		"IFLA_VFINFO_LIST",
		"IFLA_STATS64",
		"IFLA_VF_PORTS",
		"IFLA_PORT_SELF",
		"IFLA_AF_SPEC",
		"IFLA_GROUP",		/* Group the device belongs to */
		"IFLA_NET_NS_FD",
		"IFLA_EXT_MASK",		/* Extended info mask, VFs, etc */
		"IFLA_PROMISCUITY",	/* Promiscuity count: > 0 means acts PROMISC */
		"IFLA_NUM_TX_QUEUES",
		"IFLA_NUM_RX_QUEUES",
		"IFLA_CARRIER",
		"IFLA_PHYS_PORT_ID",
		"IFLA_CARRIER_CHANGES",
		"IFLA_PHYS_SWITCH_ID",
		"IFLA_LINK_NETNSID",
		"IFLA_PHYS_PORT_NAME",
		"IFLA_PROTO_DOWN",
		"IFLA_GSO_MAX_SEGS",
		"IFLA_GSO_MAX_SIZE",
		"IFLA_PAD",
		"IFLA_XDP",
		"IFLA_EVENT",
		"IFLA_NEW_NETNSID",
		"IFLA_IF_NETNSID",
		//"IFLA_TARGET_NETNSID = IFLA_IF_NETNSID", /* new alias */
		"IFLA_CARRIER_UP_COUNT",
		"IFLA_CARRIER_DOWN_COUNT",
		"IFLA_NEW_IFINDEX",
		"IFLA_MIN_MTU",
		"IFLA_MAX_MTU",
		"IFLA_PROP_LIST",
		"IFLA_ALT_IFNAME", /* Alternative ifname */
		"IFLA_PERM_ADDRESS",
		"IFLA_PROTO_DOWN_REASON",
	/* device (sysfs) name as parent, used instead
	 * of IFLA_LINK where there's no parent netdev
	 */
		"IFLA_PARENT_DEV_NAME",
		"IFLA_PARENT_DEV_BUS_NAME",
		"IFLA_GRO_MAX_SIZE",
		"IFLA_TSO_MAX_SIZE",
		"IFLA_TSO_MAX_SEGS",
	// Allmulti count: > 0 means acts ALLMULTI
		"IFLA_ALLMULTI",
		"IFLA_DEVLINK_PORT",
		"IFLA_GSO_IPV4_MAX_SIZE",
		"IFLA_GRO_IPV4_MAX_SIZE",
	};
	if( type < sizeof(ifla_type)/sizeof(ifla_type[0]) )
		return ifla_type[type];
	return "UNKNOWN";
}

const char * iflainfotype( uint16_t type )
{
	static const char * linkinfotype[] = {
		"IFLA_INFO_UNSPEC",
		"IFLA_INFO_KIND",
		"IFLA_INFO_DATA",
		"IFLA_INFO_XSTATS",
		"IFLA_INFO_SLAVE_KIND",
		"IFLA_INFO_SLAVE_DATA",
	};
	if( type < sizeof(linkinfotype)/sizeof(linkinfotype[0]) )
		return linkinfotype[type];
	return "UNKNOWN";
}

const char * iflaxdptype( uint16_t type )
{
	static const char * xdptype[] = {
		"IFLA_XDP_UNSPEC",
		"IFLA_XDP_FD",
		"IFLA_XDP_ATTACHED",
		"IFLA_XDP_FLAGS",
		"IFLA_XDP_PROG_ID",
		"IFLA_XDP_DRV_PROG_ID",
		"IFLA_XDP_SKB_PROG_ID",
		"IFLA_XDP_HW_PROG_ID",
		"IFLA_XDP_EXPECTED_FD"
	};
	if( type < sizeof(xdptype)/sizeof(xdptype[0]) )
		return xdptype[type];
	return "UNKNOWN";
}

const char * fratype( uint16_t type )
{
	static const char * ruleinfotype[] = {
		"FRA_UNSPEC",
		"FRA_DST",	/* destination address */
		"FRA_SRC",	/* source address */
		"FRA_IIFNAME",	/* interface name */
		"FRA_GOTO",	/* target to jump to (FR_ACT_GOTO) */
		"FRA_UNUSED2",
		"FRA_PRIORITY",	/* priority/preference */
		"FRA_UNUSED3",
		"FRA_UNUSED4",
		"FRA_UNUSED5",
		"FRA_FWMARK",	/* mark */
		"FRA_FLOW",	/* flow/class id */
		"FRA_TUN_ID",
		"FRA_SUPPRESS_IFGROUP",
		"FRA_SUPPRESS_PREFIXLEN",
		"FRA_TABLE",	/* Extended table id */
		"FRA_FWMASK",	/* mask for netfilter mark */
		"FRA_OIFNAME",
		"FRA_PAD",
		"FRA_L3MDEV",	/* iif or oif is l3mdev goto its table */
		"FRA_UID_RANGE",	/* UID range */
		"FRA_PROTOCOL",   /* Originator of the rule */
		"FRA_IP_PROTO",	/* ip proto */
		"FRA_SPORT_RANGE", /* sport */
		"FRA_DPORT_RANGE", /* dport */
	};
	if( type < sizeof(ruleinfotype)/sizeof(ruleinfotype[0]) )
		return ruleinfotype[type];
	return "UNKNOWN";
}

const char * ndatype( uint16_t type )
{
	static const char * nda_type[] = {
		"NDA_UNSPEC",
		"NDA_DST",
		"NDA_LLADDR",
		"NDA_CACHEINFO",
		"NDA_PROBES",
		"NDA_VLAN",
		"NDA_PORT",
		"NDA_VNI",
		"NDA_IFINDEX",
		"NDA_MASTER",
		"NDA_LINK_NETNSID",
		"NDA_SRC_VNI",
		"NDA_PROTOCOL",  /* Originator of entry */
		"NDA_NH_ID",
		"NDA_FDB_EXT_ATTRS",
		"NDA_FLAGS_EXT",
		"NDA_NDM_STATE_MASK",
		"NDA_NDM_FLAGS_MASK",
	};
	if( type < sizeof(nda_type)/sizeof(nda_type[0]) )
		return nda_type[type];
	return "UNKNOWN";
}

const char * xdpattachedtype(uint8_t type)
{
	static const char * xdptype[] = {
		"XDP_ATTACHED_NONE",
		"XDP_ATTACHED_DRV",
		"XDP_ATTACHED_SKB",
		"XDP_ATTACHED_HW",
		"XDP_ATTACHED_MULTI"
	};
	if( type < sizeof(xdptype)/sizeof(xdptype[0]) )
		return xdptype[type];
	return "UNKNOWN";
}

const char * iflavlantype( uint16_t type )
{
	static const char * vlaninfotype[] = {
		"IFLA_VLAN_UNSPEC",
		"IFLA_VLAN_ID",
		"IFLA_VLAN_FLAGS",
		"IFLA_VLAN_EGRESS_QOS",
		"IFLA_VLAN_INGRESS_QOS",
		"IFLA_VLAN_PROTOCOL"
	};
	if( type < sizeof(vlaninfotype)/sizeof(vlaninfotype[0]) )
		return vlaninfotype[type];
	return "UNKNOWN";
}

const char * iflainet6type( uint16_t type )
{
	static const char * inet6type[] = {
		"IFLA_INET6_UNSPEC",
		"IFLA_INET6_FLAGS",	/* link flags			*/
		"IFLA_INET6_CONF",	/* sysctl parameters		*/
		"IFLA_INET6_STATS",	/* statistics			*/
		"IFLA_INET6_MCAST",	/* MC things. What of them?	*/
		"IFLA_INET6_CACHEINFO",	/* time values and max reasm size */
		"IFLA_INET6_ICMP6STATS",	/* statistics (icmpv6)		*/
		"IFLA_INET6_TOKEN",	/* device token			*/
		"IFLA_INET6_ADDR_GEN_MODE", /* implicit address generator mode */
		"IFLA_INET6_RA_MTU",	/* mtu carried in the RA message */
	};
	if( type < sizeof(inet6type)/sizeof(inet6type[0]) )
		return inet6type[type];
	return "UNKNOWN";
}

// TuneNetlinkSocket() returns zero on success. On error, -1 is returned, and
// errno is set to indicate the error.
int TuneNetlinkSocket(netlink_ctx * ctx)
{
	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );

	ctx->sndbufsize = NETLINK_SEND_BUFFER_SIZE;
	ctx->rcvbufsize = NETLINK_RECV_BUFFER_SIZE;
	ctx->ext_ask = NETLINK_EXT_ASK;

	if( setsockopt(ctx->netlink_socket,
			SOL_SOCKET,
			SO_SNDBUF,
			&ctx->sndbufsize,
			sizeof(ctx->sndbufsize)) < 0 ) {
		LOG_ERROR("setsockopt(SOL_SOCKET, SO_SNDBUF %u) ... error (%s)\n", \
			ctx->sndbufsize, errorname(errno));
		return -1;
	}

	if( setsockopt(ctx->netlink_socket,
			SOL_SOCKET,
			SO_RCVBUF,
			&ctx->rcvbufsize,
			sizeof(ctx->rcvbufsize)) < 0 ) {
		LOG_ERROR("setsockopt(SOL_SOCKET, SO_RCVBUF %u) ... error (%s)\n", \
			ctx->rcvbufsize, errorname(errno));
		return -1;
	}

	// Not all kernels support extended ACK reporting
	if( setsockopt(ctx->netlink_socket,
			SOL_NETLINK,
			NETLINK_EXT_ACK,
			&ctx->ext_ask,
			sizeof(ctx->ext_ask)) < 0 ) {
		LOG_WARN("setsockopt(SOL_NETLINK, NETLINK_EXT_ACK) ... warn (%s)\n", \
			errorname(errno));
		ctx->ext_ask = FALSE;
	}
	ctx->ext_ask = FALSE;
	return 0;
}

void CloseNetlink(void * nl)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );

	free(ctx->info.ptr);
	free(ctx->buf);

	if( ctx->netlink_socket >= 0  && close(ctx->netlink_socket) < 0 ) {
		LOG_ERROR("close(ctx->netlink_socket) ... error (%s)\n", errorname(errno));	
	}
	free(nl);
}

void * OpenNetlink(void)
{
	netlink_ctx * ctx = (netlink_ctx *)malloc(sizeof(netlink_ctx));
	socklen_t socket_size = sizeof(ctx->sa);

	if( !ctx ) {
		LOG_ERROR("malloc(%u) ... error (%s)\n", sizeof(netlink_ctx), errorname(errno));
		return 0;
	}

	memset(ctx, 0, sizeof(netlink_ctx));

	do {
		// NETLINK_ROUTE — получать уведомления об изменениях таблицы маршрутизации и сетевых интерфейсов.
		// NETLINK_USERSOCK — зарезервировано для определения пользовательских протоколов.
		// NETLINK_FIREWALL — служит для передачи IPv4 пакетов из сетевого фильтра на пользовательский уровень
		// NETLINK_INET_DIAG — мониторинг inet сокетов
		// NETLINK_NFLOG — ULOG сетевого/пакетного фильтра
		// NETLINK_SELINUX — получать уведомления от системы Selinux
		// NETLINK_NETFILTER — работа с подсистемой сетевого фильтра
		// NETLINK_KOBJECT_UEVENT — получение сообщений ядра
					
		#ifndef SOCK_CLOEXEC
		ctx->netlink_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
		#else
		ctx->netlink_socket = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
		#endif
		if( ctx->netlink_socket < 0 ) {
		#ifndef SOCK_CLOEXEC
			LOG_ERROR("socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE) ... error (%s)\n", errorname(errno));
		#else
			LOG_ERROR("socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE) ... error (%s)\n", errorname(errno));
		#endif
			break;
		}

		if( TuneNetlinkSocket(ctx) < 0 )
			break;

		ctx->sa.nl_family = AF_NETLINK;

		ctx->sa.nl_pid = 0;	// the kernel assigns the process ID to the first netlink socket the
					// process opens and assigns a unique nl_pid to every netlink socket
					// that the process subsequently creates
					// pthread_self() << 16 | getpid();

		ctx->sa.nl_groups = 0;	// When bind is called on the socket, the nl_groups
					// field in the sockaddr_nl should be set to a bit mask of the
					// groups which it wishes to listen to.
					// default value for this field is zero which means that no 
					// multicasts will be received
					// Only processes with an effective UID of 0 or the CAP_NET_ADMIN capability may
					// send (or recv) to a netlink multicast group. As at Linux 3.0, the NETLINK_KOBJECT_UEVENT,
					// NETLINK_GENERIC, NETLINK_ROUTE, and NETLINK_SELINUX groups allow other users to receive messages.
					// см. enum rtnetlink_groups в rtnetlink.h
					// RTMGRP_LINK — эта группа получает уведомления об изменениях в сетевых интерфейсах (интерфейс удалился, добавился, опустился, поднялся)
					// RTMGRP_NOTIFY - псевдо-группа - добавляется ко всем группам, чтобы получать уведомления
					// RTMGRP_NEIGH - изменения в таблице ARP Neighbor Table (ARP table for IPv4)
					// RTMGRP_TC - изменения в настройках Traffic Control планировщика пакетов.
					// RTMGRP_IPV4_IFADDR — эта группа получает уведомления об изменениях в IPv4 адресах интерфейсов (адрес был добавлен или удален)
					// RTMGRP_IPV6_IFADDR — эта группа получает уведомления об изменениях в IPv6 адресах интерфейсов (адрес был добавлен или удален)
					// RTMGRP_IPV4_MROUTE - изменения в таблице маршрутизации IPv4 Multicast.
					// RTMGRP_IPV6_MROUTE - изменения в таблице маршрутизации IPv6 Multicast.
					// RTMGRP_IPV4_ROUTE — эта группа получает уведомления об изменениях в таблице маршрутизации для IPv4 адресов
					// RTMGRP_IPV6_ROUTE — эта группа получает уведомления об изменениях в таблице маршрутизации для IPv6 адресов
					// RTMGRP_IPV4_RULE - изменения в таблице правил маршрутизации IPv4.
					// RTMGRP_IPV6_PREFIX - уведомления об изменениях в префиксах IPv6.
					// RTMGRP_DECnet_IFADDR уведомления об изменениях в адресах DECnet на интерфейсе.
					// RTMGRP_DECnet_ROUTE уведомления об изменениях в маршрутах DECnet.
					// DECnet - это набор аппаратных и программных продуктов для сетевого обмена данными, которые реализуют 
					// архитектуру цифровой сети (DNA) от Digital Equipment Corporation (DEC)
					// RTMGRP_IPV6_IFINFO - изменения в интерфесах IPv6

		if( bind(ctx->netlink_socket, (struct sockaddr *)&ctx->sa, sizeof(ctx->sa)) < 0) {
			LOG_ERROR("bind(netlink_socket) ... error (%s)\n", errorname(errno));
			break;
		}

		// check socket
		if( getsockname(ctx->netlink_socket, (struct sockaddr *)&ctx->sa, &socket_size) < 0 ) {
			LOG_ERROR("getsockname(netlink_socket) ... error (%s)\n", errorname(errno));
			break;
		}

		LOG_INFO("ctx->sa.nl_family %u (%s)\n", ctx->sa.nl_family, familyname(ctx->sa.nl_family));
		LOG_INFO("ctx->sa.nl_pid    %u (%u)\n", ctx->sa.nl_pid, getpid());
		LOG_INFO("ctx->sa.nl_groups 0x%08X\n", ctx->sa.nl_groups);

		assert( ctx->sa.nl_family == AF_NETLINK );
		assert( socket_size == sizeof(ctx->sa) );

		if( ctx->sa.nl_family != AF_NETLINK ||
		    socket_size != sizeof(ctx->sa) ) {
			LOG_ERROR("unsupported AF_NETLINK socket\n");
			break;
		}
		
		ctx->buf = (char *)malloc(ctx->sndbufsize > ctx->rcvbufsize ? ctx->sndbufsize:ctx->rcvbufsize);
		memset(ctx->buf, 0, ctx->sndbufsize > ctx->rcvbufsize ? ctx->sndbufsize:ctx->rcvbufsize);

	} while(0);

	if( !ctx->buf ) {
		CloseNetlink(ctx);
		ctx = 0;
	}
	
	return ctx;
}

static int EnumMsg(netlink_ctx * ctx, int (* fn)(netlink_ctx * ctx, struct nlmsghdr * nlh))
{
	struct nlmsghdr * nlh = (struct nlmsghdr *)(ctx->buf+ctx->offset);
	int res = FALSE, rcvsize = (int)ctx->rcvsize;

	assert( rcvsize > 0 && (size_t)rcvsize <= (ctx->rcvbufsize-ctx->offset) );

	LOG_INFO("nlh->nlmsg_type: %d (%s)\n", nlh->nlmsg_type, nlmsgtype(nlh->nlmsg_type));

	for(	; (rcvsize >= 0 && NLMSG_OK(nlh, (unsigned int)rcvsize));
		nlh = NLMSG_NEXT(nlh, rcvsize) ) {

		res = FALSE;

		// skip not our message
		if( nlh->nlmsg_pid && nlh->nlmsg_pid != ctx->sa.nl_pid ) {
			LOG_WARN("ports are diffrent (nlmsg_pid %d != sa.nl_pid %d)\n", nlh->nlmsg_pid, ctx->sa.nl_pid);
			errno = ESRCH;
			continue;
		}
		if( nlh->nlmsg_seq != ctx->sequence_number ) {
			LOG_WARN("sequence_number are diffrent (nlmsg_seq %d != sequence_number %d)\n", nlh->nlmsg_seq, ctx->sequence_number);
			errno = EPROTO;
			continue;
		}
		if( nlh->nlmsg_flags & NLM_F_DUMP_INTR ) {
			LOG_ERROR("receive data was interrupted\n");
			errno = EINTR;
			break;
		}

		if( nlh->nlmsg_type >= NLMSG_MIN_TYPE ) {
			res = fn(ctx, nlh);
			if( !res )
				break;
		} else {
			const struct nlmsgerr *err = (const struct nlmsgerr *)NLMSG_DATA(nlh);
			switch(nlh->nlmsg_type) {
			case NLMSG_ERROR:
				LOG_ERROR("NLMSG_ERROR (Error) %u\n", err->error);
				break;
			case NLMSG_DONE:
				LOG_INFO("NLMSG_DONE (End of a dump) %u\n", err->error);
				break;
			case NLMSG_NOOP:
				LOG_INFO("NLMSG_NOOP (Nothing) %u\n", err->error);
				res = TRUE;
				break;
			case NLMSG_OVERRUN:
				LOG_INFO("NLMSG_OVERRUN (Data lost) %u\n", err->error);
				break;
			}
			break;
		}
	}

	return res;
}

static int IncrementTotalMsg(netlink_ctx * ctx, struct nlmsghdr * nlh)
{
	ctx->totalmsg++;
	return TRUE;
}

const char * rttype(unsigned char rtm_type)
{
	const char * rtm_type_name[] = {
		"unspec",        // "RTN_UNSPEC",                                                    
		"unicast", 	 // "RTN_UNICAST (Gateway or direct route)",                         			
		"local", 	 // "RTN_LOCAL (Accept locally)",                                    			
		"broadcast",     // "RTN_BROADCAST (Accept locally as broadcast, send as broadcast)",
		"anycast",       // "RTN_ANYCAST (Accept locally as broadcast, but send as unicast)",
		"multicast", 	 // "RTN_MULTICAST (Multicast route)",                               			
		"blackhole",     // "RTN_BLACKHOLE (Drop)",                                          
		"unreachable",   // "RTN_UNREACHABLE (Destination is unreachable)",                  
		"prohibit",      // "RTN_PROHIBIT (Administratively prohibited)",                    
		"throw",         // "RTN_THROW (Not in this table)",                                 
		"nat",           // "RTN_NAT (Translate this address)",                              
		"xresolve"       // "RTN_XRESOLVE (Use external resolver)"
	};

	if( sizeof(rtm_type_name)/sizeof(rtm_type_name[0]) > rtm_type )
		return rtm_type_name[rtm_type];

	return "UNKNOWN";
}

/*
 * nla_type (16 bits)
 * +---+---+-------------------------------+
 * | N | O | Attribute Type                |
 * +---+---+-------------------------------+
 * N := Carries nested attributes
 * O := Payload stored in network byte order
 *
 * Note: The N and O flag are mutually exclusive.
 */
//#define NLA_F_NESTED		(1 << 15)
//#define NLA_F_NET_BYTEORDER	(1 << 14)
//#define NLA_TYPE_MASK		~(NLA_F_NESTED | NLA_F_NET_BYTEORDER)
int FillAttr(struct rtattr *rta, struct rtattr **tb, unsigned short max, unsigned short flagsmask, int len, const char * (*typeprint)(uint16_t type))
{
	assert( rta != 0 );
	assert( tb != 0 );
	assert( len > 0 );

	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while( RTA_OK(rta, len) ) {
		unsigned short type = rta->rta_type & flagsmask;
		LOG_INFO("%p: type: %s (%u) size %u\n", rta, typeprint ? typeprint(type):"", type, rta->rta_len);
		if( type <= max && !tb[type] )
			tb[type] = rta;
		rta = RTA_NEXT(rta, len);
	}

	if( len ) {
		LOG_ERROR("left len=%d, rta_len=%d\n", len, rta->rta_len);
		return FALSE;
	}

	return TRUE;
}

#define MAX_INETADR_LEN (INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN)

static void genevetostring(uint32_t total_geneves, Geneve * gen, char * buffer, size_t size)
{
	char * ptr = buffer;
	while( total_geneves-- ) {
		int res = 0;
		size_t index = 0;

		if( (res = snprintf(ptr, size, "%u:", gen->cls)) < 0 || size < res )
			break;
		ptr += res;
		size -= res;


		if( (res = snprintf(ptr, size, "%u:", gen->type)) < 0 || size < res )
			break;
		ptr += res;
		size -= res;

		while( index < gen->size ) {
			if( (res = snprintf(ptr, size, "%02x", gen->data[index])) < 0 || size < res )
				return;
			ptr += res;
			size -= res;
			index++;
		}

		if( total_geneves && ((res = snprintf(ptr, size, ",")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;

		gen++;
	}
	return;
}

static int FillIp(uint8_t family, Encap * enc, struct rtattr * rta)
{
	struct rtattr * ip[LWTUNNEL_IP_MAX+1];
	uint32_t addrlen = family == AF_INET ? sizeof(uint32_t):sizeof(struct in6_addr);

	//static_assert( LWTUNNEL_IP_MAX == LWTUNNEL_IP6_MAX, "unsupported LWTUNNEL_IP6_MAX" );
	assert( family == AF_INET || family == AF_INET6 );

	enc->data.ip.family = family;

	if( !FillAttr((struct rtattr*)RTA_DATA(rta),
		ip,
		LWTUNNEL_IP_MAX,
		(short unsigned int)(~NLA_F_NESTED),
		RTA_PAYLOAD(rta),
		family == AF_INET ? iptunneltype:ip6tunneltype) )
		return FALSE;

	if( ip[LWTUNNEL_IP_ID] && RTA_PAYLOAD(ip[LWTUNNEL_IP_ID]) >= sizeof(uint64_t) ) {
		enc->data.ip.id = ntohll(RTA_UINT64_T(ip[LWTUNNEL_IP_ID]));
		enc->data.ip.valid.id = 1;
		LOG_INFO("LWTUNNEL_IP_ID:  " LLFMT "\n", enc->data.ip.id);
	}

	if( ip[LWTUNNEL_IP_DST] && RTA_PAYLOAD(ip[LWTUNNEL_IP_DST]) >= addrlen ) {
		if( inet_ntop(family, RTA_DATA(ip[LWTUNNEL_IP_DST]), enc->data.ip.dst, sizeof(enc->data.ip.dst)) )
			enc->data.ip.valid.dst = 1;
		LOG_INFO("LWTUNNEL_IP_DST:  %s\n", enc->data.ip.dst);
	}

	if( ip[LWTUNNEL_IP_SRC] && RTA_PAYLOAD(ip[LWTUNNEL_IP_SRC]) >= addrlen ) {
		if( inet_ntop(family, RTA_DATA(ip[LWTUNNEL_IP_DST]), enc->data.ip.src, sizeof(enc->data.ip.src)) )
			enc->data.ip.valid.src = 1;
		LOG_INFO("LWTUNNEL_IP_SRC:  %s\n", enc->data.ip.src);
	}

	//static_assert( (int)LWTUNNEL_IP6_HOPLIMIT == (int)LWTUNNEL_IP_TTL, "unsupported LWTUNNEL_IP6_HOPLIMIT" );
	if( ip[LWTUNNEL_IP_TTL] && RTA_PAYLOAD(ip[LWTUNNEL_IP_TTL]) >= sizeof(uint8_t) ) {

		if( family == AF_INET6 ) {
			enc->data.ip.hoplimit = RTA_UINT8_T(ip[LWTUNNEL_IP6_HOPLIMIT]);
			enc->data.ip.valid.hoplimit = 1;
			LOG_INFO("LWTUNNEL_IP6_HOPLIMIT: %u\n", enc->data.ip.hoplimit);
		} else {
			enc->data.ip.ttl = RTA_UINT8_T(ip[LWTUNNEL_IP_TTL]);
			enc->data.ip.valid.ttl = 1;
			LOG_INFO("LWTUNNEL_IP_TTL: %u\n", enc->data.ip.ttl);
		}
	}

	//static_assert( (int)LWTUNNEL_IP6_TC == (int)LWTUNNEL_IP_TOS, "unsupported LWTUNNEL_IP6_TC" );
	if( ip[LWTUNNEL_IP_TOS] && RTA_PAYLOAD(ip[LWTUNNEL_IP_TOS]) >= sizeof(uint8_t) ) {
		if( family == AF_INET6 ) {
			enc->data.ip.tc = RTA_UINT8_T(ip[LWTUNNEL_IP6_TC]);
			enc->data.ip.valid.tc = 1;
			LOG_INFO("LWTUNNEL_IP6_TC: %u\n", enc->data.ip.tc);
		} else {
			enc->data.ip.tos = RTA_UINT8_T(ip[LWTUNNEL_IP_TOS]);
			enc->data.ip.valid.tos = 1;
			LOG_INFO("LWTUNNEL_IP_TOS: %u\n", enc->data.ip.tos);
		}
	}

	if( ip[LWTUNNEL_IP_FLAGS] && RTA_PAYLOAD(ip[LWTUNNEL_IP_FLAGS]) >= sizeof(uint16_t) ) {
		enc->data.ip.flags = RTA_UINT16_T(ip[LWTUNNEL_IP_FLAGS]);
		enc->data.ip.valid.flags = 1;
		LOG_INFO("LWTUNNEL_IP_FLAGS: 0x%04X (%s)\n", enc->data.ip.flags, tunnelflagsname(enc->data.ip.flags));
	}

	if( ip[LWTUNNEL_IP_OPTS] && RTA_PAYLOAD(ip[LWTUNNEL_IP_OPTS]) >= sizeof(uint32_t) ) {
		struct rtattr * opts[LWTUNNEL_IP_OPTS_MAX+1];
		if( FillAttr((struct rtattr*)RTA_DATA(ip[LWTUNNEL_IP_OPTS]),
			opts,
			LWTUNNEL_IP_OPTS_MAX,
			(short unsigned int)(~NLA_F_NESTED),
			RTA_PAYLOAD(ip[LWTUNNEL_IP_OPTS]),
			iptunneloptstype) ) {

			if( opts[LWTUNNEL_IP_OPTS_GENEVE] ) {
				int rest = (int)RTA_PAYLOAD(opts[LWTUNNEL_IP_OPTS_GENEVE]);
				struct rtattr *rta = (struct rtattr*)RTA_DATA(opts[LWTUNNEL_IP_OPTS_GENEVE]);
				struct rtattr * geneve[LWTUNNEL_IP_OPT_GENEVE_MAX+1] = {0};

				enc->data.ip.valid.geneve = 1;

				uint32_t total_geneves = 0;
				Geneve geneve_opts[MAX_GENEVE_OPTS] = {0};

				while( RTA_OK(rta, rest) ) {
					unsigned short type = rta->rta_type & (~NLA_F_NESTED);
					LOG_INFO("%p: type: %s (%u) size %u\n", rta, iptunneloptsgenevetype(type), type, rta->rta_len);
					if( type <= LWTUNNEL_IP_OPT_GENEVE_MAX && !geneve[type] )
						geneve[type] = rta;

					if(     total_geneves < MAX_GENEVE_OPTS && \
						geneve[LWTUNNEL_IP_OPT_GENEVE_CLASS] && \
						geneve[LWTUNNEL_IP_OPT_GENEVE_TYPE] && \
						geneve[LWTUNNEL_IP_OPT_GENEVE_DATA] ) {

						Geneve * g = &geneve_opts[total_geneves];

						g->size = (size_t)RTA_PAYLOAD(geneve[LWTUNNEL_IP_OPT_GENEVE_DATA]);
						g->size = g->size < sizeof(g->data) ? g->size:sizeof(g->data);

						if( RTA_PAYLOAD(geneve[LWTUNNEL_IP_OPT_GENEVE_CLASS]) >= sizeof(uint16_t) ) {
							g->cls = ntohs(RTA_UINT16_T(geneve[LWTUNNEL_IP_OPT_GENEVE_CLASS]));
							LOG_INFO("%d. LWTUNNEL_IP_OPT_GENEVE_CLASS: %u\n", total_geneves, g->cls);
						}

						if( RTA_PAYLOAD(geneve[LWTUNNEL_IP_OPT_GENEVE_TYPE]) >= sizeof(uint8_t) ) {
							g->type = RTA_UINT8_T(geneve[LWTUNNEL_IP_OPT_GENEVE_TYPE]);
							LOG_INFO("%d. LWTUNNEL_IP_OPT_GENEVE_TYPE: %u\n", total_geneves, g->type);
						}

						memmove(g->data, RTA_DATA(geneve[LWTUNNEL_IP_OPT_GENEVE_DATA]), g->size);
						LOG_INFO("%d. LWTUNNEL_IP_OPT_GENEVE_DATA: %02X%02X%02X\n", total_geneves, g->data[0], g->data[1], g->data[2]);

						memset(geneve, 0, sizeof(geneve));
						total_geneves++;
					}

					rta = RTA_NEXT(rta, rest);
				}
				genevetostring(total_geneves, geneve_opts, enc->data.ip.geneve_opts, sizeof(enc->data.ip.geneve_opts));
				LOG_INFO("geneve_opts %s\n", enc->data.ip.geneve_opts);
			}
			if( opts[LWTUNNEL_IP_OPTS_VXLAN] ) {
				struct rtattr * vxlan[LWTUNNEL_IP_OPT_VXLAN_MAX+1];
				enc->data.ip.valid.vxlan = 1;
				if( FillAttr((struct rtattr*)RTA_DATA(opts[LWTUNNEL_IP_OPTS_VXLAN]),
					vxlan,
					LWTUNNEL_IP_OPT_VXLAN_MAX,
					(short unsigned int)(~NLA_F_NESTED),
					RTA_PAYLOAD(opts[LWTUNNEL_IP_OPTS_VXLAN]),
					iptunneloptsvxlantype) ) {
					if( vxlan[LWTUNNEL_IP_OPT_VXLAN_GBP] && RTA_PAYLOAD(vxlan[LWTUNNEL_IP_OPT_VXLAN_GBP]) >= sizeof(uint32_t) ) {
						enc->data.ip.valid.vxlan_gbp = 1;
						enc->data.ip.vxlan_gbp = RTA_UINT32_T(vxlan[LWTUNNEL_IP_OPT_VXLAN_GBP]);
						LOG_INFO("LWTUNNEL_IP_OPT_VXLAN_GBP: %u\n", enc->data.ip.vxlan_gbp);
					}
				}
			}
			if( opts[LWTUNNEL_IP_OPTS_ERSPAN] ) {
				struct rtattr * erspan[LWTUNNEL_IP_OPT_ERSPAN_MAX+1];
				enc->data.ip.valid.erspan = 1;

				struct {
					uint8_t ver;
					uint32_t index;
					uint8_t dir;
					uint8_t hwid;
				} er = {0};

				if( FillAttr((struct rtattr*)RTA_DATA(opts[LWTUNNEL_IP_OPTS_ERSPAN]),
					erspan,
					LWTUNNEL_IP_OPT_ERSPAN_MAX,
					(short unsigned int)(~NLA_F_NESTED),
					RTA_PAYLOAD(opts[LWTUNNEL_IP_OPTS_ERSPAN]),
					iptunneloptserspantype) ) {

					if( erspan[LWTUNNEL_IP_OPT_ERSPAN_VER] && RTA_PAYLOAD(erspan[LWTUNNEL_IP_OPT_ERSPAN_VER]) >= sizeof(uint8_t) ) {
						er.ver = RTA_UINT8_T(erspan[LWTUNNEL_IP_OPT_ERSPAN_VER]);
						LOG_INFO("LWTUNNEL_IP_OPT_ERSPAN_VER: %u\n", er.ver);
					}

					if( erspan[LWTUNNEL_IP_OPT_ERSPAN_INDEX] && RTA_PAYLOAD(erspan[LWTUNNEL_IP_OPT_ERSPAN_INDEX]) >= sizeof(uint32_t) ) {
						er.index = ntohl(RTA_UINT32_T(erspan[LWTUNNEL_IP_OPT_ERSPAN_INDEX]));
						LOG_INFO("LWTUNNEL_IP_OPT_ERSPAN_INDEX: %u\n", er.index);
					}

					if( erspan[LWTUNNEL_IP_OPT_ERSPAN_DIR] && RTA_PAYLOAD(erspan[LWTUNNEL_IP_OPT_ERSPAN_DIR]) >= sizeof(uint8_t) ) {
						er.dir = RTA_UINT8_T(erspan[LWTUNNEL_IP_OPT_ERSPAN_DIR]);
						LOG_INFO("LWTUNNEL_IP_OPT_ERSPAN_DIR: %u\n", er.dir);
					}

					if( erspan[LWTUNNEL_IP_OPT_ERSPAN_HWID] && RTA_PAYLOAD(erspan[LWTUNNEL_IP_OPT_ERSPAN_HWID]) >= sizeof(uint8_t) ) {
						er.hwid = RTA_UINT8_T(erspan[LWTUNNEL_IP_OPT_ERSPAN_HWID]);
						LOG_INFO("LWTUNNEL_IP_OPT_ERSPAN_HWID: %u\n", er.hwid);
					}
				}

				snprintf(enc->data.ip.erspan_opts, sizeof(enc->data.ip.erspan_opts), "%u:%u:%u:%u", er.ver, er.index, er.dir, er.hwid);
				LOG_INFO("erspan_opts %s\n", enc->data.ip.erspan_opts);
			}
		}
	}
	return TRUE;
}

int FillEncap(Encap * enc, struct rtattr * rta)
{
	int res = TRUE;

	assert( enc != 0 && rta != 0 );
	switch( enc->type ) {
	case LWTUNNEL_ENCAP_MPLS:
		{
		struct rtattr * mpls[MPLS_IPTUNNEL_MAX+1];
		if( (res = FillAttr((struct rtattr*)RTA_DATA(rta),
			mpls,
			MPLS_IPTUNNEL_MAX,
			(short unsigned int)(~NLA_F_NESTED),
			RTA_PAYLOAD(rta),
			mplsiptunneltype)) ) {
			if( mpls[MPLS_IPTUNNEL_DST] && RTA_PAYLOAD(mpls[MPLS_IPTUNNEL_DST]) >= sizeof(uint32_t) ) {
				mpls_ntop((const struct mpls_label *)RTA_DATA(mpls[MPLS_IPTUNNEL_DST]), enc->data.mpls.dst, sizeof(enc->data.mpls.dst)-1);
				enc->data.mpls.valid.dst = 1;
				LOG_INFO("MPLS_IPTUNNEL_DST: %s\n", enc->data.mpls.dst);
			}
			if( mpls[MPLS_IPTUNNEL_TTL] && RTA_PAYLOAD(mpls[MPLS_IPTUNNEL_TTL]) >= sizeof(uint8_t) ) {
				enc->data.mpls.ttl = RTA_UINT8_T(mpls[MPLS_IPTUNNEL_TTL]);
				enc->data.mpls.valid.ttl = 1;
				LOG_INFO("MPLS_IPTUNNEL_TTL: %d\n", enc->data.mpls.ttl);
			}
		}
		}
		break;
	case LWTUNNEL_ENCAP_IP:
		res = FillIp(AF_INET, enc, rta);
		break;
	// TODO:
	case LWTUNNEL_ENCAP_ILA:
		break;
	case LWTUNNEL_ENCAP_IP6:
		res = FillIp(AF_INET6, enc, rta);
		break;
	// TODO:
	case LWTUNNEL_ENCAP_SEG6:
	case LWTUNNEL_ENCAP_BPF:
	case LWTUNNEL_ENCAP_SEG6_LOCAL:
	case LWTUNNEL_ENCAP_RPL:
	case LWTUNNEL_ENCAP_IOAM6:
	case LWTUNNEL_ENCAP_XFRM:
		break;
	};
	return res;
}

static int ProcessMsg(netlink_ctx * ctx, struct nlmsghdr * nlh)
{
	// +1 sizeof(void *) from struct UniversalRecord
	size_t real_structsize = sizeof(UniversalRecord)+sizeof(void *)*ctx->max_tbl_items;
	UniversalRecord * ur = (UniversalRecord *)(ctx->info.ptr+(ctx->currentMsg*real_structsize));
	int total_len = nlh->nlmsg_len - NLMSG_LENGTH(ctx->infosize);

	assert( ctx->currentMsg < ctx->totalmsg );
	assert( ctx->max_tbl_items >= 1 );

	ur->info = (char *)NLMSG_DATA(nlh);

	if( total_len < 0 ) {
		LOG_ERROR("nlh->nlmsg_len %d < NLMSG_LENGTH(ctx->infosize) %d\n", nlh->nlmsg_len, NLMSG_LENGTH(ctx->infosize));
		return FALSE;
	}

	memmove(&ur->nlm, nlh, sizeof(struct nlmsghdr));

	if( FillAttr((struct rtattr *)(ur->info+NLMSG_ALIGN(ctx->infosize)), ur->tb, ctx->max_tbl_items, (short unsigned int)(~NLA_F_NET_BYTEORDER), total_len, ctx->typeprint ) )
		ctx->currentMsg++;

	assert( ctx->totalmsg >= ctx->currentMsg );

	// last record is always NULL (not last in case if error FillAttr)
	ur = (UniversalRecord *)(ctx->info.ptr+(ctx->currentMsg*real_structsize));
	ur->info = 0;

	return TRUE;
}

static const void * ProcessRouteMsgs(netlink_ctx * ctx)
{
	assert( ctx->totalmsg > 0 );

	if( ctx->totalmsg > ctx->currentMsg ) {
		// last record is always NULL
		size_t total_size = sizeof(RouteRecord)*(ctx->totalmsg+1);
		ctx->info.rt_recs = (RouteRecord *)realloc(ctx->info.rt_recs, total_size);
		if( !ctx->info.rt_recs ) {
			LOG_ERROR("malloc(%u) ... error (%s)\n", total_size, errorname(errno));
			return 0;
		}
	}
	ctx->typeprint = rtatype;
	ctx->max_tbl_items = RTA_MAX;
	ctx->infosize = sizeof(struct rtmsg);
	if( EnumMsg(ctx, ProcessMsg) )
		return (const void *)ctx->info.rt_recs;
	return 0;
}

static const void * ProcessAddrMsgs(netlink_ctx * ctx)
{
	assert( ctx->totalmsg > 0 );

	if( ctx->totalmsg > ctx->currentMsg ) {
		// last record is always NULL
		size_t total_size = sizeof(AddrRecord)*(ctx->totalmsg+1);
		ctx->info.adr_recs = (AddrRecord *)realloc(ctx->info.adr_recs, total_size);
		if( !ctx->info.adr_recs ) {
			LOG_ERROR("malloc(%u) ... error (%s)\n", total_size, errorname(errno));
			return 0;
		}
	}

	ctx->typeprint = ifatype;
	ctx->max_tbl_items = IFA_MAX;
	ctx->infosize = sizeof(struct ifaddrmsg);
	if( EnumMsg(ctx, ProcessMsg) )
		return (const void *)ctx->info.adr_recs;
	return 0;
}

static const void * ProcessRuleMsgs(netlink_ctx * ctx)
{
	assert( ctx->totalmsg > 0 );

	if( ctx->totalmsg > ctx->currentMsg ) {
		// last record is always NULL
		size_t total_size = sizeof(RuleRecord)*(ctx->totalmsg+1);
		ctx->info.rule_recs = (RuleRecord *)realloc(ctx->info.rule_recs, total_size);
		if( !ctx->info.rule_recs ) {
			LOG_ERROR("malloc(%u) ... error (%s)\n", total_size, errorname(errno));
			return 0;
		}
	}

	ctx->typeprint = fratype;
	ctx->max_tbl_items = FRA_MAX;
	ctx->infosize = sizeof(struct fib_rule_hdr);
	if( EnumMsg(ctx, ProcessMsg) )
		return (const void *)ctx->info.rule_recs;
	return 0;
}

static const void * ProcessLinkMsgs(netlink_ctx * ctx)
{
	assert( ctx->totalmsg > 0 );

	if( ctx->totalmsg > ctx->currentMsg ) {
		size_t total_size = sizeof(LinkRecord)*(ctx->totalmsg+1);
		ctx->info.link_recs = (LinkRecord *)realloc(ctx->info.link_recs, total_size);
		if( !ctx->info.link_recs ) {
			LOG_ERROR("malloc(%u) ... error (%s)\n", total_size, errorname(errno));
			return 0;
		}
	}

	ctx->typeprint = iflatype;
	ctx->max_tbl_items = IFLA_MAX;
	ctx->infosize = sizeof(struct ifinfomsg);
	if( EnumMsg(ctx, ProcessMsg) )
		return (const void *)ctx->info.link_recs;
	return 0;
}

static const void * ProcessNeighborMsgs(netlink_ctx * ctx)
{
	assert( ctx->totalmsg > 0 );

	if( ctx->totalmsg > ctx->currentMsg ) {
		size_t total_size = sizeof(NeighborRecord)*(ctx->totalmsg+1);
		ctx->info.neighbor_recs = (NeighborRecord *)realloc(ctx->info.neighbor_recs, total_size);
		if( !ctx->info.neighbor_recs ) {
			LOG_ERROR("malloc(%u) ... error (%s)\n", total_size, errorname(errno));
			return 0;
		}
	}

	ctx->typeprint = ndatype;
	ctx->max_tbl_items = NDA_MAX;
	ctx->infosize = sizeof(struct ndmsg);
	if( EnumMsg(ctx, ProcessMsg) )
		return (const void *)ctx->info.neighbor_recs;
	return 0;
}

static ssize_t __netlink_recvmsg(int fd, struct msghdr *msg, int flags)
{
	ssize_t rcvsize;
	do {
		rcvsize = recvmsg(fd, msg, flags);
	} while( rcvsize < 0 && (errno == EINTR || errno == EAGAIN) );

	if( rcvsize < 0 )
		LOG_ERROR("netlink_recvmsg(flags = 0x%08X) ... error %s (%s)\n", flags, strerror(errno), errorname(errno));

	if( rcvsize == 0 ) {
		LOG_ERROR("netlink_recvmsg(flags = 0x%08X) ... EOF on netlink\n", flags);
		errno = ENODATA;
	}
	return rcvsize;
}

static ssize_t netlink_recvmsg(netlink_ctx * ctx)
{
	struct sockaddr_nl addr;
	struct iovec iov = {
		.iov_base	= ctx->buf + ctx->offset,
		.iov_len	= ctx->rcvbufsize - ctx->offset,
	};
	struct msghdr msg = {
		.msg_name	= &addr,
		.msg_namelen	= sizeof(struct sockaddr_nl),
		.msg_iov	= &iov,
		.msg_iovlen	= 1,
		.msg_control	= NULL,
		.msg_controllen	= 0,
		.msg_flags	= 0,
	};

	ssize_t rcvsize;

	while( (rcvsize = __netlink_recvmsg(ctx->netlink_socket, &msg, MSG_PEEK | MSG_TRUNC)) > 0 ) {

		// tune buffer
		size_t need_size = (size_t)rcvsize + ctx->offset;
		if( need_size > ctx->rcvbufsize ) {
			char * buf = (char *)realloc(ctx->buf, ctx->sndbufsize > need_size ? ctx->sndbufsize:need_size);
			if( !buf ) {
				LOG_ERROR("malloc(%u) ... error (%s)\n", need_size, errorname(errno));
				return -errno;
			}
			ctx->buf = buf;
			ctx->rcvbufsize = need_size;
			iov.iov_base = ctx->buf + ctx->offset;
			iov.iov_len = ctx->rcvbufsize - ctx->offset;

			if( setsockopt(ctx->netlink_socket,
				SOL_SOCKET,
				SO_RCVBUF,
				&ctx->rcvbufsize,
				sizeof(ctx->rcvbufsize)) < 0 ) {
				LOG_ERROR("setsockopt(SOL_SOCKET, SO_RCVBUF %u) ... error (%s)\n", \
						ctx->rcvbufsize, errorname(errno));
				return -errno;
			}
			continue;
		}

		// recv data
		rcvsize = __netlink_recvmsg(ctx->netlink_socket, &msg, 0);
		if( rcvsize <= 0 )
			break;

		LOG_INFO("recvmsg() ... size:             %d (%d)\n", rcvsize, NLMSG_ALIGN(sizeof(struct nlmsghdr)));
		LOG_INFO("recvmsg() ... msg.msg_flags:    0x%08X\n", msg.msg_flags);

		// check data
		// strict condition with asserts
		assert( msg.msg_namelen == sizeof(struct sockaddr_nl) );

		errno = 0;
		if( msg.msg_namelen != sizeof(struct sockaddr_nl) ) {
			LOG_ERROR("unsupported AF_NETLINK socket size\n");
			errno = EINVAL;
			break;
		}

		// not strict condition
		if( (msg.msg_flags & MSG_TRUNC) != 0 ) {
			LOG_ERROR("truncate date\n");
			errno = EINVAL;
			break;
		}

		return rcvsize;
	}
	return -1;
}

const void * GetInfo(netlink_ctx * ctx, const void * (* fn)(netlink_ctx * ctx))
{
	const void * info = 0;

	assert( fn != 0 );
	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->buf != 0 );

	if( send(ctx->netlink_socket, ctx->buf, ctx->nlh->nlmsg_len, 0) < 0) {
		LOG_ERROR("send(%u) ... error (%s)\n", ctx->nlh->nlmsg_type, errorname(errno));
		return 0;
	}

	ctx->totalmsg = 0;
	ctx->currentMsg = 0;
	ctx->rcvsize = 0;
	ctx->offset = 0;

	while( (ctx->rcvsize = netlink_recvmsg(ctx)) > 0 ) {
		assert( ctx->rcvsize > 0 && (size_t)ctx->rcvsize <= ctx->rcvbufsize );
		LOG_INFO("1. ctx->totalmsg %d\n", ctx->totalmsg);
		if( EnumMsg(ctx, IncrementTotalMsg) ) {
			LOG_INFO("2. ctx->totalmsg %d\n", ctx->totalmsg);
			if( ctx->totalmsg ) {
				info = fn(ctx);
				ctx->offset += (size_t)ctx->rcvsize;
				assert( ctx->rcvbufsize >= ctx->offset );
			}
		} else
			break;
	}

	LOG_INFO("ctx->offset = %u\n", ctx->offset);
	return info;
}

const RouteRecord * GetRoutes(void * nl, int family)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;
	struct rtmsg * rtm = (struct rtmsg *)NLMSG_DATA(ctx->buf);

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->nlh != 0 );

	memset( ctx->buf, 0, NLMSG_ALIGN(sizeof(struct nlmsghdr)) );
	ctx->nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	ctx->nlh->nlmsg_type = RTM_GETROUTE;
	ctx->nlh->nlmsg_seq = ++ctx->sequence_number;
	ctx->nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

	rtm->rtm_family = family;
	return (const RouteRecord *)GetInfo(ctx, ProcessRouteMsgs);
}

const LinkRecord * GetLinks(void * nl)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;
	struct rtgenmsg *rt = (struct rtgenmsg *)NLMSG_DATA(ctx->buf);

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->nlh != 0 );

	memset( ctx->buf, 0, NLMSG_ALIGN(sizeof(struct nlmsghdr)) );
	ctx->nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
	ctx->nlh->nlmsg_type = RTM_GETLINK;
	ctx->nlh->nlmsg_seq = ++ctx->sequence_number;
	ctx->nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

	rt->rtgen_family = AF_PACKET;
	return (const LinkRecord *)GetInfo(ctx, ProcessLinkMsgs);
}

const AddrRecord * GetAddr(void *nl, int family)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;
	struct ifaddrmsg *ifam = (struct ifaddrmsg *)NLMSG_DATA(ctx->buf);

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->nlh != 0 );

	memset( ctx->buf, 0, NLMSG_ALIGN(sizeof(struct nlmsghdr)) );
	ctx->nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	ctx->nlh->nlmsg_type = RTM_GETADDR;
	ctx->nlh->nlmsg_seq = ++ctx->sequence_number;
	ctx->nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	ifam->ifa_family = family;
	return (const AddrRecord *)GetInfo(ctx, ProcessAddrMsgs);
}

const RuleRecord * GetRules(void *nl, int family)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;
	struct fib_rule_hdr *frh =(struct fib_rule_hdr *)NLMSG_DATA(ctx->buf);

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->nlh != 0 );

	memset( ctx->buf, 0, NLMSG_ALIGN(sizeof(struct nlmsghdr)) );
	ctx->nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct fib_rule_hdr));
	ctx->nlh->nlmsg_type = RTM_GETRULE;
	ctx->nlh->nlmsg_seq = ++ctx->sequence_number;
	ctx->nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	frh->family = family;
	return (const RuleRecord *)GetInfo(ctx, ProcessRuleMsgs);
}

const NeighborRecord * GetNeighbors(void *nl, int family, int ndm_flags)
{
	netlink_ctx * ctx = (netlink_ctx *)nl;
	struct ndmsg *ndm =(struct ndmsg *)NLMSG_DATA(ctx->buf);

	assert( ctx != 0 );
	assert( ctx->netlink_socket >= 0 );
	assert( ctx->nlh != 0 );

	memset( ctx->buf, 0, NLMSG_ALIGN(sizeof(struct nlmsghdr)) );
	ctx->nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	ctx->nlh->nlmsg_type = RTM_GETNEIGH;
	ctx->nlh->nlmsg_seq = ++ctx->sequence_number;
	ctx->nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

	ndm->ndm_family = family;
	ndm->ndm_flags = ndm_flags;

	return (const NeighborRecord *)GetInfo(ctx, ProcessNeighborMsgs);
}
#endif // #if !defined(__APPLE__) && !defined(__FreeBSD__)

#ifdef MAIN_COMMON_NETLINK

int main(int argc, char * argv[])
{

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	void * netlink = OpenNetlink();
	if(netlink) {
		const LinkRecord * lr = GetLinks(netlink);
		const RouteRecord * rr = GetRoutes(netlink, AF_INET);
		const AddrRecord * ar = 0;
		const RuleRecord * r = 0;
		const NeighborRecord * nb = 0;
		rr = GetRoutes(netlink, AF_INET6);
		ar = GetAddr(netlink, AF_UNSPEC);
		//ar = GetAddr(netlink, AF_PACKET);
		//ar = GetAddr(netlink, AF_INET);
		//ar = GetAddr(netlink, AF_INET6);
		r = GetRules(netlink, AF_UNSPEC);
		nb = GetNeighbors(netlink, AF_UNSPEC, 0);
		nb = GetNeighbors(netlink, AF_UNSPEC, NTF_PROXY);
		CloseNetlink(netlink);
	}
#endif
	return 0;
}

#endif //MAIN_COMMON_NETLINK
