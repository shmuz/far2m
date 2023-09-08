#ifndef __NETROUTE_H__
#define __NETROUTE_H__

#include <string>
#include <vector>
#include <stdint.h>

// permanent   static routes:
//             Debian GNU/Linux: /etc/network/interfaces
//             RHEL/CentOS/Scientifix: /etc/sysconfig/network-scripts/route-<interface name>
//             Gentoo: /etc/conf.d/net

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <common/netlink.h>
struct NextHope {
	uint8_t flags;
	uint8_t weight; // ttl for rtm_flags & RTM_F_CLONED && rtm_type == RTN_MULTICAST
	uint32_t ifindex;
	std::wstring iface;
	struct {
		unsigned char nexthope:1;
		unsigned char encap:1;
		unsigned char gateway:1;
		unsigned char flowfrom:1;
		unsigned char flowto:1;
		unsigned char rtvia:1;
		unsigned char iface:1;
	} valid;
	Encap enc;
	std::wstring gateway;
	uint32_t flowfrom;
	uint32_t flowto;

	uint8_t rtvia_family;
	std::wstring rtvia_addr;

	NextHope(): flags(0), weight(0), ifindex(0), valid({0}), enc({0}) {};
	~NextHope() {};

};
#endif

struct IpRouteInfo {
	std::wstring iface;
	std::wstring destIpandMask;
	std::wstring gateway;
	std::wstring prefsrc;		// Предпочтительный исходный адрес для маршрута
					// в случаях где более одного исходного адреса может быть использовано (несколько сетевых интерфейсов).
	uint32_t flags;
	uint8_t sa_family;
	uint8_t dstprefixlen;
	uint32_t ifnameIndex;
	uint32_t hoplimit;

#if defined(__APPLE__) || defined(__FreeBSD__)
	typedef enum GatewayType {
		IpGatewayType,
		InterfaceGatewayType,
		MacGatewayType,
		OtherGatewayType
	} GatewayType;
#endif

	struct {
		unsigned int flags:1;
		unsigned int iface:1;
		unsigned int destIpandMask:1;
		unsigned int gateway:1;
		unsigned int prefsrc:1;
		unsigned int hoplimit:1;
		unsigned int ifnameIndex:1;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		unsigned int fromsrcIpandMask:1;
		unsigned int metric:1;
		unsigned int tos:1;
		unsigned int icmp6pref:1;
		unsigned int protocol:1;
		unsigned int scope:1;
		unsigned int type:1;
		unsigned int table:1;
		unsigned int dstprefixlen:1;
		unsigned int rtcache:1;
		unsigned int rtmetrics:1;
		unsigned int rtnexthop:1;
		unsigned int encap:1;
		unsigned int rtvia:1;
		unsigned int nhid:1;
#else
		unsigned int expire:1;
		unsigned int mask:1;
		unsigned int parentflags:1;
#endif
	} valid;

	struct {
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		std::wstring fromsrcIpandMask;
		uint32_t metric;
		uint32_t table;
		uint32_t nhid;

		uint8_t tos;
		uint8_t protocol;
		uint8_t scope;
		uint8_t type;
		uint8_t icmp6pref;

		Encap enc;

		uint8_t rtvia_family;
		std::wstring rtvia_addr;

		struct {
			uint32_t rta_clntref;			// количество клиентов, использующих маршрут
			uint32_t rta_lastuse;			// время последнего использования маршрута
			int32_t	rta_expires;			// время жизни маршрута
			int32_t rta_error;			// код ошибки со знаком минус
			uint32_t rta_used;			// количество использований маршрута
			uint32_t rta_id;			// идентификатор маршрута (это идентификатор IP-пакета)
			uint32_t rta_ts;			// временная метка
			uint32_t rta_tsage;			// это возраст временной метки, содержит информацию о времени последнего обновления маршрута в секундах
		} rtcache;
		struct {
			uint32_t unspec;               // RTAX_UNSPEC
			uint32_t lock;                 // RTAX_LOCK				
			uint32_t mtu;                  // RTAX_MTU				
			uint32_t window;               // RTAX_WINDOW			
			uint32_t rtt;                  // RTAX_RTT			Время приема-передачи (англ. round-trip time, RTT) — 
										// это время, затраченное на отправку сигнала, плюс 
										// время, которое требуется для подтверждения, 
										// что сигнал был получен. Это время задержки, 
										// следовательно, состоит из времени передачи сигнала
										// между двумя точками.
			uint32_t rttvar;               // RTAX_RTTVAR		это оценка дисперсии времени задержки передачи
			uint32_t ssthresh;             // RTAX_SSTHRESH		это порог медленного старта. Медленный старт — часть стратегии управления окном перегрузки 
										// алгоритм медленного старта работает за счёт увеличения
										// окна TCP каждый раз когда получено подтверждение, 
										// то есть увеличивает размер окна в зависимости от количества
										// подтверждённых сегментов. Это происходит до тех пор,
										// пока для какого-то сегмента не будет получено подтверждение или будет достигнуто какое-то заданное пороговое значение.
										// сначала с размером окна перегрузки (congestion window — CWND) 1, 2 или 10[2] сегментов и увеличивает его на один размер сегмента (segment size — SS) для каждого полученного ACK.
										// Когда происходит потеря, половина текущего CWND сохраняется в виде порога медленного старта (SSThresh) и медленный старт начинается снова от своего первоначального CWND. Как только CWND достигает SSThresh, TCP переходит в режим предотвращения перегрузки, где каждый ACK увеличивает CWND на SS * SS / CWND. Это приводит к линейному увеличению CWND.
										// https://ru.wikipedia.org/wiki/Медленный_старт
			uint32_t cwnd;                 // RTAX_CWND		это размер окна перегрузки
			uint32_t advmss;               // RTAX_ADVMSS		это максимальный размер сегмента TCP (MSS (англ. Maximum segment size) является параметром протокола TCP и определяет максимальный размер полезного блока данных в байтах для TCP-пакета (сегмента). Таким образом этот параметр не учитывает длину заголовков TCP и IP)
			uint32_t reordering;           // RTAX_REORDERING	это метрика переупорядочивания
			uint32_t hoplimit;             // RTAX_HOPLIMIT		это предельное количество прыжков для пакета
			uint32_t initcwnd;             // RTAX_INITCWND		начальный размер окна перегрузки (congestion window — CWND)
			uint32_t features;             // RTAX_FEATURES			
			uint32_t rto_min;              // RTAX_RTO_MIN			
			uint32_t initrwnd;             // RTAX_INITRWND			
			uint32_t quickack;             // RTAX_QUICKACK			
			uint32_t congctl;              // RTAX_CC_ALGO			
			uint32_t fastopen_no_cookie;   // RTAX_FASTOPEN_NO_COOKIE это флаг быстрого открытия 
		} rtmetrics;

		std::vector<NextHope> nhs;
#else
		uint32_t parentflags;
		uint32_t expire;
		GatewayType gateway_type;
#endif
	} osdep;

	bool CreateIpRoute(void);
	bool DeleteIpRoute(void);
	bool ChangeIpRoute(IpRouteInfo & ipr);

	void Log(void) const;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	void LogRtCache(void) const;
#endif
	IpRouteInfo();
	~IpRouteInfo();
private:
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	const char * GetEncap(const Encap & enc) const;
	const char * GetNextHopes(void) const;
#endif
};

#if !defined(__APPLE__) && !defined(__FreeBSD__)
struct arproute_cacheinfo {
	uint32_t ndm_confirmed;
	uint32_t ndm_used;
	uint32_t ndm_updated;
	uint32_t ndm_refcnt;
};
#endif

struct ArpRouteInfo {
	std::wstring iface;
	std::wstring ip;
	std::wstring mac;
	uint32_t flags;
	uint32_t sa_family;
	uint32_t ifnameIndex;

	struct {
		unsigned int iface:1;
		unsigned int ip:1;
		unsigned int mac:1;
		unsigned int flags:1;
		unsigned int ifnameIndex:1;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		unsigned int state:1;
		unsigned int vlan:1;
		unsigned int port:1;
		unsigned int protocol:1;
		unsigned int type:1;
		unsigned int nh_id:1;
		unsigned int flags_ext:1;
		unsigned int vni:1;
		unsigned int master:1;
		unsigned int probes:1;
		unsigned int ci:1;
#endif
	} valid;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	uint32_t probes;
	uint32_t hwType;
	uint16_t state;
	uint16_t vlan;
	uint16_t port;
	uint8_t protocol;
	uint8_t type;
	uint32_t nh_id;
	uint32_t flags_ext;
	uint32_t vni;
	uint32_t master;
	struct arproute_cacheinfo ci;
	void LogCacheInfo(void) const;
#endif
	bool Create(void);
	bool Delete(void);
	bool Change(ArpRouteInfo & arpr);

	void Log(void) const;

	ArpRouteInfo();
	~ArpRouteInfo();
};

#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <common/netlink.h>

struct uid_range {
	__u32		start;
	__u32		end;
};

struct port_range {
	__u16		start;
	__u16		end;
};

struct RuleRouteInfo {
	std::wstring rule;
	std::wstring fromIpandMask;
	std::wstring toIpandMask;
	std::wstring gateway;
	std::wstring iiface;
	std::wstring oiface;

	uint32_t flags;

	uint16_t type;
	uint8_t action;
	uint8_t family;
	uint8_t tos;
	uint8_t protocol;
	uint8_t ip_protocol;
	uint8_t l3mdev;			// iif or oif is l3mdev goto its table, 
					// L3 Master Device (l3mdev) - это механизм ядра Linux, который позволяет создавать
					// виртуальные маршрутизаторы (VRF) и связывать их с конкретными интерфейсами.
					// L3 Master Device - это интерфейс, который связывает VRF с физическим интерфейсом.
	uint32_t table;
	uint32_t goto_priority;
	uint32_t suppress_prefixlength; //FRA_SUPPRESS_PREFIXLEN valid if suppress_prefixlength != -1
	uint32_t suppress_ifgroup; // FRA_SUPPRESS_IFGROUP  valid if suppress_ifgroup != -1
	uint32_t priority; // FRA_PRIORITY
	uint32_t fwmark;
	uint32_t fwmask;

				// каждый маршрут может быть назначен области (realm) (0 - если область неизвестна)
	uint32_t flowfrom;	// realms Для каждого пакета ядро ​​вычисляет набор областей, исходной области и области назначения, используя следующий алгоритм:
	uint32_t flowto;	// 1. Если у маршрута есть область, целевая область пакета устанавливается в нее.
				// 2. Если у правила есть исходная область, исходная область пакета устанавливается на нее.
				// 3. Если целевая область не была получена из маршрута, а в правиле есть целевая область, установите целевую область из правила.
				// 4. Если хотя бы одна из областей все еще неизвестна, ядро ​​находит обратный маршрут к источнику пакета.
				// 5. Если исходная область все еще неизвестна, получите ее обратным путем.
				// 6. Если одна из областей все еще неизвестна, поменяйте местами области обратных маршрутов и снова примените шаг 2.
				// После завершения этой процедуры мы знаем, из какой области пришел пакет и в какую область он собирается распространиться. 
				// Если какая-либо из областей неизвестна, она инициализируется нулем (или неизвестной областью).

	uint64_t tun_id;
	struct uid_range uid_range;
	struct port_range sport_range;
	struct port_range dport_range;
	struct {
		unsigned int fromIpandMask:1;
		unsigned int toIpandMask:1;
		unsigned int gateway:1;
		unsigned int iiface:1;
		unsigned int oiface:1;
		unsigned int protocol:1;
		unsigned int ip_protocol:1;
		unsigned int l3mdev:1;
		unsigned int table:1;
		unsigned int goto_priority:1;
		unsigned int suppress_prefixlength:1;
		unsigned int suppress_ifgroup:1;
		unsigned int priority:1;
		unsigned int fwmark:1;
		unsigned int fwmask:1;
		unsigned int flowfrom:1;
		unsigned int flowto:1;
		unsigned int tun_id:1;
		unsigned int uid_range:1;
		unsigned int sport_range:1;
		unsigned int dport_range:1;
	} valid;

	bool operator<(const RuleRouteInfo & rule) const { return rule.priority > priority; };

	bool DeleteRule(void);
	bool CreateRule(void);
	bool ChangeRule(RuleRouteInfo & ipr);

	void ToRuleString(bool skipExtInfo = false);
	void Log(void) const;

	RuleRouteInfo();
	~RuleRouteInfo();
};
#endif

#endif /* __NETROUTE_H__ */
