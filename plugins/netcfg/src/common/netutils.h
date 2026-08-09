#ifndef __COMMON_NETUTILS_H__
#define __COMMON_NETUTILS_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <unistd.h>

// Maximal IPv6 address length see https://dirask.com/posts/Maximal-IPv6-address-length-joz4Np
#define MAX_IP_SIZE sizeof("ffff:ffff:ffff:ffff:ffff:ffff:192.168.100.100%wlxd123456789ab")
#define MAX_INTERFACE_NAME_LEN 16

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

#ifndef ntohll
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#if !defined(__APPLE__) && !defined(__FreeBSD__)

#include <linux/if_vlan.h>

#ifndef IFF_DYNAMIC
#define IFF_DYNAMIC (1<<15)
#else
#include <linux/if_link.h>
#include <linux/if_addr.h>
#endif

#ifndef IFF_UP
#define IFF_UP 0x1
#endif
#ifndef IFF_BROADCAST
#define IFF_BROADCAST 0x2
#endif
#ifndef IFF_DEBUG
#define IFF_DEBUG 0x4
#endif
#ifndef IFF_LOOPBACK
#define IFF_LOOPBACK 0x8
#endif
#ifndef IFF_POINTOPOINT
#define IFF_POINTOPOINT 0x10
#endif 
#ifndef IFF_NOTRAILERS
#define IFF_NOTRAILERS 0x20
#endif 
#ifndef IFF_RUNNING
#define IFF_RUNNING 0x40
#endif 
#ifndef IFF_NOARP
#define IFF_NOARP 0x80
#endif 
#ifndef IFF_PROMISC
#define IFF_PROMISC 0x100
#endif 
#ifndef IFF_ALLMULTI
#define IFF_ALLMULTI 0x200
#endif 
#ifndef IFF_MASTER
#define IFF_MASTER 0x400
#endif 
#ifndef IFF_SLAVE
#define IFF_SLAVE 0x800
#endif 
#ifndef IFF_MULTICAST
#define IFF_MULTICAST 0x1000
#endif 
#ifndef IFF_PORTSEL
#define IFF_PORTSEL 0x2000
#endif 
#ifndef IFF_AUTOMEDIA
#define IFF_AUTOMEDIA 0x4000
#endif 
#ifndef IFF_DYNAMIC
#define IFF_DYNAMIC 0x8000
#endif 


#ifndef AF_RDS
#define AF_RDS		21	/* RDS sockets 			*/
#define AF_LLC		26	/* Linux LLC			*/
#define AF_IB		27	/* Native InfiniBand address	*/
#define AF_MPLS		28	/* MPLS */
#define AF_CAN		29	/* Controller Area Network      */
#define AF_TIPC		30	/* TIPC sockets			*/
#define AF_IUCV		32	/* IUCV sockets			*/
#define AF_RXRPC	33	/* RxRPC sockets 		*/
#define AF_ISDN		34	/* mISDN sockets 		*/
#define AF_PHONET	35	/* Phonet sockets		*/
#define AF_IEEE802154	36	/* IEEE802154 sockets		*/
#define AF_CAIF		37	/* CAIF sockets			*/
#define AF_ALG		38	/* Algorithm sockets		*/
#define AF_NFC		39	/* NFC sockets			*/
#define AF_VSOCK	40	/* vSockets			*/
#define AF_KCM		41	/* Kernel Connection Multiplexor*/
#define AF_QIPCRTR	42	/* Qualcomm IPC Router          */
#define AF_SMC		43	/* smc sockets: reserve number for
				 * PF_SMC protocol family that
				 * reuses AF_INET address family*/
#endif

#ifndef ARPHRD_HWX25
#define ARPHRD_HWX25	272		/* Boards with X.25 in firmware	*/
#endif
#ifndef ARPHRD_RAWIP
#define ARPHRD_RAWIP    519		/* Raw IP                       */
#endif
#ifndef ARPHRD_IEEE80211_PRISM
#define ARPHRD_IEEE80211_PRISM 802	/* IEEE 802.11 + Prism2 header  */
#endif
#ifndef ARPHRD_IEEE80211_RADIOTAP
#define ARPHRD_IEEE80211_RADIOTAP 803	/* IEEE 802.11 + radiotap header */
#endif
#ifndef ARPHRD_IEEE802154
#define ARPHRD_IEEE802154	  804
#endif
#ifndef ARPHRD_IEEE802154_MONITOR
#define ARPHRD_IEEE802154_MONITOR 805	/* IEEE 802.15.4 network monitor */
#endif
#ifndef ARPHRD_VOID
#define ARPHRD_VOID	  0xFFFF	/* Void type, nothing is known */
#endif

#define VLAN_FLAG_REORDER_HDR		0x1
#define VLAN_FLAG_GVRP			0x2
#define VLAN_FLAG_LOOSE_BINDING		0x4
#define VLAN_FLAG_MVRP			0x8
#define VLAN_FLAG_BRIDGE_BINDING	0x10

#ifndef IFA_F_SECONDARY
#define IFA_F_SECONDARY		0x01
#define IFA_F_TEMPORARY		IFA_F_SECONDARY
#define	IFA_F_NODAD		0x02
#define IFA_F_OPTIMISTIC	0x04
#define IFA_F_DADFAILED		0x08
#define	IFA_F_HOMEADDRESS	0x10
#define IFA_F_DEPRECATED	0x20
#define IFA_F_TENTATIVE		0x40
#define IFA_F_PERMANENT		0x80
#define IFA_F_MANAGETEMPADDR	0x100
#define IFA_F_NOPREFIXROUTE	0x200
#define IFA_F_MCAUTOJOIN	0x400
#define IFA_F_STABLE_PRIVACY	0x800
#endif

#define IN6_ADDR_GEN_MODE_EUI64 0
#define IN6_ADDR_GEN_MODE_NONE 1
#define IN6_ADDR_GEN_MODE_STABLE_PRIVACY 2
#define IN6_ADDR_GEN_MODE_RANDOM 3

#ifndef AF_MCTP
#define AF_MCTP		45	/* Management component transport protocol */
#undef AF_MAX
#define AF_MAX		46
#ifndef AF_XDP
#define AF_XDP		44	/* XDP sockets			*/
#endif
#endif

#else

#include <netinet/if_ether.h>

#ifndef AF_AFP
#define AF_AFP  36              /* Used by AFP */
#endif
#ifndef AF_MULTIPATH
#define AF_MULTIPATH    39
#endif
#ifndef AF_VSOCK
#define AF_VSOCK        40      /* VM Sockets */
#endif

#endif

#ifndef ARPHRD_LOOPBACK
#define ARPHRD_LOOPBACK	772		/* Loopback device		*/
#endif

#ifndef ARPHRD_NONE
#define ARPHRD_NONE	  0xFFFE	/* zero header length */
#endif

#ifndef ARPHRD_IEEE802154_MONITOR
#define ARPHRD_IEEE802154_MONITOR 805	/* IEEE 802.15.4 network monitor */
#endif

#ifndef ARPHRD_PHONET
#define ARPHRD_PHONET	820		/* PhoNet media type		*/
#endif

#ifndef ARPHRD_PHONET_PIPE
#define ARPHRD_PHONET_PIPE 821		/* PhoNet pipe header		*/
#endif

#ifndef ARPHRD_CAIF
#define ARPHRD_CAIF	822		/* CAIF media type		*/
#endif

#ifndef ARPHRD_IP6GRE
#define ARPHRD_IP6GRE	823		/* GRE over IPv6		*/
#endif

#ifndef ARPHRD_NETLINK
#define ARPHRD_NETLINK	824		/* Netlink header		*/
#endif

#ifndef ARPHRD_6LOWPAN
#define ARPHRD_6LOWPAN	825		/* IPv6 over LoWPAN             */
#endif

#ifndef ARPHRD_VSOCKMON
#define ARPHRD_VSOCKMON	826		/* Vsock monitor header		*/
#endif

#ifndef ARPHRD_MCTP
#define ARPHRD_MCTP	290
#endif

#ifndef ARPHRD_CAN
#define ARPHRD_CAN	280		/* Controller Area Network      */
#endif

const char * ifflagsname(uint32_t flags); // IFF_
uint32_t addflag(char * buf, const char * name, uint32_t flags, uint32_t flag);
uint32_t addflag_space_separator(char * buf, const char * name, uint32_t flags, uint32_t flag);
const char * ifflagsname(uint32_t flags);
uint8_t Ip6MaskToBits(const char * mask);
uint8_t IpMaskToBits(const char * mask);
const char * IpBitsToMask(uint32_t bits, char * buffer, size_t maxlen);
const char * RouteFlagsToString(uint32_t iflags, int is_ipv6 );

#ifndef RTNL_FAMILY_IPMR
#define RTNL_FAMILY_IPMR 128 // rtm_type == RTN_MULTICAST
#endif

#ifndef RTNL_FAMILY_IP6MR
#define RTNL_FAMILY_IP6MR 129 // rtm_type == RTN_MULTICAST
#endif

const char * familyname(char family);
const char * ipfamilyname(char family);
const char * arphdrname(uint16_t ar_hrd);
const char * ethprotoname(uint16_t h_proto); // !NB function uses host byte order - use after ntohs(h_proto)
const char * iprotocolname(uint8_t proto); // !NB out of range IPPROTO_MPTCP = 262, /* Multipath TCP connection	*/

#if !defined(__APPLE__) && !defined(__FreeBSD__)

int has_accept_ra_rt_table(void);

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 0x10000
#endif //IFF_LOWER_UP

#ifndef IFF_DORMANT
#define IFF_DORMANT 0x20000
#endif //IFF_DORMANT

#ifndef IFF_ECHO
#define IFF_ECHO 0x40000
#endif //IFF_ECHO

#ifndef VLAN_FLAG_BRIDGE_BINDING
#define VLAN_FLAG_BRIDGE_BINDING 0x10
#endif

const char * vlanflags(uint32_t vflags); // VLAN_FLAG_ https://www.juniper.net/documentation/ru/ru/software/junos/multicast-l2/topics/topic-map/mvrp.html

#ifndef IFA_F_MCAUTOJOIN
#define IFA_F_MCAUTOJOIN	0x400
#ifndef IFA_F_STABLE_PRIVACY
#define IFA_F_STABLE_PRIVACY	0x800
#endif
#endif

// IFA_F_SECONDARY - адрес вторичного интерфейса.
// IFA_F_NODAD - не использовать алгоритм определения дубликатов адресов (DAD).
// IFA_F_OPTIMISTIC - оптимистичный алгоритм определения дубликатов адресов (DAD).
// IFA_F_DADFAILED - алгоритм определения дубликатов адресов (DAD) не удался.
// IFA_F_HOMEADDRESS - адрес домашнего интерфейса.
// IFA_F_DEPRECATED - адрес устарел.
// IFA_F_TENTATIVE - адрес временный.
// IFA_F_PERMANENT - адрес постоянный.
// IFA_F_MANAGETEMPADDR - управление временными адресами.
// IFA_F_NOPREFIXROUTE - не использовать префикс маршрута.
// IFA_F_MCAUTOJOIN означает, что адрес является адресом мультикаста и будет автоматически присоединен к соответствующей группе мультикаста при отправке пакетов на этот адрес
// IFA_F_STABLE_PRIVACY означает, что адрес является стабильным и не изменится в течение жизни интерфейса. Это означает, что адрес не будет меняться при перезагрузке или перезапуске интерфейса.
const char * ifaddrflags(uint32_t iflags); // IFA_F_ 


#ifndef IF_RA_OTHERCONF
// RFC 2462 https://habr.com/ru/articles/210224/ https://dic.academic.ru/dic.nsf/ruwiki/12084
// Router Advertisement (RA) - это сообщение, которое отправляется маршрутизатором в локальной сети,
// чтобы объявить свой IP-адрес доступным для маршрутизации
// RA-сообщения используются протоколом обнаружения соседей (NDP) для обнаружения соседей,
// объявления префиксов IPv6, помощи в предоставлении адресов и передачи параметров связи, 
// таких как максимальный размер передаваемого блока (MTU), предел перехода, интервалы извещений и время жизни
// Router Solicitation (RS) - это сообщение, которое отправляется на “все маршрутизаторы ipv6”
// FF02::2 multicast адреc
// RA (Router Advertisement) отправляется маршрутизатором и включает в себя его link-local IPv6 адрес
// Маршрутизаторы, получившие это сообщение, отвечают ICMPv6 сообщением «Router Advertisement», 
// в котором может содержаться информация о сетевом префиксе, адресе шлюза, адресах рекурсивных DNS серверов, MTU и т.д
#define IF_RA_OTHERCONF	0x80	// это флаг, который устанавливается в структуре in6_ifaddr при получении сообщения 
				// Router Advertisement (RA) от маршрутизатора. Он указывает, что адреса, которые 
				// были получены через RA, не являются предпочтительными и могут быть использованы
				// только в случае отсутствия других адресов.
#define IF_RA_MANAGED	0x40	// полная противоположность IF_RA_OTHERCONF - адреса, которые были получены через RA,
				// являются предпочтительными и могут быть использованы для связи.
#define IF_RA_RCVD	0x20	// устанавливается в структуре in6_ifaddr при получении сообщения Router Advertisement (RA) от маршрутизатора (адреса получены)
#define IF_RS_SENT	0x10	// устанавливается в структуре in6_ifaddr при получении сообщения Router Solicitation (RS) от маршрутизатора (ждем ответа)
#define IF_READY	0x80000000 // Интерфейс готов к использованию
#endif

const char * inet6flags(uint32_t iflags); // IF_RA_ (inet6_dev.if_flags)
const char * ipv6genmodeflags(uint8_t iflags); // IN6_ADDR_GEN_MODE_
// В сети IPv6 адреса часто присваиваются (создаются) ядром с использованием как PREFIX: информации, предоставленной маршрутизатором (DHCPv6-сервером),
// так и информации :SUFFIX , извлеченной из оборудования (MAC-адрес).
// Механизм маршрутизатора для этой функции называется «Объявление маршрутизатора» (RA).
// Машина обычно имеет несколько адресов IPv6 для глобальной, канальной или сетевой области.
// Если машине нужен доступ в Интернет без дополнительного маршрутизатора или NAT, ей нужен общедоступный IPv6-адрес с тем же префиксом, что и у шлюза IPv6.
// И благодаря токену глобальный адрес также сохраняет свой суффикс!


//IF_OPER_UNKNOWN (0): Интерфейс находится в неизвестном состоянии, ни драйвер, ни пользовательское пространство не установили операционное состояние. Интерфейс должен рассматриваться для пользовательских данных, так как установка операционного состояния не реализована в каждом драйвере.
//IF_OPER_NOTPRESENT (1): Интерфейс отсутствует и не может быть использован.
//IF_OPER_DOWN (2): Интерфейс находится в отключенном состоянии.
//IF_OPER_LOWERLAYERDOWN (3): Нижний уровень интерфейса находится в отключенном состоянии.
//IF_OPER_TESTING (4): Интерфейс находится в тестовом режиме.
//IF_OPER_DORMANT (5): Интерфейс находится в спящем режиме.
//IF_OPER_UP (6): Интерфейс находится в подключенном состоянии и готов к использованию
const char * ifoperstate(uint8_t operstate);
const char * iflinkmode(uint8_t link_mode);

#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_types.h>
#include <net/route.h>
const char * ifttoname(uint8_t ift); // IFT_
const char * rtmtoname(uint8_t rtm); // RTM_
#endif

#ifdef __cplusplus
}
#endif

#endif // __COMMON_NETUTILS_H__
