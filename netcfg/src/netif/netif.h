#ifndef __NETIF_H__
#define __NETIF_H__

#include <string>
#include <map>
#include <stdint.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <common/netlink.h>
#include <cstring>

#ifndef DEVCONF_ACCEPT_RA_RT_TABLE
#define DEVCONF_ACCEPT_RA_RT_TABLE 30
#endif

#endif

// TODO: добавить net.ipv4.conf.default.promote_secondaries (net.ipv4.conf.${interface}.promote_secondaries)
// если promote_secondariesустановлено значение 0, удаление первичного адреса также приведет к удалению всех
// вторичных адресов из его интерфейса
// PS: Вторичные IPv6-адреса всегда повышаются до первичных, если первичный адрес удаляется
// TODO: добавить label ${interface name}:${description}  ip address add 192.0.2.1/24 dev eth0 label eth0:ip_for_special_net
// PS: Для адресов IPv6 эта команда не действует. Он добавит адрес правильно, но проигнорирует метку.
// TODO: Удалить все адреса из интерфейса ip [-4 -6] address flush dev ${interface name}


struct IpAddressInfo {
	uint32_t flags;
	uint8_t family;
	uint8_t prefixlen;
	uint8_t scope;
	std::wstring ip;
	std::wstring netmask;
	std::wstring point_to_point;
	std::wstring broadcast;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	std::wstring local;
	std::wstring label;
	int32_t netnsid;
	uint32_t rt_priority;
	uint8_t proto;
	struct {
		uint32_t ifa_prefered;
		uint32_t ifa_valid;
		uint32_t cstamp; /* created timestamp, hundredths of seconds */
		uint32_t tstamp; /* updated timestamp, hundredths of seconds */
	} cacheinfo;
	IpAddressInfo():flags(0),
			family(0),
			prefixlen(0),
			scope(0),
			netnsid(-1),
			rt_priority(0),
			proto(0) { memset(&cacheinfo, 0, sizeof(cacheinfo) ); };
	~IpAddressInfo() {};
#endif
};

struct MacAddressInfo {
	uint32_t flags;
	uint8_t family;
	std::wstring mac;
	std::wstring broadcast;
};

struct NetInterface {
	uint32_t size;
	std::wstring name;
	std::map<std::wstring, MacAddressInfo> mac;
	std::map<std::wstring, IpAddressInfo> ip;
	std::map<std::wstring, IpAddressInfo> ip6;
	// TODO: multicast addresses
	std::wstring permanent_mac;
	uint32_t type;
	uint32_t ifa_flags;
	uint32_t mtu;
	uint32_t ifindex;
	uint64_t recv_bytes;
	uint64_t send_bytes;
	uint64_t recv_packets;
	uint64_t send_packets;
	uint64_t recv_errors;
	uint64_t send_errors;
	uint64_t multicast;
	uint64_t collisions;

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	struct {

		std::wstring qdisc;				//  noqueue означает «отправляй мгновенно, не ставь в очередь». Это стандартная дисциплина qdisc для виртуальных устройств, например адресов LOOPBACK.
/*
Дисциплина очереди (qdisc) - это механизм ядра Linux для управления трафиком сети. Она определяет, каким образом пакеты будут обрабатываться и передаваться через интерфейс.
Чтобы установить дисциплину очереди для интерфейса, используйте команду tc qdisc add. Например, чтобы добавить дисциплину очереди pfifo_fast для интерфейса eth0, выполните следующую команду:

tc qdisc add dev eth0 root pfifo_fast

Если вы хотите заменить существующую дисциплину очереди на новую, используйте команду tc qdisc replace. Например:

tc qdisc replace dev eth0 root pfifo_fast

Как работает дисциплина очереди в Linux зависит от конкретной дисциплины. 
Например, дисциплина pfifo_fast использует простую очередь FIFO для обработки пакетов, 
а дисциплина HTB использует иерархическую структуру для управления пропускной способностью и задержкой

pfifo_fast - использует простую очередь FIFO для обработки пакетов.
HTB - использует иерархическую структуру для управления пропускной способностью и задержкой.
TBF - использует токенный бакетный фильтр для управления пропускной способностью и задержкой.
SFQ - использует справедливую очередь для обработки пакетов.
FQ - использует справедливую очередь для обработки пакетов и управления пропускной способностью.
CoDel - дисциплина очереди с контролируемой задержкой.
noqueue - используется для интерфейсов, которые не нуждаются в управлении очередью пакетов 

Как работает fq_codel:

FQ_Codel (управляемая задержка честной очереди) - это дисциплина очередей, которая объединяет честную очередь со схемой 
CoDel AQM. FQ_Codel использует стохастическую модель для классификации входящих пакетов в разные потоки и используется 
для обеспечения справедливой доли пропускной способности для всех потоков, использующих очередь. Каждый такой поток 
управляется дисциплиной очередей CoDel. Переупорядочение внутри потока исключается, поскольку Codel внутренне использует очередь FIFO.

fq_codel использует несколько очередей для обработки пакетов.
Каждая очередь имеет свой собственный буфер и свою собственную очередь.
Каждый пакет помещается в очередь на основе его хэш-значения.
Каждая очередь имеет свой собственный таймер, который определяет, когда пакет должен быть отправлен.
Если очередь переполнена, fq_codel отбрасывает пакеты с наибольшей задержкой.
Это позволяет уменьшить задержку и потери пакетов в сетях с высокой загрузкой.
fq_codel является одной из наиболее эффективных дисциплин очереди для управления трафиком в Linux.
*/

		struct rtnl_link_ifmap ifmap;
		struct rtnl_link_stats64 stat64;
		struct {
			uint64_t  totalStatsFields; //         IPSTATS_MIB_NUM = 0,
			uint64_t  Ip6InReceives; //            IPSTATS_MIB_INPKTS 1,			
			uint64_t  Ip6InOctets; //              IPSTATS_MIB_INOCTETS 2,			
			uint64_t  Ip6InDelivers; //            IPSTATS_MIB_INDELIVERS 3,			
			uint64_t  Ip6OutForwDatagrams; //      IPSTATS_MIB_OUTFORWDATAGRAMS,		
			uint64_t  Ip6OutRequests; //           IPSTATS_MIB_OUTPKTS,			
			uint64_t  Ip6OutOctets; //             IPSTATS_MIB_OUTOCTETS,			
			uint64_t  Ip6InHdrErrors; //           IPSTATS_MIB_INHDRERRORS,		
			uint64_t  Ip6InTooBigErrors; //        IPSTATS_MIB_INTOOBIGERRORS,		
			uint64_t  Ip6InNoRoutes; //            IPSTATS_MIB_INNOROUTES,			
			uint64_t  Ip6InAddrErrors; //          IPSTATS_MIB_INADDRERRORS,		
			uint64_t  Ip6InUnknownProtos; //       IPSTATS_MIB_INUNKNOWNPROTOS,		
			uint64_t  Ip6InTruncatedPkts; //       IPSTATS_MIB_INTRUNCATEDPKTS,		
			uint64_t  Ip6InDiscards; //            IPSTATS_MIB_INDISCARDS,			
			uint64_t  Ip6OutDiscards; //           IPSTATS_MIB_OUTDISCARDS,		
			uint64_t  Ip6OutNoRoutes; //           IPSTATS_MIB_OUTNOROUTES,		
			uint64_t  Ip6ReasmTimeout; //          IPSTATS_MIB_REASMTIMEOUT,		
			uint64_t  Ip6ReasmReqds; //            IPSTATS_MIB_REASMREQDS,			
			uint64_t  Ip6ReasmOKs; //              IPSTATS_MIB_REASMOKS,			
			uint64_t  Ip6ReasmFails; //            IPSTATS_MIB_REASMFAILS,			
			uint64_t  Ip6FragOKs; //               IPSTATS_MIB_FRAGOKS,			
			uint64_t  Ip6FragFails; //             IPSTATS_MIB_FRAGFAILS,			
			uint64_t  Ip6FragCreates; //           IPSTATS_MIB_FRAGCREATES,		
			uint64_t  Ip6InMcastPkts; //           IPSTATS_MIB_INMCASTPKTS,		
			uint64_t  Ip6OutMcastPkts; //          IPSTATS_MIB_OUTMCASTPKTS,		
			uint64_t  Ip6InBcastPkts; //           IPSTATS_MIB_INBCASTPKTS,		
			uint64_t  Ip6OutBcastPkts; //          IPSTATS_MIB_OUTBCASTPKTS,		
			uint64_t  Ip6InMcastOctets; //         IPSTATS_MIB_INMCASTOCTETS,		
			uint64_t  Ip6OutMcastOctets; //        IPSTATS_MIB_OUTMCASTOCTETS,		
			uint64_t  Ip6InBcastOctets; //         IPSTATS_MIB_INBCASTOCTETS,		
			uint64_t  Ip6OutBcastOctets; //        IPSTATS_MIB_OUTBCASTOCTETS,		
			uint64_t  Ip6InCsumErrors; //          IPSTATS_MIB_CSUMERRORS,			
			uint64_t  Ip6InNoECTPkts; //           IPSTATS_MIB_NOECTPKTS,			
			uint64_t  Ip6InECT1Pkts; //            IPSTATS_MIB_ECT1PKTS,			
			uint64_t  Ip6InECT0Pkts; //            IPSTATS_MIB_ECT0PKTS,			
			uint64_t  Ip6InCEPkts; //              IPSTATS_MIB_CEPKTS,			
			uint64_t  Ip6ReasmOverlaps; //         IPSTATS_MIB_REASM_OVERLAPS,		
		} inet6stat;

		struct {
			uint64_t  totalStatsFields;     // ICMP6_MIB_NUM = 0,
			uint64_t  Icmp6InMsgs;          // ICMP6_MIB_INMSGS,			/* InMsgs */
			uint64_t  Icmp6InErrors;        // ICMP6_MIB_INERRORS,			/* InErrors */
			uint64_t  Icmp6OutMsgs;         // ICMP6_MIB_OUTMSGS,			/* OutMsgs */
			uint64_t  Icmp6OutErrors;       // ICMP6_MIB_OUTERRORS,			/* OutErrors */
			uint64_t  Icmp6InCsumErrors;    // ICMP6_MIB_CSUMERRORS,		/* InCsumErrors */
			uint64_t  Icmp6OutRateLimitHost;// ICMP6_MIB_RATELIMITHOST,		/* OutRateLimitHost */
		} icmp6stat;

		// Каталог /proc/sys/net/ipv4(6)/DEV/, где под DEV следует понимать название того или иного устройства, содержит настройки 
		// для конкретного сетевого интерфейса. Настройки из каталога conf/all/ применяются ко ВСЕМ сетевым интерфейсам.
		// И наконец каталог conf/default/ содержит значения по-умолчанию. Они не влияют на настройки существующих интерфейсов,
		// но используются для первоначальной настройки вновь устанавливаемых устройств.

		struct {
			uint32_t totalFields;
			int32_t forwarding;             // DEVCONF_FORWARDING = 0,
			int32_t hop_limit;              // DEVCONF_HOPLIMIT,
			int32_t mtu;                    // DEVCONF_MTU6,
			int32_t accept_ra;              // DEVCONF_ACCEPT_RA,
			int32_t accept_redirects;       // DEVCONF_ACCEPT_REDIRECTS, ICMP Redirect используются для уведомления маршрутизаторов или хостов
							// о существовании лучшего маршрута движения пакетов к заданному хосту,
							// который (маршрут) может быть быстрее или менее загружен. Опция далеко небезопасна.
			int32_t autoconf;               // DEVCONF_AUTOCONF,
			int32_t dad_transmits;          // DEVCONF_DAD_TRANSMITS, параметр в IPv6, который определяет количество попыток 
							// отправки сообщения DAD (Duplicate Address Detection)
			int32_t router_solicitations;	// DEVCONF_RTR_SOLICITS,
			int32_t router_solicitation_interval; // DEVCONF_RTR_SOLICIT_INTERVAL, определяет количество секунд между отправкой
							// запросов на получение маршрутизатора (Router Solicitation)
			int32_t router_solicitation_delay; // DEVCONF_RTR_SOLICIT_DELAY, определяет количество секунд, которое нужно подождать
							// после запуска интерфейса перед отправкой запросов на получение маршрутизатора (Router Solicitation) 
			int32_t use_tempaddr;           // DEVCONF_USE_TEMPADDR,
			int32_t temp_valid_lft;         // DEVCONF_TEMP_VALID_LFT - это время в секундах, в течение которого временный адрес является
							// действительным. По умолчанию это 7 дней (604800 секунд).
			int32_t temp_prefered_lft;	// DEVCONF_TEMP_PREFERED_LFT - это время в секундах, в течение которого временный адрес является
							// предпочтительным. По умолчанию это 1 день (86400 секунд).
			int32_t regen_max_retry;	// DEVCONF_REGEN_MAX_RETRY,
			int32_t max_desync_factor;	// DEVCONF_MAX_DESYNC_FACTOR - параметр протокола NTP (Network Time Protocol), который определяет
							// максимальное количество времени в секундах, на которое клиент может отклоняться от сервера времени
			int32_t max_addresses;          // DEVCONF_MAX_ADDRESSES,
			int32_t force_mld_version;      // DEVCONF_FORCE_MLD_VERSION,
			int32_t accept_ra_defrtr;       // DEVCONF_ACCEPT_RA_DEFRTR,
			int32_t accept_ra_pinfo;        // DEVCONF_ACCEPT_RA_PINFO,
			int32_t accept_ra_rtr_pref;     // DEVCONF_ACCEPT_RA_RTR_PREF,
			int32_t router_probe_interval;  // DEVCONF_RTR_PROBE_INTERVAL, интервал между отправкой пакетов маршрутизатора для проверки доступности
							// маршрута измеряется в секундах.
			int32_t accept_ra_rt_info_max_plen; // DEVCONF_ACCEPT_RA_RT_INFO_MAX_PLEN,
			int32_t proxy_ndp;              // DEVCONF_PROXY_NDP,
			int32_t optimistic_dad;         // DEVCONF_OPTIMISTIC_DAD,
			int32_t accept_source_route;    // DEVCONF_ACCEPT_SOURCE_ROUTE,
			int32_t mc_forwarding;          // DEVCONF_MC_FORWARDING, отвечает за пересылку многоадресных пакетов. Он используется для настройки 
							// маршрутизатора для пересылки многоадресных пакетов в сети.
			int32_t disable_ipv6;           // DEVCONF_DISABLE_IPV6,
			int32_t accept_dad;             // DEVCONF_ACCEPT_DAD,
			int32_t force_tllao;            // DEVCONF_FORCE_TLLAO,
			int32_t ndisc_notify;           // DEVCONF_NDISC_NOTIFY,
			int32_t mldv1_unsolicited_report_interval; // DEVCONF_MLDV1_UNSOLICITED_REPORT_INTERVAL, https://datatracker.ietf.org/doc/html/rfc2710,
							// параметр протокола Multicast Listener Discovery (MLD), который определяет интервал времени между
							// отправкой непринужденных отчетов MLDv1. Он измеряется в секундах
			int32_t mldv2_unsolicited_report_interval; // DEVCONF_MLDV2_UNSOLICITED_REPORT_INTERVAL, 
			int32_t suppress_frag_ndisc;    // DEVCONF_SUPPRESS_FRAG_NDISC,
			int32_t accept_ra_from_local;   // DEVCONF_ACCEPT_RA_FROM_LOCAL,
			int32_t use_optimistic;         // DEVCONF_USE_OPTIMISTIC,
			int32_t accept_ra_mtu;          // DEVCONF_ACCEPT_RA_MTU,
			int32_t stable_secret;          // DEVCONF_STABLE_SECRET,
			int32_t use_oif_addrs_only;     // DEVCONF_USE_OIF_ADDRS_ONLY,
			int32_t accept_ra_min_hop_limit; // DEVCONF_ACCEPT_RA_MIN_HOP_LIMIT,
			int32_t ignore_routes_with_linkdown; // DEVCONF_IGNORE_ROUTES_WITH_LINKDOWN,
			int32_t drop_unicast_in_l2_multicast; // DEVCONF_DROP_UNICAST_IN_L2_MULTICAST,
			int32_t drop_unsolicited_na;    // DEVCONF_DROP_UNSOLICITED_NA,
			int32_t keep_addr_on_down;      // DEVCONF_KEEP_ADDR_ON_DOWN,
			int32_t router_solicitation_max_interval; // DEVCONF_RTR_SOLICIT_MAX_INTERVAL, определяет максимальный интервал времени между отправкой
							// запросов на получение маршрутизатора (Router Solicitation) сек
			int32_t seg6_enabled;           // DEVCONF_SEG6_ENABLED,
			int32_t seg6_require_hmac;      // DEVCONF_SEG6_REQUIRE_HMAC,
			int32_t enhanced_dad;           // DEVCONF_ENHANCED_DAD,
			int32_t addr_gen_mode;          // DEVCONF_ADDR_GEN_MODE,
			int32_t disable_policy;         // DEVCONF_DISABLE_POLICY,
			int32_t accept_ra_rt_info_min_plen; // DEVCONF_ACCEPT_RA_RT_INFO_MIN_PLEN,
			int32_t ndisc_tclass;           // DEVCONF_NDISC_TCLASS,
			int32_t rpl_seg_enabled;        // DEVCONF_RPL_SEG_ENABLED,
			int32_t ra_defrtr_metric;       // DEVCONF_RA_DEFRTR_METRIC,
			int32_t ioam6_enabled;          // DEVCONF_IOAM6_ENABLED,
			int32_t ioam6_id;               // DEVCONF_IOAM6_ID,
			int32_t ioam6_id_wide;          // DEVCONF_IOAM6_ID_WIDE,
			int32_t ndisc_evict_nocarrier;  // DEVCONF_NDISC_EVICT_NOCARRIER,
			int32_t accept_untracked_na;    // DEVCONF_ACCEPT_UNTRACKED_NA,
		} ipv6conf;

		int32_t accept_ra_rt_table;
// DEVCONF_ACCEPT_RA_RT_TABLE,
// Currently, IPv6 router discovery always puts routes into
// RT6_TABLE_MAIN. This makes it difficult to maintain and switch
// between multiple simultaneous network connections (e.g., wifi
// and wired).

// To work around this connection managers typically either move
// autoconfiguration to userspace entirely (e.g., dhcpcd) or take
// the routes they want and re-add them to the main table as static
// routes with low metrics (e.g., NetworkManager). This puts the
// burden on the connection manager to watch netlink or listen to
// RAs to see if the routes have changed, delete the routes when
// their lifetime expires, etc. This is complex and often not
// implemented correctly.

// This patch adds a per-interface sysctl to have the kernel put
// autoconf routes into different tables. This allows each interface
// to have its own routing table if desired.  Choosing the default
// interface, or using different interfaces at the same time on a
// per-socket or per-packet basis) can be done using policy routing
// mechanisms that use as SO_BINDTODEVICE / IPV6_PKTINFO, mark-based
// routing, or UID-based routing to select specific routing tables.

// The sysctl behaves as follows:

// - = 0: default. Put routes into RT6_TABLE_MAIN if the interface
//        is not in a VRF, or into the VRF table if it is.
// - > 0: manual. Put routes into the specified table.
// - < 0: automatic. Add the absolute value of the sysctl to the
//        device's ifindex, and use that table.

// The automatic mode is most useful in conjunction with
// net.ipv6.conf.default.accept_ra_rt_table. A connection manager
// or distribution can set this to, say, -1000 on boot, and
// thereafter know that routes received on every interface will
// always be in that interface's routing table, and that the mapping
// between interfaces and routing tables is deterministic. It also
// ensures that if an interface is created and immediately receives
// an RA, the route will go into the correct routing table without
// needing any intervention from userspace.

// The automatic mode (with conf.default.accept_ra_rt_table = -1000)
// has been used in Android since 5.0.

		// https://www.opennet.ru/docs/RUS/ipsysctl/#IPPARAM.TXT
		// https://www.opennet.ru/docs/RUS/ipsysctl/misc/ipsysctl-tutorial/other/ip-param.txt
		// https://maxidrom.net/archives/1478

		struct {
			uint32_t totalFields;			    // https://www.opennet.ru/docs/RUS/ipsysctl/#AEN239 https://maxidrom.net/archives/1478
			int32_t forwarding;                         // IPV4_DEVCONF_FORWARDING: передача транзитных пакетов между сетевыми интерфейсами
								    //     По-умолчанию все переменные conf/DEV/forwarding принимают значение равное ipv4/ip_forward
			int32_t mc_forwarding;                      // IPV4_DEVCONF_MC_FORWARDING: поддержку маршрутизации групповых рассылок для заданного интерфейса
								    //     ядро с включенной опцией CONFIG_MROUTE. Дополнительно в системе должен иметься демон, осуществляющий групповую маршрутизацию.
								    //     Демон реализует один из протоколов:
								    //     DVMRP (от англ. Distance Vector Multicast Routing Protocol - протокол маршрутизации групповых рассылок типа "вектор-расстояние")
								    //     PIM (от англ. Protocol Independent Multicast - протокол маршрутизации групповых рассылок) PIM-SM
								    //     PIM-DM (Protocol Independent Multicast-Dense Mode - протокол маршрутизации групповых рассылок, независимый от используемого протокола маршрутизации, "уплотненного" режима)
								    //     https://tldp.org/HOWTO/Multicast-HOWTO.html

								    //     Групповая адресация используется в тех случаях, когда необходимо выполнить доставку информации
								    //     сразу к нескольким пунктам назначения. Например, WEB-страничка передается отдельно каждому, 
								    //     кто ее запросил, а если несколько человек решили принять участие в видеоконференции? 
								    //     Есть два пути реализации доставки. Либо каждому участнику отдавать отдельный поток данных 
								    //     (тогда трафик будет таким огромным, что для него может просто не хватить пропускной способности
								    //     канала), либо использовать групповую рассылку. В этом случае отправитель создает
								    //     одну дейтаграмму с групповым адресом назначения, по мере продвижения через сеть дейтаграмма 
								    //     будет дублироваться только на "развилках" маршрутов от отправителя к получателям.

			int32_t proxy_arp;                          // IPV4_DEVCONF_PROXY_ARP: Включает/выключает проксирование arp-запросов для заданного интерфейса. 
								    //     ARP-прокси позволяет маршрутизатору отвечать на ARP запросы в одну сеть, в то время как 
								    //     запрашиваемый хост находится в другой сети. С помощью этого средства происходит "обман" 
								    //     отправителя, который отправил ARP запрос, после чего он думает, что маршрутизатор является
								    //     хостом назначения, тогда как в действительности хост назначения находится "на другой стороне"
								    //     маршрутизатора. http://www.ibiblio.org/pub/Linux/docs/HOWTO/unmaintained/mini/Proxy-ARP

								    //     Если наш сервер не маршрутизатор выключаем, маршрутизатор - строго включено.
								    //     ICMP-перенаправления могут быть использованы злоумышленником для изменения таблиц маршрутизации.
			int32_t accept_redirects;                   // IPV4_DEVCONF_ACCEPT_REDIRECTS: мы запрещаем принимать ICMP пакеты перенаправления enabled if local forwarding is disabled.
			int32_t secure_redirects;                   // IPV4_DEVCONF_SECURE_REDIRECTS: Accept ICMP redirect messages only to gateways listed in the
								    //     interface’s current gateway list. Even if disabled, RFC1122 redirect rules still apply. 
								    //     Overridden by shared_media.
								    //     Если этот параметр установлен в 1, то ICMP-перенаправления принимаются только к шлюзам, 
								    //     перечисленным в текущем списке шлюзов по-умолчанию (gateway) интерфейса. Если этот параметр установлен 
								    //     в 0, то принимаются все ICMP-перенаправления
								    //     действие этой переменной отменяется переменной shared_media
			int32_t send_redirects;                     // IPV4_DEVCONF_SEND_REDIRECTS: мы запрещаем отправлять ICMP пакеты перенаправления Send redirects, if router.
								    //     send_redirects for the interface will be enabled if at least one of
								    //     conf/{all,interface}/send_redirects is set to TRUE, it will be disabled otherwise

			int32_t shared_media;                       // IPV4_DEVCONF_SHARED_MEDIA: Включает/выключает признак того, что физическая сеть является носителем
								    //     нескольких логических подсетей, например, когда на одном физическом кабеле организовано 
								    //     несколько подсетей с различными сетевыми масками. Этот признак используется ядром при принятии
								    //     решения о необходимости выдачи ICMP-сообщений о переадресации.
			int32_t rp_filter;                          // IPV4_DEVCONF_RP_FILTER: Смысл этой переменной достаточно прост все что поступает к нам проходит
								    //     проверку на соответствие исходящего адреса с нашей таблицей маршрутизации и такая проверка 
								    //     считается успешной, если принятый пакет предполагает передачу ответа через тот же самый интерфейс
								    //     Если вы используете расширенную маршрутизацию тем или иным образом, то вам следует всерьез задуматься
								    //     о выключении этой переменной, поскольку она может послужить причиной потери пакетов.
								    //     Например, в случае, когда входящий трафик идет через один маршрутизатор, а исходящий через другой.
								    //     Так, WEB-сервер, подключенный через один сетевой интерфейс к входному роутеру, а через другой к выходному
								    //     (в случае, когда включен rp_filter), будет просто терять входящий трафик, поскольку обратный маршрут,
								    //     в таблице маршрутизации, задан через другой интерфейс. https://www.opennet.ru/docs/RUS/ipsysctl/misc/ipsysctl-tutorial/other/rfc1812.txt
								    //     sysctl -w net.ipv4.conf.all.rp_filter=1
			int32_t accept_source_route;                // IPV4_DEVCONF_ACCEPT_SOURCE_ROUTE: позволяет отправителю
								    //     определить путь, по которому пакет должен пройти по сети Internet, чтобы
								    //     достигнуть пункта назначения.
								    //     Это означает, что отправитель может указать путь, по которому пакет должен пройти по сети
								    //     Internet, чтобы достигнуть пункта назначения. Небезопасно - лучше отключать.
			int32_t bootp_relay;                        // IPV4_DEVCONF_BOOTP_RELAY: используется для настройки ретрансляции DHCP и BOOTP. Он указывает на адрес интерфейса, на котором должен быть произведен ретранслятор DHCP или BOOTP.
								    //      Переменная разрешает/запрещает форвардинг пакетов с исходящими адресами 0.b.c.d. Демон BOOTP relay должен перенаправлять эти пакеты на корректный адрес.
			int32_t log_martians;                       // IPV4_DEVCONF_LOG_MARTIANS: включает/выключает функцию журналирования всех пакетов, которые содержат
								    //      неправильные (невозможные) адреса (так называемые martians -- "марсианские" пакеты)
								    //      Под невозможными адресами, в данном случае, следует понимать такие адреса, которые отсутствуют в таблице маршрутизации.
			int32_t tag;                                // IPV4_DEVCONF_TAG:
			int32_t arp_filter;                         // IPV4_DEVCONF_ARPFILTER: Включает/выключает связывание IP-адреса с ARP-адресом. Если эта опция включена, то ответ будет передаваться через тот интерфейс, через который поступил запрос.
			int32_t medium_id;                          // IPV4_DEVCONF_MEDIUM_ID:
			int32_t disable_xfrm;                       // IPV4_DEVCONF_NOXFRM:
			int32_t disable_policy;                     // IPV4_DEVCONF_NOPOLICY:
			int32_t force_igmp_version;                 // IPV4_DEVCONF_FORCE_IGMP_VERSION:
			int32_t arp_announce;                       // IPV4_DEVCONF_ARP_ANNOUNCE:
			int32_t arp_ignore;                         // IPV4_DEVCONF_ARP_IGNORE:
			int32_t promote_secondaries;                // IPV4_DEVCONF_PROMOTE_SECONDARIES:
			int32_t arp_accept;                         // IPV4_DEVCONF_ARP_ACCEPT:
			int32_t arp_notify;                         // IPV4_DEVCONF_ARP_NOTIFY:
			int32_t accept_local;                       // IPV4_DEVCONF_ACCEPT_LOCAL:
			int32_t src_valid_mark;                     // IPV4_DEVCONF_SRC_VMARK:
			int32_t proxy_arp_pvlan;                    // IPV4_DEVCONF_PROXY_ARP_PVLAN:
			int32_t route_localnet;                     // IPV4_DEVCONF_ROUTE_LOCALNET:
			int32_t igmpv2_unsolicited_report_interval; // IPV4_DEVCONF_IGMPV2_UNSOLICITED_REPORT_INTERVAL: это параметр протокола IGMPv2, который определяет интервал времени между отправкой отчетов IGMPv2. Он измеряется в секундах.
			int32_t igmpv3_unsolicited_report_interval; // IPV4_DEVCONF_IGMPV3_UNSOLICITED_REPORT_INTERVAL:
			int32_t ignore_routes_with_linkdown;        // IPV4_DEVCONF_IGNORE_ROUTES_WITH_LINKDOWN:
			int32_t drop_unicast_in_l2_multicast;       // IPV4_DEVCONF_DROP_UNICAST_IN_L2_MULTICAST:
			int32_t drop_gratuitous_arp;                // IPV4_DEVCONF_DROP_GRATUITOUS_ARP:
			int32_t bc_forwarding;                      // IPV4_DEVCONF_BC_FORWARDING:
			int32_t arp_evict_nocarrier;                // IPV4_DEVCONF_ARP_EVICT_NOCARRIER:
		} ipv4conf;



		// TODO: output

		struct {
			uint32_t max_reasm_len;  // максимальный размер фрагмента пакета, который может быть переупорядочен в процессе сборки пакета
			uint32_t tstamp;	 // временная метка, которая указывает время создания записи кэша маршрутизации
			uint32_t reachable_time; // время в миллисекундах, которое запись кэша маршрутизации остается доступной до тех пор, пока не будет удалена из кэша
			uint32_t retrans_time;   // время в миллисекундах, которое запись кэша маршрутизации остается доступной после того, как она стала недоступной
		} inet6cacheinfo;

		std::wstring ipv6token; // IPv6-токен - часть IPv6-адреса, которая используется для создания статических адресов IPv6.
					// Он представляет собой 64-битный идентификатор интерфейса, который добавляется к глобальному префиксу IPv6.
					// Токен может быть настроен вручную или автоматически сгенерирован (ip token set ::35 dev eth0) https://wiki.gentoo.org/wiki/IPv6_Static_Addresses_using_Tokens
		uint32_t inet6flags;
		uint8_t ipv6genmodeflags;

		uint8_t xdp_attached;	// https://developers.redhat.com/blog/2021/04/01/get-started-with-xdp


		std::wstring alias;
		uint32_t gso_max_segs;	// определяет рекомендуемое максимальное количество сегментов Generic Segment Offload (GSO),
					// которые должно принимать новое устройство.
		uint32_t gso_max_size;  // GSO - это техника, используемая сетевыми устройствами для выгрузки сегментации больших пакетов 
					// на аппаратное обеспечение. Это может улучшить производительность сети, 
					// уменьшив использование процессора и увеличивая пропускную способность.
					// Для пакетов, которые превышают размер MTU (Maximum Transmission Unit),
					// GSO разбивает пакет на несколько меньших сегментов и помещает каждый сегмент в отдельный пакет.
					// Эти пакеты затем передаются по сети и собираются обратно в исходный пакет на приемной стороне.
		uint32_t gso_ipv4_max_size;

		uint32_t gro_max_size;	// Generic Receive Offload (GRO) техника в Linux для агрегации нескольких входящих пакетов,
					// принадлежащих одному потоку. Эта техника позволяет снизить использование процессора,
					// так как вместо того, чтобы каждый пакет проходил через сетевой стек индивидуально,
					// один объединенный пакет проходит через сетевой стек.
					// GRO - это широко используемая программная техника для снижения накладных расходов на обработку пакетов.
					// Путем пересборки маленьких пакетов в большие GRO позволяет приложениям обрабатывать меньше больших пакетов напрямую,
					// что уменьшает количество пакетов, которые необходимо обрабатывать.
		uint32_t gro_ipv4_max_size;

		uint32_t tso_max_segs;  // устанавливает максимальное количество сегментов TCP для TCP Segmentation Offload (TSO).
					// Это означает, что если количество сегментов превышает это значение, они не будут обработаны TSO
					// и будут переданы в обработку на уровне ядра
					// С включенным TSO контроллер сетевого интерфейса (NIC) разбивает большие блоки данных, 
					// передаваемые по сети, на меньшие сегменты TCP. Без TSO сегментация выполняется процессором,
					// что создает накладные расходы.
		uint32_t tso_max_size;	// максимальный размер пакета для TCP Segmentation Offload (TSO) для данного интерфейса.
					// Это означает, что если пакет превышает этот размер, он не будет обработан TSO и будет
					// передан в обработку на уровне ядра. TSO также известен как LSO (Large Segment Offload).

		uint32_t txqueuelen; // число пакетов добавляемых в очередь перед передачей.
							 // Если очередь заполнена, write() на блокирующем сокете будет ожидать,
							 // пока очередь не освободится, на неблокирующем выдаст ошибку EWOULDBLOCK
							 // ip link set eth0 txqueuelen 5000 (ifconfig eth1 txqueuelen 10000)
		uint32_t numtxqueues; // число очередей передачи на сетевом интерфейсе
		uint32_t numrxqueues; // число очередей приема на сетевом интерфейсе

		uint8_t operstate;	 // RFC 2863 operational status */
							 // IF_OPER_UNKNOWN
							 // IF_OPER_NOTPRESENT
							 // IF_OPER_DOWN
							 // IF_OPER_LOWERLAYERDOWN,
							 // IF_OPER_TESTING,
							 // IF_OPER_DORMANT,
							 // IF_OPER_UP,
		uint8_t link_mode;	 // link modes
							 // IF_LINK_MODE_DEFAULT
							 // IF_LINK_MODE_DORMANT - limit upward transition to dormant
							 // IF_LINK_MODE_TESTING - limit upward transition to testing
		uint8_t carrier;			 // carrier state
							 // IF_CARRIER_DOWN,
							 // IF_CARRIER_UP

		uint32_t carrier_changes;
		uint32_t carrier_up_count;
		uint32_t carrier_down_count;


		uint32_t minmtu;
		uint32_t maxmtu;
		uint32_t group;		 // 0 default (any other - special group /etc/iproute2/group)

		uint32_t promiscuity; // стчетчик PROMISC режима, если больше 0 то в неразборчивом режиме

		std::wstring linkinfo_kind; // vlan tun sit

		std::wstring egress_qos_map;	// Аргумент (egress-qos-map в ip) определяет сопоставление внутреннего 
						// приоритета пакетов Linux (SO_PRIORITY) структуры skb sk_priority (32 бита) 
						// с полем PCP(полем приоритета заголовка 802.1q) заголовка VLAN 
						// для исходящих кадров. Формат FROM(PCB):TO(Linux priority) с несколькими сопоставлениями, разделенными пробелами.
		std::wstring ingress_qos_map; 	// для входящих кадров "sudo ip link set dev enp2s0.10 type vlan id 10 ingress-qos-map 2:5 3:1 5:3"
		// поле приоритета vlan заголовка имеет размер 3 бита и может принимать значения от 0 до 7.
		// Размер же поля приоритета структуры skb составляет 32 бита.

		//Quality of Service (QoS) —
		//Type of Service (ToS) — поле в IP-заголовке (1 байт). Предназначено для маркировки трафика на сетевом уровне. Сам байт ToS был переименован в поле Differentiated Services (DS);
		// 3 бита
		//Имя                   Десятичное значение  Двоичное значение
		//Routine               0	             000
		//Priority              1                    001
		//Immediate             2                    010
		//Flash                 3                    011
		//Flash Override        4                    100
		//Critic/Critical       5                    101
		//Internetwork Control  6                    110
		//Network Control       7                    111

		// TOS Value		Description				Reference
		// ---------		--------------------------		---------
		// 0000		Default					[Obsoleted by RFC2474]
		// 0001		Minimize Monetary Cost			[Obsoleted by RFC2474]
		// 0010		Maximize Reliability			[Obsoleted by RFC2474]
		// 0100		Maximize Throughput			[Obsoleted by RFC2474]
		// 1000		Minimize Delay				[Obsoleted by RFC2474]
		// 1111		Maximize Security			[Obsoleted by RFC2474]
		// Protocol           TOS Value
		// TELNET (1)         1000                 (minimize delay)
		// FTP
		//   Control          1000                 (minimize delay)
		//   Data (2)         0100                 (maximize throughput)
		// TFTP               1000                 (minimize delay)
		// SMTP (3)
		//   Command phase    1000                 (minimize delay)
		//   DATA phase       0100                 (maximize throughput)
		// Domain Name Service
		//   UDP Query        1000                 (minimize delay)
		//   TCP Query        0000
		//   Zone Transfer    0100                 (maximize throughput)
		// NNTP               0001                 (minimize monetary cost)
		// ICMP
		//   Errors           0000
		//   Requests         0000 (4)
		//   Responses        <same as request> (4)
		// Any IGP            0010                 (maximize reliability)
		// EGP                0000
		// SNMP               0010                 (maximize reliability)
		// BOOTP              0000

		//Позже был определен новый формат поля ToS.
		//Серия RFC, которые определяют новую трактовку поля ToS и возможности связанные с этим, называется Differentiated Services (DiffServ) RFC 2475:
		//Сам байт ToS был переименован в поле Differentiated Services (DS);
		//Вместо поля IPP было определено новое поле из 6 бит, которое называется Differentiated Services Code Point (DSCP) RFC 2474;
		//Оставшиеся 2 бита поля DS используются в QoS Explicit Congestion Notification (ECN).
		//Class of Service (CoS) — поле из 3 бит в теге 802.1Q Ethernet-кадра.

		//Примеры полей:
		//802.1Q (ethernet frame) — Class of Service (CoS), 3 бита;
		//Frame Relay — Discard Eligible (DE), 1 бит;
		//ATM — Cell Loss Priority (CLP), 1 бит;
		//MPLS — Traffic Class (TC), 3 бита.

		//QinQ-инкапсуляция в Linux (https://www.opennet.ru/tips/info/1381.shtml)
		//Q-in-Q инкапсуляция позволяет создавать дважды тегированный трафик. Для каждого уровня вложенности создаётся свой собственный интерфейс.
		//Первое, что в голову приходит это преодоление лимита в 4К вланов. 
		//Второй пример - транзит вланов клиентов через оборудование провайдера.
		//Третий вариант, когда в большой сети где каждому клиенту (считай порту) выделяется свой влан,
		//удобно каждому свичу дать отдельный влан и вланы клиентов гнать через него, 
		//т.е. путь влана прописывается только для свича, клиенские вланы трогаем только на входе и выходе.


		union {
			struct {

				uint16_t vlanprotocol;	// network byte order
				uint16_t vlanid;	// Native VLAN - По умолчанию это VLAN 1. Трафик, передающийся в этом VLAN, не тегируется.
				uint32_t vlanflags; 	// VLAN_FLAG_REORDER_HDR: "on"(default) (reorder_hdr в ip команде) - кадры проходящие
							//                        через vlan интерфейс не содержат тегов ("off" содержат)
							//                        Это нужно учесть, если вы, например, используете фильтры tc 
							//                        типа u32 с указанием смещений
							// VLAN_FLAG_LOOSE_BINDING: "off"(default) (loose_binding в ip) отвечает за синхронизацию
							//                        состояния vlan-интерфейса с нижележащим интерфейсом и при 
							//                        переключении нижележащего интерфейса в состояние down 
							//                        состояние vlan-интерфейса так же меняется на down.
							//                        (если "on", то не меняется и не зависит от состояния нижележащего интерфейса))
							// VLAN_FLAG_GVRP: "off"(default) (gvrp в ip) отвечает за то, распространять ли информацию
							//                        о данном vlan по протоколу gvrp (должна быть поддержка данной возможности со стороны ядра)
							//                        Generic Attribute Registration Protocol. GARP является частью стандарта 802.1D
							//                        предназначен для создания, удаления и переименования VLANов на сетевых устройствах.

			} vlan; //modprobe 8021q нужен для поддержки, просмотр инфы cat /proc/net/vlan/eth0.2 cat /proc/net/vlan/config
		} link;

		struct {
			unsigned int vlan:1;
			unsigned int master:1;
			unsigned int parentdev_name:1;
			unsigned int parentdev_busname:1;
		} valid;

		uint32_t master; // ifmasterindex

		std::wstring parentdev_name;
		std::wstring parentdev_busname;

//	       + nla_total_size(IFNAMSIZ) /* IFLA_IFNAME */
//	       + nla_total_size(IFALIASZ) /* IFLA_IFALIAS */
//	       + nla_total_size(IFNAMSIZ) /* IFLA_QDISC */
//	       + nla_total_size_64bit(sizeof(struct rtnl_link_ifmap))
//	       + nla_total_size(sizeof(struct rtnl_link_stats))
//	       + nla_total_size_64bit(sizeof(struct rtnl_link_stats64))
//	       + nla_total_size(MAX_ADDR_LEN) /* IFLA_ADDRESS */
//	       + nla_total_size(MAX_ADDR_LEN) /* IFLA_BROADCAST */
//	       + nla_total_size(4) /* IFLA_TXQLEN */
//	       + nla_total_size(4) /* IFLA_WEIGHT */
//	       + nla_total_size(4) /* IFLA_MTU */
//	       + nla_total_size(4) /* IFLA_LINK */
//	       + nla_total_size(4) /* IFLA_MASTER */
//	       + nla_total_size(1) /* IFLA_CARRIER */
//	       + nla_total_size(4) /* IFLA_PROMISCUITY */
//	       + nla_total_size(4) /* IFLA_NUM_TX_QUEUES */
//	       + nla_total_size(4) /* IFLA_NUM_RX_QUEUES */
//	       + nla_total_size(4) /* IFLA_GSO_MAX_SEGS */
//	       + nla_total_size(4) /* IFLA_GSO_MAX_SIZE */
//	       + nla_total_size(4) /* IFLA_GRO_MAX_SIZE */
//	       + nla_total_size(4) /* IFLA_TSO_MAX_SIZE */
//	       + nla_total_size(4) /* IFLA_TSO_MAX_SEGS */
//	       + nla_total_size(1) /* IFLA_OPERSTATE */
//	       + nla_total_size(1) /* IFLA_LINKMODE */
//	       + nla_total_size(4) /* IFLA_CARRIER_CHANGES */
//	       + nla_total_size(4) /* IFLA_LINK_NETNSID */
//	       + nla_total_size(4) /* IFLA_GROUP */
//	       + nla_total_size(ext_filter_mask
//			        & RTEXT_FILTER_VF ? 4 : 0) /* IFLA_NUM_VF */
//	       + rtnl_vfinfo_size(dev, ext_filter_mask) /* IFLA_VFINFO_LIST */
//	       + rtnl_port_size(dev, ext_filter_mask) /* IFLA_VF_PORTS + IFLA_PORT_SELF */
//	       + rtnl_link_get_size(dev) /* IFLA_LINKINFO */
//	       + rtnl_link_get_af_size(dev, ext_filter_mask) /* IFLA_AF_SPEC */
//	       + nla_total_size(MAX_PHYS_ITEM_ID_LEN) /* IFLA_PHYS_PORT_ID */
//	       + nla_total_size(MAX_PHYS_ITEM_ID_LEN) /* IFLA_PHYS_SWITCH_ID */
//	       + nla_total_size(IFNAMSIZ) /* IFLA_PHYS_PORT_NAME */
//	       + rtnl_xdp_size() /* IFLA_XDP */
//	       + nla_total_size(4)  /* IFLA_EVENT */
//	       + nla_total_size(4)  /* IFLA_NEW_NETNSID */
//	       + nla_total_size(4)  /* IFLA_NEW_IFINDEX */
//	       + rtnl_proto_down_size(dev)  /* proto down */
//	       + nla_total_size(4)  /* IFLA_TARGET_NETNSID */
//	       + nla_total_size(4)  /* IFLA_CARRIER_UP_COUNT */
//	       + nla_total_size(4)  /* IFLA_CARRIER_DOWN_COUNT */
//	       + nla_total_size(4)  /* IFLA_MIN_MTU */
//	       + nla_total_size(4)  /* IFLA_MAX_MTU */
//	       + rtnl_prop_list_size(dev)
//	       + nla_total_size(MAX_ADDR_LEN) /* IFLA_PERM_ADDRESS */
/*
+[info]  netlink.c:489 FillAttr  - type: IFLA_IFNAME (3) size 7
+[info]  netlink.c:489 FillAttr  - type: IFLA_TXQLEN (13) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_OPERSTATE (16) size 5
+[info]  netlink.c:489 FillAttr  - type: IFLA_LINKMODE (17) size 5
+[info]  netlink.c:489 FillAttr  - type: IFLA_MTU (4) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_MIN_MTU (50) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_MAX_MTU (51) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_GROUP (27) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_PROMISCUITY (30) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_NUM_TX_QUEUES (31) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GSO_MAX_SEGS (40) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GSO_MAX_SIZE (41) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GRO_MAX_SIZE (58) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_TSO_MAX_SIZE (59) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_TSO_MAX_SEGS (60) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_NUM_RX_QUEUES (32) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER (33) size 5
[info]  netlink.c:489 FillAttr  - type: IFLA_QDISC (6) size 12
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_CHANGES (35) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_UP_COUNT (47) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_DOWN_COUNT (48) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_PROTO_DOWN (39) size 5
[info]  netlink.c:489 FillAttr  - type: IFLA_MAP (14) size 36
[info]  netlink.c:489 FillAttr  - type: IFLA_ADDRESS (1) size 10
[info]  netlink.c:489 FillAttr  - type: IFLA_BROADCAST (2) size 10
[info]  netlink.c:489 FillAttr  - type: IFLA_STATS64 (23) size 204
[info]  netlink.c:489 FillAttr  - type: IFLA_STATS (7) size 100
[info]  netlink.c:489 FillAttr  - type: IFLA_XDP (43) size 12
[info]  netlink.c:489 FillAttr  - type: IFLA_AF_SPEC (26) size 804
+[info]  netlink.c:489 FillAttr  - type: IFLA_IFNAME (3) size 11
+[info]  netlink.c:489 FillAttr  - type: IFLA_TXQLEN (13) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_OPERSTATE (16) size 5
+[info]  netlink.c:489 FillAttr  - type: IFLA_LINKMODE (17) size 5
+[info]  netlink.c:489 FillAttr  - type: IFLA_MTU (4) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_MIN_MTU (50) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_MAX_MTU (51) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_GROUP (27) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_PROMISCUITY (30) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_NUM_TX_QUEUES (31) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GSO_MAX_SEGS (40) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GSO_MAX_SIZE (41) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_GRO_MAX_SIZE (58) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_TSO_MAX_SIZE (59) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_TSO_MAX_SEGS (60) size 8
+[info]  netlink.c:489 FillAttr  - type: IFLA_NUM_RX_QUEUES (32) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER (33) size 5
[info]  netlink.c:489 FillAttr  - type: IFLA_QDISC (6) size 13
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_CHANGES (35) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_UP_COUNT (47) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_CARRIER_DOWN_COUNT (48) size 8
[info]  netlink.c:489 FillAttr  - type: IFLA_PROTO_DOWN (39) size 5
[info]  netlink.c:489 FillAttr  - type: IFLA_MAP (14) size 36
+[info]  netlink.c:489 FillAttr  - type: IFLA_ADDRESS (1) size 10
[info]  netlink.c:489 FillAttr  - type: IFLA_BROADCAST (2) size 10
[info]  netlink.c:489 FillAttr  - type: IFLA_STATS64 (23) size 204
[info]  netlink.c:489 FillAttr  - type: IFLA_STATS (7) size 100
[info]  netlink.c:489 FillAttr  - type: IFLA_XDP (43) size 12
+[info]  netlink.c:489 FillAttr  - type: IFLA_PERM_ADDRESS (54) size 10
[info]  netlink.c:489 FillAttr  - type: IFLA_AF_SPEC (26) size 792
+[info]  netlink.c:489 FillAttr  - type: IFLA_PARENT_DEV_NAME (56) size 17
+[info]  netlink.c:489 FillAttr  - type: IFLA_PARENT_DEV_BUS_NAME (57) size 8
*/
	} osdep;

	bool HasAcceptRaRtTable(void);
#else
#endif

	bool tcpdumpOn;
	bool IsTcpdumpOn() {return tcpdumpOn;};

	explicit NetInterface(std::wstring name);
	~NetInterface();

	bool IsUp();
	bool SetUp(bool on);
	bool IsBroadcast();
	bool IsDebug();
	bool SetDebug(bool on);
	bool IsLoopback();
	bool IsPointToPoint();
	bool IsNotrailers();
	bool SetNotrailers(bool on);
	bool IsRunning();
	bool IsNoARP();
	bool SetNoARP(bool on);
	bool IsPromisc();
	bool SetPromisc(bool on);
	bool IsMulticast();
	bool SetMulticast(bool on);
	bool IsAllmulticast();
	bool SetAllmulticast(bool on);

	bool IsCarrierOn();
	bool IsDormant(); // can`t use - setup, or sleep
	bool IsEcho();  // wifi IEEE80211_TX_CTL_REQ_TX_STATUS receiving TX status frames for frames sent on an interface in managed mode
			// see /sys/class/net/<dev>/flags

	#if !defined(__APPLE__) && !defined(__FreeBSD__)	
	bool IsMaster();
	bool IsSlave();
	#endif

	bool SetInterfaceName(const wchar_t * name);
	bool SetMac(const wchar_t * mac);
	bool SetMtu(uint32_t newmtu);
	bool DeleteIp(const wchar_t * ip);
	bool DeleteIp6(const wchar_t * ip);
	bool AddIp(const wchar_t * newip, const wchar_t * mask, const wchar_t * bcip);
	bool AddIp6(const wchar_t * newip, const wchar_t * mask);
	bool ChangeIp(const wchar_t * oldip, const wchar_t * newip, const wchar_t * mask, const wchar_t * bcip);
	bool ChangeIp6(const wchar_t * oldip, const wchar_t * newip, const wchar_t * mask);

	bool TcpDumpStart(const wchar_t * file, bool promisc);
	void TcpDumpStop(void);

	bool UpdateStats(void);

	void Log(void);
	void LogStats(void);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	void LogInet6Stats(void);
	void LogIcmp6Stats(void);
	void LogHardInfo(void);
	void LogIpv4Conf(void);
	void LogIpv6Conf(void);
#endif
};

#endif /* __NETIF_H__ */
