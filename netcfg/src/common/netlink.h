#ifndef __COMMON_NETLINK_H__
#define __COMMON_NETLINK_H__

#include <stdint.h>
#include <stddef.h>
#include <net/if.h>
#include <arpa/inet.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <linux/fib_rules.h>
#include <linux/neighbour.h>

#include <linux/ila.h>
#include <linux/mpls.h>
#include <linux/lwtunnel.h>
#include <linux/mpls_iptunnel.h>
#include <linux/seg6.h>
#include <linux/seg6_iptunnel.h>
#include <linux/seg6_hmac.h>
#include <linux/seg6_local.h>
#include <linux/if_tunnel.h>

//#include <linux/if_link.h>
//#include <linux/if_addr.h>
#include <linux/icmpv6.h>
#include <sys/socket.h>
#include <assert.h>


#ifndef RTM_GETSTATS
#define RTM_GETSTATS 94
#endif

#ifndef RTM_SETSTATS
#define RTM_SETSTATS 95
#endif

#ifndef	RTM_NEWCACHEREPORT
#define RTM_NEWCACHEREPORT 96
#endif

#ifndef	RTM_NEWCHAIN
#define RTM_NEWCHAIN 100
#endif
#ifndef	RTM_DELCHAIN
#define RTM_DELCHAIN 101
#endif
#ifndef	RTM_GETCHAIN
#define RTM_GETCHAIN 102
#endif
#ifndef	RTM_NEWNEXTHOP
#define RTM_NEWNEXTHOP	104
#endif
#ifndef	RTM_DELNEXTHOP
#define RTM_DELNEXTHOP 105
#endif
#ifndef	RTM_GETNEXTHOP
#define RTM_GETNEXTHOP 106
#endif
#ifndef	RTM_NEWLINKPROP
#define RTM_NEWLINKPROP	108
#endif
#ifndef	RTM_DELLINKPROP
#define RTM_DELLINKPROP 109
#endif
#ifndef	RTM_GETLINKPROP
#define RTM_GETLINKPROP 110
#endif
#ifndef	RTM_NEWVLAN
#define RTM_NEWVLAN 112
#endif
#ifndef	RTM_DELVLAN
#define RTM_DELVLAN	113
#endif
#ifndef	RTM_GETVLAN
#define RTM_GETVLAN	114
#endif
#ifndef	RTM_NEWNEXTHOPBUCKET
#define RTM_NEWNEXTHOPBUCKET 116
#endif
#ifndef	RTM_DELNEXTHOPBUCKET
#define RTM_DELNEXTHOPBUCKET 117
#endif
#ifndef	RTM_GETNEXTHOPBUCKET
#define RTM_GETNEXTHOPBUCKET 118
#endif
#ifndef	RTM_NEWTUNNEL
#define RTM_NEWTUNNEL 120
#endif
#ifndef	RTM_DELTUNNEL
#define RTM_DELTUNNEL 121
#endif
#ifndef	RTM_GETTUNNEL
#define RTM_GETTUNNEL 122
#endif

#ifndef ICMPV6_ROUTER_PREF_LOW
#define ICMPV6_ROUTER_PREF_LOW		0x3
#define ICMPV6_ROUTER_PREF_MEDIUM	0x0
#define ICMPV6_ROUTER_PREF_HIGH		0x1
#define ICMPV6_ROUTER_PREF_INVALID	0x2
#endif

#ifndef RT_TABLE_COMPAT
#define RT_TABLE_COMPAT 252
#define RT_TABLE_DEFAULT 253
#define RT_TABLE_MAIN 254
#define RT_TABLE_LOCAL 255
#endif

#ifndef RTPROT_XORP
#define RTPROT_XORP		14	/* XORP */
#endif

#ifndef RTPROT_NTK
#define RTPROT_NTK		15	/* Netsukuku */
#endif

#ifndef RTPROT_DHCP
#define RTPROT_DHCP		16	/* DHCP client */
#endif

#ifndef RTPROT_MROUTED
#define RTPROT_MROUTED		17	/* Multicast daemon */
#endif

#ifndef RTPROT_BABEL
#define RTPROT_BABEL		42	/* Babel daemon */
#endif

#ifndef NLM_F_DUMP_INTR
#define NLM_F_DUMP_INTR		0x10	/* Dump was inconsistent due to sequence change */
#endif

#ifndef NLA_F_NET_BYTEORDER
#define NLA_F_NET_BYTEORDER	(1 << 14)
#endif

#ifndef NLMSG_MIN_TYPE
#define NLMSG_MIN_TYPE		0x10	/* < 0x10: reserved control messages */
#endif

#ifndef IFLA_MIN_MTU
#define IFLA_MIN_MTU 50
#endif

#ifndef IFLA_MAX_MTU
#define IFLA_MAX_MTU 51
#endif

#ifndef IFLA_PROP_LIST
#define IFLA_PROP_LIST 52
#endif

#ifndef IFLA_ALT_IFNAME
#define IFLA_ALT_IFNAME 53
#endif

#ifndef IFLA_PERM_ADDRESS
#define IFLA_PERM_ADDRESS 54
#endif

#ifndef IFLA_PROTO_DOWN_REASON
#define IFLA_PROTO_DOWN_REASON 55
#endif

#ifndef	IFLA_PARENT_DEV_NAME
#define	IFLA_PARENT_DEV_NAME 56
#endif

#ifndef	IFLA_PARENT_DEV_BUS_NAME
#define	IFLA_PARENT_DEV_BUS_NAME 57
#endif

#ifndef	IFLA_GRO_MAX_SIZE
#define	IFLA_GRO_MAX_SIZE 58
#endif

#ifndef	IFLA_TSO_MAX_SIZE
#define	IFLA_TSO_MAX_SIZE 59
#endif

#ifndef	IFLA_TSO_MAX_SEGS
#define	IFLA_TSO_MAX_SEGS 60
#endif

#ifndef	IFLA_ALLMULTI
#define	IFLA_ALLMULTI 61
#endif

#ifndef	IFLA_DEVLINK_PORT
#define	IFLA_DEVLINK_PORT 62
#endif

#ifndef	IFLA_GSO_IPV4_MAX_SIZE
#define	IFLA_GSO_IPV4_MAX_SIZE 63
#endif

#ifndef IFLA_GRO_IPV4_MAX_SIZE
#define	IFLA_GRO_IPV4_MAX_SIZE 64
#endif

#undef IFLA_MAX
#define IFLA_MAX 64


#ifndef IFA_PROTO
#define IFA_PROTO 11
#undef IFA_MAX
#define IFA_MAX 11
#ifndef IFA_TARGET_NETNSID
#define IFA_TARGET_NETNSID 10
#ifndef IFA_RT_PRIORITY
#define IFA_RT_PRIORITY 9
#endif
#endif
#endif

#ifndef ETH_ALEN
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#endif


#ifndef LWTUNNEL_ENCAP_XFRM
#define LWTUNNEL_ENCAP_XFRM 10
#undef LWTUNNEL_ENCAP_MAX
#define LWTUNNEL_ENCAP_MAX 10
#ifndef LWTUNNEL_ENCAP_IOAM6
#define LWTUNNEL_ENCAP_IOAM6 9
#ifndef LWTUNNEL_ENCAP_RPL
#define LWTUNNEL_ENCAP_RPL 8
#endif
#endif
#endif

#ifndef LWTUNNEL_IP_OPTS
#define LWTUNNEL_IP_OPTS 8
#undef LWTUNNEL_IP_MAX
#define LWTUNNEL_IP_MAX 8
#endif

#ifndef LWTUNNEL_IP6_OPTS
#define LWTUNNEL_IP6_OPTS 8
#undef LWTUNNEL_IP6_MAX
#define LWTUNNEL_IP6_MAX 8
#endif

#ifndef LWTUNNEL_IP_OPTS_ERSPAN
#define LWTUNNEL_IP_OPTS_ERSPAN 3
#undef LWTUNNEL_IP_OPTS_MAX
#define LWTUNNEL_IP_OPTS_MAX 3
#ifndef LWTUNNEL_IP_OPTS_VXLAN
#define LWTUNNEL_IP_OPTS_VXLAN 2
#ifndef LWTUNNEL_IP_OPTS_GENEVE
#define LWTUNNEL_IP_OPTS_GENEVE 1
#ifndef LWTUNNEL_IP_OPTS_UNSPEC
#define LWTUNNEL_IP_OPTS_UNSPEC 0
#endif
#endif
#endif
#endif


#ifndef LWTUNNEL_IP_OPT_GENEVE_DATA
#define LWTUNNEL_IP_OPT_GENEVE_DATA 3
#undef LWTUNNEL_IP_OPT_GENEVE_MAX
#define LWTUNNEL_IP_OPT_GENEVE_MAX 3
#ifndef LWTUNNEL_IP_OPT_GENEVE_TYPE
#define LWTUNNEL_IP_OPT_GENEVE_TYPE 2
#ifndef LWTUNNEL_IP_OPT_GENEVE_CLASS
#define LWTUNNEL_IP_OPT_GENEVE_CLASS 1
#ifndef LWTUNNEL_IP_OPT_GENEVE_UNSPEC
#define LWTUNNEL_IP_OPT_GENEVE_UNSPEC 0
#endif
#endif
#endif
#endif

#ifndef LWTUNNEL_IP_OPT_VXLAN_GBP
#define LWTUNNEL_IP_OPT_VXLAN_GBP 1
#undef LWTUNNEL_IP_OPT_VXLAN_MAX
#define LWTUNNEL_IP_OPT_VXLAN_MAX 1
#ifndef LWTUNNEL_IP_OPT_VXLAN_UNSPEC
#define LWTUNNEL_IP_OPT_VXLAN_UNSPEC 0
#endif
#endif

#ifndef LWTUNNEL_IP_OPT_ERSPAN_HWID
#define LWTUNNEL_IP_OPT_ERSPAN_HWID 4
#undef LWTUNNEL_IP_OPT_ERSPAN_MAX
#define LWTUNNEL_IP_OPT_ERSPAN_MAX 4
#ifndef LWTUNNEL_IP_OPT_ERSPAN_DIR
#define LWTUNNEL_IP_OPT_ERSPAN_DIR 3
#ifndef LWTUNNEL_IP_OPT_ERSPAN_INDEX
#define LWTUNNEL_IP_OPT_ERSPAN_INDEX 2
#ifndef LWTUNNEL_IP_OPT_ERSPAN_VER
#define LWTUNNEL_IP_OPT_ERSPAN_VER 1
#ifndef LWTUNNEL_IP_OPT_ERSPAN_UNSPEC
#define LWTUNNEL_IP_OPT_ERSPAN_UNSPEC 0
#endif
#endif
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char * nlmsgtype(uint16_t type);

#ifndef XDP_ATTACHED_MULTI
#define	XDP_ATTACHED_MULTI 4
#ifndef XDP_ATTACHED_HW
#define	XDP_ATTACHED_HW 3
#ifndef XDP_ATTACHED_SKB
#define	XDP_ATTACHED_SKB 2
#ifndef XDP_ATTACHED_DRV
#define	XDP_ATTACHED_DRV 1
#ifndef XDP_ATTACHED_NONE
#define	XDP_ATTACHED_NONE 0
#endif
#endif
#endif
#endif
#endif

#ifndef IFLA_XDP_EXPECTED_FD
#define IFLA_XDP_EXPECTED_FD 8
#undef IFLA_XDP_MAX
#define IFLA_XDP_MAX 8
#ifndef IFLA_XDP_HW_PROG_ID
#define IFLA_XDP_HW_PROG_ID 7
#ifndef IFLA_XDP_SKB_PROG_ID
#define IFLA_XDP_SKB_PROG_ID 6
#ifndef IFLA_XDP_DRV_PROG_ID
#define IFLA_XDP_DRV_PROG_ID 5
#ifndef IFLA_XDP_PROG_ID
#define IFLA_XDP_PROG_ID 4
#ifndef IFLA_XDP_FLAGS
#define IFLA_XDP_FLAGS 3
#ifndef IFLA_XDP_ATTACHED
#define IFLA_XDP_ATTACHED 2
#ifndef IFLA_XDP_FD
#define IFLA_XDP_FD 1
#ifndef IFLA_XDP_UNSPEC
#define IFLA_XDP_UNSPEC 0
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif


#ifndef FRA_DPORT_RANGE
#define FRA_DPORT_RANGE 24 
#undef FRA_MAX
#define FRA_MAX 24
#ifndef FRA_SPORT_RANGE
#define FRA_SPORT_RANGE 23 
#ifndef FRA_IP_PROTO
#define	FRA_IP_PROTO 22	
#ifndef FRA_PROTOCOL
#define	FRA_PROTOCOL 21  
#ifndef FRA_UID_RANGE
#define FRA_UID_RANGE 20
#ifndef FRA_L3MDEV
#define FRA_L3MDEV 19 
#ifndef FRA_PAD
#define FRA_PAD 18
#endif
#endif
#endif
#endif
#endif
#endif
#endif


#ifndef FRA_UID_START
#define FRA_UID_START FRA_PAD
#endif
#ifndef FRA_UID_END
#define FRA_UID_END FRA_L3MDEV
#endif


#ifndef NDA_NDM_FLAGS_MASK
#define NDA_NDM_FLAGS_MASK 17
#undef NDA_MAX
#define NDA_MAX 17
#ifndef NDA_NDM_STATE_MASK
#define NDA_NDM_STATE_MASK 16
#ifndef NDA_FLAGS_EXT
#define NDA_FLAGS_EXT 15
#ifndef NDA_FDB_EXT_ATTRS
#define NDA_FDB_EXT_ATTRS 14
#ifndef NDA_NH_ID
#define NDA_NH_ID 13
#ifndef NDA_PROTOCOL
#define NDA_PROTOCOL 12  /* Originator of entry */
#ifndef NDA_SRC_VNI
#define NDA_SRC_VNI 11
#ifndef NDA_LINK_NETNSID
#define NDA_LINK_NETNSID 10
#ifndef NDA_MASTER
#define NDA_MASTER 9
#ifndef NDA_IFINDEX
#define NDA_IFINDEX 8
#ifndef NDA_VNI
#define NDA_VNI 7
#ifndef NDA_PORT
#define NDA_PORT 6
#ifndef NDA_VLAN
#define NDA_VLAN 5
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#ifndef RTA_NH_ID
#define RTA_NH_ID 30
#undef RTA_MAX
#define RTA_MAX 30
#ifndef RTA_DPORT
#define RTA_DPORT 29
#ifndef RTA_SPORT
#define RTA_SPORT 28
#ifndef RTA_IP_PROTO
#define RTA_IP_PROTO 27
#endif
#endif
#endif
#endif

typedef struct {
	struct nlmsghdr nlm;
	struct rtmsg *rt;
	struct rtattr *tb[RTA_MAX+1];
} RouteRecord;

typedef struct {
	struct nlmsghdr nlm;
	struct ifinfomsg *ifm;
	struct rtattr *tb[IFLA_MAX+1];
} LinkRecord;

typedef struct {
	struct nlmsghdr nlm;
	struct ifaddrmsg *ifam;
	struct rtattr *tb[IFA_MAX+1];
} AddrRecord;

typedef struct {
	struct nlmsghdr nlm;
	struct fib_rule_hdr *frh;
	struct rtattr *tb[FRA_MAX+1];
} RuleRecord;

typedef struct {
	struct nlmsghdr nlm;
	struct ndmsg * ndm;
	struct rtattr *tb[NDA_MAX+1];
} NeighborRecord;


#define RTA_UINT8_T(rta) *((uint8_t *)RTA_DATA(rta))
#define RTA_UINT16_T(rta) *((uint16_t *)RTA_DATA(rta))
#define RTA_UINT32_T(rta) *((uint32_t *)RTA_DATA(rta))
#define RTA_INT32_T(rta) *((int32_t *)RTA_DATA(rta))
#define RTA_UINT64_T(rta) *((uint64_t *)RTA_DATA(rta))
#define RTA_INT64_T(rta) *((int64_t *)RTA_DATA(rta))

void * OpenNetlink(void);
void CloseNetlink(void * nl);
const RouteRecord * GetRoutes(void * nl, int family);
const LinkRecord * GetLinks(void * nl);
const AddrRecord * GetAddr(void *nl, int family);
const RuleRecord * GetRules(void *nl, int family);
const NeighborRecord * GetNeighbors(void *nl, int family, int ndm_flags);
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
// FillAttr flags mask
//#define NLA_F_NESTED		(1 << 15)
//#define NLA_F_NET_BYTEORDER	(1 << 14)
//#define NLA_TYPE_MASK		~(NLA_F_NESTED | NLA_F_NET_BYTEORDER)

const char * rtatype( uint16_t type ); // RTA_
const char * rtaxtype( uint16_t type ); // RTAX_
const char * ifatype( uint16_t type ); // IFA_
const char * iflainfotype( uint16_t type ); // IFLA_INFO_
const char * iflaxdptype( uint16_t type ); //IFLA_XDP_
const char * iflatype( uint16_t type ); //IFLA_ 
const char * fratype( uint16_t type ); // FRA_
const char * iflavlantype( uint16_t type ); // IFLA_VLAN_
const char * iflainet6type( uint16_t type ); // IFLA_INET6_
const char * iptunneloptstype( uint16_t type ); // LWTUNNEL_IP_OPTS_
const char * iptunneloptsgenevetype( uint16_t type ); // LWTUNNEL_IP_OPT_GENEVE_
const char * iptunneloptsvxlantype( uint16_t type ); // LWTUNNEL_IP_OPT_VXLAN_
const char * iptunneloptserspantype( uint16_t type ); // LWTUNNEL_IP_OPT_ERSPAN_

#ifndef TUNNEL_CSUM
#define TUNNEL_CSUM		__cpu_to_be16(0x01)
#define TUNNEL_ROUTING		__cpu_to_be16(0x02)
#define TUNNEL_KEY		__cpu_to_be16(0x04)
#define TUNNEL_SEQ		__cpu_to_be16(0x08)
#define TUNNEL_STRICT		__cpu_to_be16(0x10)
#define TUNNEL_REC		__cpu_to_be16(0x20)
#define TUNNEL_VERSION		__cpu_to_be16(0x40)
#define TUNNEL_NO_KEY		__cpu_to_be16(0x80)
#define TUNNEL_DONT_FRAGMENT    __cpu_to_be16(0x0100)
#define TUNNEL_OAM		__cpu_to_be16(0x0200)
#define TUNNEL_CRIT_OPT		__cpu_to_be16(0x0400)
#define TUNNEL_GENEVE_OPT	__cpu_to_be16(0x0800)
#define TUNNEL_VXLAN_OPT	__cpu_to_be16(0x1000)
#define TUNNEL_NOCACHE		__cpu_to_be16(0x2000)
#define TUNNEL_ERSPAN_OPT	__cpu_to_be16(0x4000)
#endif

#ifndef TUNNEL_GTP_OPT
#define TUNNEL_GTP_OPT		__cpu_to_be16(0x8000)
#endif

const char * tunnelflagsname(uint16_t flags); // TUNNEL_

/* Values of protocol >= RTPROT_STATIC are not interpreted by kernel;
   they are just passed from user and back as is.
   It will be used by hypothetical multiple routing daemons.
   Note that protocol values should be standardized in order to
   avoid conflicts.
 */

#ifndef RTPROT_KEEPALIVED
#define RTPROT_KEEPALIVED	18	/* Keepalived daemon */
#endif
#ifndef RTPROT_OPENR
#define RTPROT_OPENR		99	/* Open Routing (Open/R) Routes */
#endif
#ifndef RTPROT_BGP
#define RTPROT_BGP		186	/* BGP Routes */
#endif
#ifndef RTPROT_EIGRP
#define RTPROT_EIGRP		192	/* EIGRP Routes */
#endif
#ifndef RTPROT_RIP
#define RTPROT_RIP		189	/* RIP Routes */
#endif
#ifndef RTPROT_OSPF
#define RTPROT_OSPF		188	/* OSPF Routes */
#endif
#ifndef RTPROT_ISIS
#define RTPROT_ISIS		187	/* ISIS Routes */
#endif

const char * rtprotocoltype( uint8_t type ); // RTPROT_

//RT_SCOPE_UNIVERSE (0): глобальный адрес
//RT_SCOPE_SITE (200): адрес сайта
//RT_SCOPE_LINK (253): адрес сетевого интерфейса
//RT_SCOPE_HOST (254): адрес хоста
//RT_SCOPE_NOWHERE (255): неизвестный адрес

const char * rtscopetype(uint8_t scope); //RT_SCOPE_
const char * rticmp6pref(uint8_t pref); // ICMPV6_ROUTER_PREF_

int FillAttr(struct rtattr *rta, struct rtattr **tb, unsigned short max, unsigned short flagsmask, int len, const char * (*typeprint)(uint16_t type));

// Maximal IPv6 address length see https://dirask.com/posts/Maximal-IPv6-address-length-joz4Np
#ifndef MAX_IP_SIZE
#define MAX_IP_SIZE sizeof("ffff:ffff:ffff:ffff:ffff:ffff:192.168.100.100%wlxd123456789ab")
#endif
#ifndef MAX_INTERFACE_NAME_LEN
#define MAX_INTERFACE_NAME_LEN 16
#endif


#define MAX_GENEVE_OPTS_STRING 100
#define MAX_ERSPAN_OPTS_STRING sizeof("256:4294967296:256:256")


#define MAX_MPLS_LABELS_LEN 64
#define MAX_GENEVE_OPTS 10
#define MAX_GENEVE_OPTS_DATA 30

typedef struct {
	uint16_t cls;
	uint8_t type;
	size_t size;
	uint8_t data[MAX_GENEVE_OPTS_DATA];
} Geneve;

typedef struct {
	uint16_t type;
	union {
		struct {
			struct {
				unsigned char ttl:1;
				unsigned char dst:1;
			} valid;
			uint8_t ttl;
			char dst[MAX_MPLS_LABELS_LEN+1];
		} mpls;
		struct {
			struct {
				unsigned int id:1;
				unsigned int ttl:1;
				unsigned int tos:1;

				unsigned int hoplimit:1;
				unsigned int tc:1;

				unsigned int src:1;
				unsigned int dst:1;
				unsigned int flags:1;

				// VXLAN (Virtual Extensible LAN) - протокол туннелирования сетевого уровня, который используется для передачи кадров Ethernet через IP-сети.
				unsigned int vxlan:1;
				unsigned int vxlan_gbp:1;

				// GENEVE - это протокол туннелирования сетевого уровня, который был разработан для использования в виртуальных сетях (VXLAN). 
				// Он используется для передачи кадров Ethernet через IP-сети. Формат CLASS:TYPE:DATA, где CLASS представлено в виде 16-битного шестнадцатеричного значения,
				// TYPE - 8-битного шестнадцатеричного значения, а DATA - шестнадцатеричного значения переменной длины.
				// Кроме того, несколько опций могут быть перечислены с использованием разделителя запятой
				unsigned int geneve:1;

				// ERSPAN (Encapsulated Remote Switched Port Analyzer) - это механизм мониторинга трафика в сети. 
				// Он позволяет скопировать трафик с одного или нескольких портов коммутатора и перенаправить его на другой порт для анализа.
				unsigned int erspan:1;
			} valid;
			uint64_t id;
			char dst[MAX_IP_SIZE];
			char src[MAX_IP_SIZE];
			uint8_t family;
			uint8_t ttl;
			uint8_t tos;

			uint8_t hoplimit;
			uint8_t tc;

			uint16_t flags;
			uint32_t vxlan_gbp;

			char geneve_opts[MAX_GENEVE_OPTS_STRING];
			char erspan_opts[MAX_ERSPAN_OPTS_STRING];

		} ip;
	} data;
} Encap;

int FillEncap(Encap * enc, struct rtattr * rta);

const char * rttype(unsigned char rtm_type);
const char * rtruletable(uint32_t table);
const char * fractionrule( uint8_t action ); // FR_ACT_ + RTN_
const char * fibruleflagsname(uint32_t flags); // FIB_RULE_
const char * ndmsgstate(uint16_t state); // NUD_

const char * lwtunnelencaptype(uint16_t type); // LWTUNNEL_ENCAP_
const char * mplsiptunneltype(uint16_t type); //MPLS_IPTUNNEL_

void mpls_ntop(const struct mpls_label *addr, char * buf, size_t destlen);

#ifndef RTNH_F_TRAP
#define RTNH_F_TRAP 64
#endif

#ifndef RTM_F_OFFLOAD
#define RTM_F_OFFLOAD		0x4000	/* route is offloaded */
#endif
#ifndef RTM_F_TRAP
#define RTM_F_TRAP		0x8000	/* route is trapping packets */
#endif
#ifndef RTM_F_OFFLOAD_FAILED
#define RTM_F_OFFLOAD_FAILED	0x20000000 /* route offload failed, this value*/
#endif


const char * rtnhflagsname(uint8_t flags); // RTNH_F_

#ifndef NTF_STICKY
#define NTF_STICKY	(1 << 6)
#endif

#ifndef NTF_ROUTER
#define NTF_ROUTER	(1 << 7)
#endif

const char * ndmsgflagsname(uint8_t flags); // NTF_

//NTF_PROXY // хост отвечает на запрос адреса своим MAC (как ARP сервер)

#ifndef NTF_EXT_MANAGED
#define NTF_EXT_MANAGED		(1 << 0)
#endif

#ifndef NTF_EXT_LOCKED
#define NTF_EXT_LOCKED		(1 << 1)
#endif

const char * ndmsgflags_extname(uint32_t flags); // NTF_EXT_

const char * xdpattachedtype(uint8_t type); // XDP_ATTACHED_

#ifdef __cplusplus
}
#endif

#endif

#endif // __COMMON_NETLINK_H__
