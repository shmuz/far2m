#include "netutils.h"

const char * iprotocolname(uint8_t proto)
{
	// out of range IPPROTO_MPTCP = 262, /* Multipath TCP connection		*/
	static const char *protocol_names[] = {
	"ip",                /* 0 (IPPROTO_HOPOPTS, IPv6 Hop-by-Hop Option) */
	"icmp",              /* 1 (IPPROTO_ICMP, Internet Control Message) */
	"igmp",              /* 2 (IPPROTO_IGMP, Internet Group Management) */
	"ggp",               /* 3 (Gateway-to-Gateway) */
	"ipencap",           /* 4 (IPPROTO_IPV4, IPv4 encapsulation) */
	"st",                /* 5 (Stream, ST datagram mode) */
	"tcp",               /* 6 (IPPROTO_TCP, Transmission Control) */
	"cbt",               /* 7 (CBT) */
	"egp",               /* 8 (IPPROTO_EGP, Exterior Gateway Protocol) */
	"igp",               /* 9 (IPPROTO_PIGP, "any private interior gateway
	                      *   (used by Cisco for their IGRP)")
	                      */
	"bbn-rcc-mon",       /* 10 (BBN RCC Monitoring) */
	"nvp-ii",            /* 11 (Network Voice Protocol) */
	"pup",               /* 12 (PARC universal packet protocol) */
	"argus",             /* 13 (ARGUS) */
	"emcon",             /* 14 (EMCON) */
	"xnet",              /* 15 (Cross Net Debugger) */
	"chaos",             /* 16 (Chaos) */
	"udp",               /* 17 (IPPROTO_UDP, User Datagram) */
	"mux",               /* 18 (Multiplexing) */
	"dcn-meas",          /* 19 (DCN Measurement Subsystems) */
	"hmp",               /* 20 (Host Monitoring) */
	"prm",               /* 21 (Packet Radio Measurement) */
	"xns-idp",           /* 22 (XEROX NS IDP) */
	"trunk-1",           /* 23 (Trunk-1) */
	"trunk-2",           /* 24 (Trunk-2) */
	"leaf-1",            /* 25 (Leaf-1) */
	"leaf-2",            /* 26 (Leaf-2) */
	"rdp",               /* 27 (Reliable Data Protocol) */
	"irtp",              /* 28 (Internet Reliable Transaction) */
	"iso-tp4",           /* 29 (ISO Transport Protocol Class 4) */
	"netblt",            /* 30 (Bulk Data Transfer Protocol) */
	"mfe-nsp",           /* 31 (MFE Network Services Protocol) */
	"merit-inp",         /* 32 (MERIT Internodal Protocol) */
	"dccp",              /* 33 (IPPROTO_DCCP, Datagram Congestion
	                      *    Control Protocol)
	                      */
	"3pc",               /* 34 (Third Party Connect Protocol) */
	"idpr",              /* 35 (Inter-Domain Policy Routing Protocol) */
	"xtp",               /* 36 (Xpress Transfer Protocol) */
	"ddp",               /* 37 (Datagram Delivery Protocol) */
	"idpr-cmtp",         /* 38 (IDPR Control Message Transport Proto) */
	"tp++",              /* 39 (TP++ Transport Protocol) */
	"il",                /* 40 (IL Transport Protocol) */
	"ipv6",              /* 41 (IPPROTO_IPV6, IPv6 encapsulation) */
	"sdrp",              /* 42 (Source Demand Routing Protocol) */
	"ipv6-route",        /* 43 (IPPROTO_ROUTING, Routing Header for IPv6) */
	"ipv6-frag",         /* 44 (IPPROTO_FRAGMENT, Fragment Header for
	                      *    IPv6)
	                      */
	"idrp",              /* 45 (Inter-Domain Routing Protocol) */
	"rsvp",              /* 46 (IPPROTO_RSVP, Reservation Protocol) */
	"gre",               /* 47 (IPPROTO_GRE, Generic Routing
	                      *    Encapsulation)
	                      */
	"dsr",               /* 48 (Dynamic Source Routing Protocol) */
	"bna",               /* 49 (BNA) */
	"esp",               /* 50 (IPPROTO_ESP, Encap Security Payload) */
	"ah",                /* 51 (IPPROTO_AH, Authentication Header) */
	"i-nlsp",            /* 52 (Integrated Net Layer Security TUBA) */
	"swipe",             /* 53 (IP with Encryption) */
	"narp",              /* 54 (NBMA Address Resolution Protocol) */
	"mobile",            /* 55 (IPPROTO_MOBILE, IP Mobility) */
	"tlsp",              /* 56 (Transport Layer Security Protocol using
	                      *    Kryptonet key management)
	                      */
	"skip",              /* 57 (SKIP) */
	"ipv6-icmp",         /* 58 (IPPROTO_ICMPV6, ICMP for IPv6) */
	"ipv6-nonxt",        /* 59 (IPPROTO_NONE, No Next Header for IPv6) */
	"ipv6-opts",         /* 60 (IPPROTO_DSTOPTS, Destination Options for
	                      *    IPv6)
	                      */
	"ahip",              /* 61 (IPPROTO_AHIP any host internal protocol) */
	"cftp",              /* 62 (IPPROTO_MOBILITY_OLD, CFTP, see the note
	                      *    in ipproto.h)
	                      */
	"hello",             /* 63 IPPROTO_HELLO "hello" routing protocol */
	"sat-expak",         /* 64 (SATNET and Backroom EXPAK) */
	"kryptolan",         /* 65 (Kryptolan) */
	"rvd",               /* 66 (MIT Remote Virtual Disk Protocol) */
	"ippc",              /* 67 (Internet Pluribus Packet Core) */
	"adfs",              /* 68 (IPPROTO_ADFS any distributed file system) */
	"sat-mon",           /* 69 (SATNET Monitoring) */
	"visa",              /* 70 (VISA Protocol) */
	"ipcv",              /* 71 (Internet Packet Core Utility) */
	"cpnx",              /* 72 (Computer Protocol Network Executive) */
	"rspf",              /* 73 (Radio Shortest Path First, CPHB -- Computer
	                      *    Protocol Heart Beat -- in IANA)
	                      */
	"wsn",               /* 74 (Wang Span Network) */
	"pvp",               /* 75 (Packet Video Protocol) */
	"br-sat-mon",        /* 76 (Backroom SATNET Monitoring) */
	"sun-nd",            /* 77 (IPPROTO_ND, SUN ND PROTOCOL-Temporary) */
	"wb-mon",            /* 78 (WIDEBAND Monitoring) */
	"wb-expak",          /* 79 (WIDEBAND EXPAK) */
	"iso-ip",            /* 80 (ISO Internet Protocol) */
	"vmtp",              /* 81 (Versatile Message Transport) */
	"secure-vmtp",       /* 82 (Secure VMTP) */
	"vines",             /* 83 (VINES) */
	"ttp",               /* 84 (Transaction Transport Protocol, also IPTM --
	                      *    Internet Protocol Traffic Manager)
	                      */
	"nsfnet-igp",        /* 85 (NSFNET-IGP) */
	"dgp",               /* 86 (Dissimilar Gateway Protocol) */
	"tcf",               /* 87 (TCF) */
	"eigrp",             /* 88 (IPPROTO_EIGRP, Cisco EIGRP) */
	"ospf",              /* 89 (IPPROTO_OSPF, Open Shortest Path First
	                      *    IGP)
	                      */
	"sprite-rpc",        /* 90 (Sprite RPC Protocol) */
	"larp",              /* 91 (Locus Address Resolution Protocol) */
	"mtp",               /* 92 (Multicast Transport Protocol) */
	"ax.25",             /* 93 (AX.25 Frames) */
	"ipip",              /* 94 (IP-within-IP Encapsulation Protocol) */
	"micp",              /* 95 (Mobile Internetworking Control Pro.) */
	"scc-sp",            /* 96 (Semaphore Communications Sec. Pro.) */
	"etherip",           /* 97 (Ethernet-within-IP Encapsulation) */
	"encap",             /* 98 (Encapsulation Header) */
	"apes",              /* 99 (IPPROTO_APES any private encryption scheme) */
	"gmtp",              /* 100 (GMTP) */
	"ifmp",              /* 101 (Ipsilon Flow Management Protocol) */
	"pnni",              /* 102 (PNNI over IP) */
	"pim",               /* 103 (IPPROTO_PIM, Protocol Independent
	                      *     Multicast)
	                      */
	"aris",              /* 104 (ARIS) */
	"scps",              /* 105 (SCPS) */
	"qnx",               /* 106 (QNX) */
	"a/n",               /* 107 (Active Networks) */
	"ipcomp",            /* 108 (IPPROTO_IPCOMP, IP Payload Compression
	                      *     Protocol)
	                      */
	"snp",               /* 109 (Sitara Networks Protocol) */
	"compaq-peer",       /* 110 (Compaq Peer Protocol) */
	"ipx-in-ip",         /* 111 (IPX in IP) */
	"vrrp",              /* 112 (IPPROTO_VRRP, Virtual Router Redundancy
	                      *     Protocol)
	                      */
	"pgm",               /* 113 (IPPROTO_PGM, PGM Reliable Transport
	                      *     Protocol)
	                      */
	"any0-hop",          /* 114 (any 0-hop protocol) */
	"l2tp",              /* 115 (Layer Two Tunneling Protocol) */
	"ddx",               /* 116 (D-II Data Exchange (DDX)) */
	"iatp",              /* 117 (Interactive Agent Transfer Protocol) */
	"stp",               /* 118 (Schedule Transfer Protocol) */
	"srp",               /* 119 (SpectraLink Radio Protocol) */
	"uti",               /* 120 (UTI) */
	"smp",               /* 121 (Simple Message Protocol) */
	"sm",                /* 122 (Simple Multicast Protocol) */
	"ptp",               /* 123 (Performance Transparency Protocol) */
	"isis",              /* 124 (ISIS over IPv4) */
	"fire",              /* 125 (FIRE) */
	"crtp",              /* 126 (Combat Radio Transport Protocol) */
	"crudp",             /* 127 (Combat Radio User Datagram) */
	"sscopmce",          /* 128 (SSCOPMCE) */
	"iplt",              /* 129 (IPLT) */
	"sps",               /* 130 (Secure Packet Shield) */
	"pipe",              /* 131 (Private IP Encapsulation within IP) */
	"sctp",              /* 132 (IPPROTO_SCTP, Stream Control Transmission
	                      *     Protocol)
	                      */
	"fc",                /* 133 (Fibre Channel) */
	"rsvp-e2e-ignore",   /* 134 (RSVP-E2E-IGNORE) */
	"mobility-header",   /* 135 (IPPROTO_MOBILITY, Mobility Header) */
	"udplite",           /* 136 (UDPLite) */
	"mpls-in-ip",        /* 137 (MPLS-in-IP) */
	"manet",             /* 138 (MANET Protocols) */
	"hip",               /* 139 (Host Identity Protocol) */
	"shim6",             /* 140 (Shim6 Protocol) */
	"wesp",              /* 141 (Wrapped Encapsulating Security Payload) */
	"rohc",              /* 142 (Robust Header Compression) */
	"ethernet"           /* 143 (IPPROTO_ETHERNET Ethernet-within-IPv6 Encapsulation) */
	};

	if( proto < sizeof(protocol_names)/sizeof(protocol_names[0]) )
			return protocol_names[proto];

	if( proto == IPPROTO_RAW )
		return "raw";

	return "";
}

const char * ifflagsname(uint32_t flags)
{
	static char buf[sizeof("UP, BROADCAST, DEBUG, LOOPBACK, POINTOPOINT, NOTRAILERS, RUNNING, NOARP, PROMISC, ALLMULTI, OACTIVE, SIMPLEX, MULTICAST, PORTSEL, AUTOMEDIA, DYNAMIC, LOWER_UP, DORMANT, ECHO")] = {0};
	buf[0] = '\0';
	flags = addflag(buf, "UP", flags, IFF_UP);
	flags = addflag(buf, "BROADCAST", flags, IFF_BROADCAST);
	flags = addflag(buf, "DEBUG", flags, IFF_DEBUG);
	flags = addflag(buf, "LOOPBACK", flags, IFF_LOOPBACK);
	flags = addflag(buf, "POINTOPOINT", flags, IFF_POINTOPOINT);
	flags = addflag(buf, "NOTRAILERS", flags, IFF_NOTRAILERS);
	flags = addflag(buf, "RUNNING", flags, IFF_RUNNING);
	flags = addflag(buf, "NOARP", flags, IFF_NOARP);
	flags = addflag(buf, "PROMISC", flags, IFF_PROMISC);
	flags = addflag(buf, "ALLMULTI", flags, IFF_ALLMULTI);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	flags = addflag(buf, "MASTER", flags, IFF_MASTER);
	flags = addflag(buf, "SLAVE", flags, IFF_SLAVE);
	flags = addflag(buf, "PORTSEL", flags, IFF_PORTSEL);
	flags = addflag(buf, "AUTOMEDIA", flags, IFF_AUTOMEDIA);
	flags = addflag(buf, "DYNAMIC", flags, IFF_DYNAMIC);
	flags = addflag(buf, "LOWER_UP", flags, IFF_LOWER_UP);
	flags = addflag(buf, "DORMANT", flags, IFF_DORMANT);
	flags = addflag(buf, "ECHO", flags, IFF_ECHO);
#else
	flags = addflag(buf, "OACTIVE", flags, IFF_OACTIVE); /* transmission in progress */
	flags = addflag(buf, "SIMPLEX", flags, IFF_SIMPLEX); /* can't hear own transmissions */
	flags = addflag(buf, "LINK0", flags, IFF_LINK0); /* per link layer defined bit */
	flags = addflag(buf, "LINK1", flags, IFF_LINK1); /* per link layer defined bit */
	flags = addflag(buf, "ALTPHYS", flags, IFF_ALTPHYS); /* use alternate physical connection #define IFF_ALTPHYS     IFF_LINK2 */
#endif
	flags = addflag(buf, "MULTICAST", flags, IFF_MULTICAST);
	return buf;
}

const char * ipfamilyname(char family)
{
	switch(family) {
	case AF_INET:
		return "inet";
	case AF_INET6:
		return "inet6";
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	case AF_PACKET:
#else
	case AF_LINK:
#endif
		return "link";
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	case AF_MPLS:
		return "mpls";
	case AF_BRIDGE:
		return "bridge";
#endif
	};
	return familyname(family);
}

#define CASE_FAMILY(family) \
	case family: return #family

const char * familyname(char family)
{
	//if( family >= AF_MAX )
	//	return "UNKNOWN";

	switch( (unsigned char)family ) {
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	CASE_FAMILY( AF_UNSPEC );
	CASE_FAMILY( AF_UNIX );      /* Unix domain sockets	      */
	CASE_FAMILY( AF_INET );      /* Internet IP Protocol	      */
	CASE_FAMILY( AF_AX25 );      /* Amateur Radio AX.25	      */
	CASE_FAMILY( AF_IPX );       /* Novell IPX 		      */
	CASE_FAMILY( AF_APPLETALK ); /* AppleTalk DDP 		      */
	CASE_FAMILY( AF_NETROM	);   /* Amateur Radio NET/ROM 	      */
	CASE_FAMILY( AF_BRIDGE	);   /* Multiprotocol bridge 	      */
	CASE_FAMILY( AF_ATMPVC	);   /* ATM PVCs		      */
	CASE_FAMILY( AF_X25 );       /* Reserved for X.25 project     */
	CASE_FAMILY( AF_INET6 );     /* IP version 6		      */
	CASE_FAMILY( AF_ROSE );      /* Amateur Radio X.25 PLP	      */
	CASE_FAMILY( AF_DECnet );    /* Reserved for DECnet project   */
	CASE_FAMILY( AF_NETBEUI );   /* Reserved for 802.2LLC project */
	CASE_FAMILY( AF_SECURITY );  /* Security callback pseudo AF   */
	CASE_FAMILY( AF_KEY );       /* PF_KEY key management API */
	CASE_FAMILY( AF_NETLINK	);
	CASE_FAMILY( AF_PACKET	);   /* Packet family		      */
	CASE_FAMILY( AF_ASH );       /* Ash			      */
	CASE_FAMILY( AF_ECONET	);   /* Acorn Econet			*/
	CASE_FAMILY( AF_ATMSVC	);   /* ATM SVCs			*/
	CASE_FAMILY( AF_RDS );       /* RDS sockets 			*/
	CASE_FAMILY( AF_SNA );       /* Linux SNA Project (nutters!) */
	CASE_FAMILY( AF_IRDA );      /* IRDA sockets			*/
	CASE_FAMILY( AF_PPPOX );     /* PPPoX sockets		*/
	CASE_FAMILY( AF_WANPIPE	);   /* Wanpipe API Sockets */
	CASE_FAMILY( AF_LLC );       /* Linux LLC			*/
	CASE_FAMILY( AF_IB );        /* Native InfiniBand address	*/
	CASE_FAMILY( AF_MPLS );      /* MPLS */
	CASE_FAMILY( AF_CAN );       /* Controller Area Network      */
	CASE_FAMILY( AF_TIPC );      /* TIPC sockets			*/
	CASE_FAMILY( AF_BLUETOOTH ); /* Bluetooth sockets 		*/
	CASE_FAMILY( AF_IUCV );      /* IUCV sockets			*/
	CASE_FAMILY( AF_RXRPC );     /* RxRPC sockets 		*/
	CASE_FAMILY( AF_ISDN );      /* mISDN sockets 		*/
	CASE_FAMILY( AF_PHONET );    /* Phonet sockets		*/
	CASE_FAMILY( AF_IEEE802154 );/* IEEE802154 sockets		*/
	CASE_FAMILY( AF_CAIF );      /* CAIF sockets			*/
	CASE_FAMILY( AF_ALG );       /* Algorithm sockets		*/
	CASE_FAMILY( AF_NFC );       /* NFC sockets			*/
	CASE_FAMILY( AF_VSOCK );     /* vSockets			*/
	CASE_FAMILY( AF_KCM );       /* Kernel Connection Multiplexor*/
	CASE_FAMILY( AF_QIPCRTR	);   /* Qualcomm IPC Router          */
	CASE_FAMILY( AF_SMC );       /* smc sockets: reserve number for
			              * PF_SMC protocol family that
			              * reuses AF_INET address family */
#ifdef AF_XDP
	CASE_FAMILY( AF_XDP );       /* XDP sockets			*/
#endif
#ifdef AF_MCTP
	CASE_FAMILY( AF_MCTP );      /* Management component
			              * transport protocol */
#endif

	CASE_FAMILY( RTNL_FAMILY_IPMR );
	CASE_FAMILY( RTNL_FAMILY_IP6MR );

#else // apple freebsd

	CASE_FAMILY( AF_UNSPEC       ); /* unspecified */
	CASE_FAMILY( AF_UNIX         ); /* local to host (pipes) */
	CASE_FAMILY( AF_INET         ); /* internetwork: UDP, TCP, etc. */
	CASE_FAMILY( AF_IMPLINK      ); /* arpanet imp addresses */
	CASE_FAMILY( AF_PUP          ); /* pup protocols: e.g. BSP */
	CASE_FAMILY( AF_CHAOS        ); /* mit CHAOS protocols */
	CASE_FAMILY( AF_NS           ); /* XEROX NS protocols */
	CASE_FAMILY( AF_ISO          ); /* ISO protocols */
	CASE_FAMILY( AF_ECMA         ); /* European computer manufacturers */
	CASE_FAMILY( AF_DATAKIT      ); /* datakit protocols */
	CASE_FAMILY( AF_CCITT        ); /* CCITT protocols, X.25 etc */
	CASE_FAMILY( AF_SNA          ); /* IBM SNA */
	CASE_FAMILY( AF_DECnet       ); /* DECnet */
	CASE_FAMILY( AF_DLI          ); /* DEC Direct data link interface */
	CASE_FAMILY( AF_LAT          ); /* LAT */
	CASE_FAMILY( AF_HYLINK       ); /* NSC Hyperchannel */
	CASE_FAMILY( AF_APPLETALK    ); /* Apple Talk */
	CASE_FAMILY( AF_ROUTE        ); /* Internal Routing Protocol */
	CASE_FAMILY( AF_LINK         ); /* Link layer interface */
	CASE_FAMILY( pseudo_AF_XTP   ); /* eXpress Transfer Protocol (no AF) */
	CASE_FAMILY( AF_COIP         ); /* connection-oriented IP, aka ST II */
	CASE_FAMILY( AF_CNT          ); /* Computer Network Technology */
	CASE_FAMILY( pseudo_AF_RTIP  ); /* Help Identify RTIP packets */
	CASE_FAMILY( AF_IPX          ); /* Novell Internet Protocol */
	CASE_FAMILY( AF_SIP          ); /* Simple Internet Protocol */
	CASE_FAMILY( pseudo_AF_PIP   ); /* Help Identify PIP packets */
	CASE_FAMILY( AF_NDRV         ); /* Network Driver 'raw' access */
	CASE_FAMILY( AF_ISDN         ); /* Integrated Services Digital Network */
	CASE_FAMILY( pseudo_AF_KEY   ); /* Internal key-management function */
	CASE_FAMILY( AF_INET6        ); /* IPv6 */
	CASE_FAMILY( AF_NATM         ); /* native ATM access */
	CASE_FAMILY( AF_SYSTEM       ); /* Kernel event messages */
	CASE_FAMILY( AF_NETBIOS      ); /* NetBIOS */
	CASE_FAMILY( AF_PPP          ); /* PPP communication protocol */
	CASE_FAMILY( pseudo_AF_HDRCMPLT ); /* Used by BPF to not rewrite headers
	                                    *  in interface output routine */
	CASE_FAMILY( AF_AFP          ); /* Used by AFP */
	CASE_FAMILY( AF_IEEE80211    ); /* IEEE 802.11 protocol */
	CASE_FAMILY( AF_UTUN         );
	CASE_FAMILY( AF_MULTIPATH    );
	CASE_FAMILY( AF_VSOCK        ); /* VM Sockets */

#endif
	};

	return "ERROR";
}

#define CASE_ARPHRD(ar_hrd, info) \
	case ar_hrd: return #ar_hrd ", " #info

const char * arphdrname(uint16_t ar_hrd)
{
	switch( ar_hrd ) {
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	CASE_ARPHRD( ARPHRD_NETROM, "(from KA9Q: NET/ROM pseudo)" );
	CASE_ARPHRD( ARPHRD_ETHER, "(Ethernet hardware format)" );
	CASE_ARPHRD( ARPHRD_EETHER, "(Experimental Ethernet)" );
	CASE_ARPHRD( ARPHRD_AX25, "(AX.25 Level 2)" );
	CASE_ARPHRD( ARPHRD_PRONET, "(PROnet token ring)" );
	CASE_ARPHRD( ARPHRD_CHAOS, "(Chaosnet)" );
	CASE_ARPHRD( ARPHRD_IEEE802, "(IEEE 802.2 Ethernet/TR/TB (token-ring hardware format))" );
	CASE_ARPHRD( ARPHRD_ARCNET, "(ARCnet)" );
	CASE_ARPHRD( ARPHRD_APPLETLK, "(APPLEtalk)" );
	CASE_ARPHRD( ARPHRD_DLCI, "(Frame Relay DLCI)" );
	CASE_ARPHRD( ARPHRD_ATM, "(ATM)" );
	CASE_ARPHRD( ARPHRD_METRICOM, "(Metricom STRIP (new IANA id))" );
	CASE_ARPHRD( ARPHRD_IEEE1394, "(IEEE 1394 IPv4 - RFC 2734)" );
	CASE_ARPHRD( ARPHRD_EUI64, "(EUI-64)" );
	CASE_ARPHRD( ARPHRD_INFINIBAND, "(InfiniBand)" );
	CASE_ARPHRD( ARPHRD_SLIP, "" );
	CASE_ARPHRD( ARPHRD_CSLIP, "" );
	CASE_ARPHRD( ARPHRD_SLIP6, "" );
	CASE_ARPHRD( ARPHRD_CSLIP6, "" );
	CASE_ARPHRD( ARPHRD_RSRVD, "(Notional KISS type)" );
	CASE_ARPHRD( ARPHRD_ADAPT, "" );
	CASE_ARPHRD( ARPHRD_ROSE, "" );
	CASE_ARPHRD( ARPHRD_X25, "(CCITT X.25)" );
	CASE_ARPHRD( ARPHRD_HWX25, "(Boards with X.25 in firmware)" );
	CASE_ARPHRD( ARPHRD_CAN, "(Controller Area Network)" );
	CASE_ARPHRD( ARPHRD_MCTP, "" );
	CASE_ARPHRD( ARPHRD_PPP, "" );
	CASE_ARPHRD( ARPHRD_CISCO, "(Cisco HDLC)" );
	CASE_ARPHRD( ARPHRD_LAPB, "(LAPB)" );
	CASE_ARPHRD( ARPHRD_DDCMP, "(Digital's DDCMP protocol)" );
	CASE_ARPHRD( ARPHRD_RAWHDLC, "(Raw HDLC)" );
	CASE_ARPHRD( ARPHRD_RAWIP, "(Raw IP)" );
	CASE_ARPHRD( ARPHRD_TUNNEL, "(IPIP tunnel)" );
	CASE_ARPHRD( ARPHRD_TUNNEL6, "(IP6IP6 tunnel)" );
	CASE_ARPHRD( ARPHRD_FRAD, "(Frame Relay Access Device)" );
	CASE_ARPHRD( ARPHRD_SKIP, "(SKIP vif)" );
	CASE_ARPHRD( ARPHRD_LOOPBACK, "(Loopback device)" );
	CASE_ARPHRD( ARPHRD_LOCALTLK, "(Localtalk device)" );
	CASE_ARPHRD( ARPHRD_FDDI, "(Fiber Distributed Data Interface)" );
	CASE_ARPHRD( ARPHRD_BIF, "(AP1000 BIF)" );
	CASE_ARPHRD( ARPHRD_SIT, "(sit0 device - IPv6-in-IPv4)" );
	CASE_ARPHRD( ARPHRD_IPDDP, "(IP over DDP tunneller)" );
	CASE_ARPHRD( ARPHRD_IPGRE, "(GRE over IP)" );
	CASE_ARPHRD( ARPHRD_PIMREG, "(PIMSM register interface)" );
	CASE_ARPHRD( ARPHRD_HIPPI, "(High Performance Parallel Interface)" );
	CASE_ARPHRD( ARPHRD_ASH, "(Nexus 64Mbps Ash)" );
	CASE_ARPHRD( ARPHRD_ECONET, "(Acorn Econet)" );
	CASE_ARPHRD( ARPHRD_IRDA, "(Linux-IrDA)" );
	CASE_ARPHRD( ARPHRD_FCPP, "(Point to point fibrechannel)" );
	CASE_ARPHRD( ARPHRD_FCAL, "(Fibrechannel arbitrated loop)" );
	CASE_ARPHRD( ARPHRD_FCPL, "(Fibrechannel public loop)" );
	CASE_ARPHRD( ARPHRD_FCFABRIC, "(Fibrechannel fabric)" );
	CASE_ARPHRD( ARPHRD_IEEE802_TR, "(Magic type ident for TR)" );
	CASE_ARPHRD( ARPHRD_IEEE80211, "(IEEE 802.11)" );
	CASE_ARPHRD( ARPHRD_IEEE80211_PRISM, "(IEEE 802.11 + Prism2 header)" );
	CASE_ARPHRD( ARPHRD_IEEE80211_RADIOTAP, "(IEEE 802.11 + radiotap header)" );
	CASE_ARPHRD( ARPHRD_IEEE802154, "" );	  		
	CASE_ARPHRD( ARPHRD_IEEE802154_MONITOR, "(IEEE 802.15.4 network monitor)" );
	CASE_ARPHRD( ARPHRD_PHONET, "(PhoNet media type)" );
	CASE_ARPHRD( ARPHRD_PHONET_PIPE, "(PhoNet pipe header)" );
	CASE_ARPHRD( ARPHRD_CAIF, "(CAIF media type)" );
	CASE_ARPHRD( ARPHRD_IP6GRE, "(GRE over IPv6)" );
	CASE_ARPHRD( ARPHRD_NETLINK, "(Netlink header)" );
	CASE_ARPHRD( ARPHRD_6LOWPAN, "(IPv6 over LoWPAN)" );
	CASE_ARPHRD( ARPHRD_VSOCKMON, "(Vsock monitor header)" );
	CASE_ARPHRD( ARPHRD_VOID, "(Void type, nothing is known)" );
	CASE_ARPHRD( ARPHRD_NONE, "(zero header length)" );
#else
	CASE_ARPHRD( ARPHRD_ETHER, "(ethernet hardware format)" );
	CASE_ARPHRD( ARPHRD_IEEE802, "(IEEE 802.2 Ethernet/TR/TB (token-ring hardware format))" );
	CASE_ARPHRD( ARPHRD_FRELAY, "(frame relay hardware format)" );
	CASE_ARPHRD( ARPHRD_IEEE1394, "(IEEE 1394 IPv4 - RFC 2734)" );
	CASE_ARPHRD( ARPHRD_IEEE1394_EUI64, "(IEEE1394 hardware address)" );
#endif
	};
	return "UNKNOWN";
}


#define CASE_ETH_P(num, h_proto, info) \
	case num: return #num ", " #h_proto ", " #info

const char * ethprotoname(uint16_t h_proto) // ETH_P_ !NB host byte order - use after ntohs(h_proto)
{
	static char s[sizeof("0xFFFF, UNKNOWN protocol")] = {0};
	switch( h_proto ) {
/*
 *	These are the defined Ethernet Protocol ID's.
 */
	CASE_ETH_P(0x0060, loop, "(Ethernet Loopback packet)" );
	CASE_ETH_P(0x0200, pup, "(Xerox PUP packet)" );
	CASE_ETH_P(0x0201, pupat, "(Xerox PUP Addr Trans packet)" );
	CASE_ETH_P(0x22F0, tsn, "(TSN (IEEE 1722) packet)" );
	CASE_ETH_P(0x22EB, erspan2, "(ERSPAN version 2 (type III))" );
	CASE_ETH_P(0x0800, ip, "(Internet Protocol packet)" );
	CASE_ETH_P(0x0805, x25, "(CCITT X.25)" );
	CASE_ETH_P(0x0806, arp, "(Address Resolution packet)" );
	CASE_ETH_P(0x08FF, bpq, "(G8BPQ AX.25 Ethernet Packet)" );
	CASE_ETH_P(0x0a00, ieeepup, "(Xerox IEEE802.3 PUP packet)" );
	CASE_ETH_P(0x0a01, ieeepupat, "(Xerox IEEE802.3 PUP Addr Trans packet)" );
	CASE_ETH_P(0x4305, batman, "(B.A.T.M.A.N.-Advanced packet)" );
	CASE_ETH_P(0x6000, dec, "(DEC Assigned proto)" );
	CASE_ETH_P(0x6001, dna_dl, "(DEC DNA Dump/Load)" );
	CASE_ETH_P(0x6002, dna_rc, "(DEC DNA Remote Console)" );
	CASE_ETH_P(0x6003, dna_rt, "(DEC DNA Routing)" );
	CASE_ETH_P(0x6004, lat, "(DEC LAT)" );
	CASE_ETH_P(0x6005, diag, "(DEC Diagnostics)" );
	CASE_ETH_P(0x6006, cust, "(DEC Customer use)" );
	CASE_ETH_P(0x6007, sca, "(DEC Systems Comms Arch)" );
	CASE_ETH_P(0x6558, teb, "(Trans Ether Bridging)" );
	CASE_ETH_P(0x8035, rarp, "(Reverse Addr Res packet)" );
	CASE_ETH_P(0x809B, atalk, "(Appletalk DDP)" );
	CASE_ETH_P(0x80F3, aarp, "(Appletalk AARP)" );
	CASE_ETH_P(0x8100, 8021q, "(802.1Q VLAN Extended Header)" );
	CASE_ETH_P(0x88BE, erspan, "(ERSPAN type II)" );
	CASE_ETH_P(0x8137, ipx, "(IPX over DIX)" );
	CASE_ETH_P(0x86DD, ipv6, "(IPv6 over bluebook)" );
	CASE_ETH_P(0x8808, pause, "(IEEE Pause frames. See 802.3 31B)" );
	CASE_ETH_P(0x8809, slow, "(Slow Protocol. See 802.3ad 43B)" );
	CASE_ETH_P(0x883E, wccp, "(Web-cache coordination protocol)" );
	CASE_ETH_P(0x8847, mpls_uc, "(MPLS Unicast traffic)" );
	CASE_ETH_P(0x8848, mpls_mc, "(MPLS Multicast traffic)" );
	CASE_ETH_P(0x884c, atmmpoa, "(MultiProtocol Over ATM)" );
	CASE_ETH_P(0x8863, ppp_disc, "(PPPoE discovery messages)" );
	CASE_ETH_P(0x8864, ppp_ses, "(PPPoE session messages)" );
	CASE_ETH_P(0x886c, link_ctl, "(HPNA, wlan link local tunnel)" );
	CASE_ETH_P(0x8884, atmfate, "(Frame-based ATM Transport over Ethernet)" );
	CASE_ETH_P(0x888E, pae, "(EAPOL PAE/802.1x Port Access Entity (IEEE 802.1X))" );
	CASE_ETH_P(0x8892, profinet, "(PROFINET)" );
	CASE_ETH_P(0x8899, realtek, "(Multiple proprietary protocols)" );
	CASE_ETH_P(0x88A2, aoe, "(ATA over Ethernet)" );
	CASE_ETH_P(0x88A4, ethercat, "(EtherCAT)" );
	CASE_ETH_P(0x88A8, 8021ad, "(802.1ad Service VLAN)" );
	CASE_ETH_P(0x88B5, 802_ex1, "(802.1 Local Experimental 1.)" );
	CASE_ETH_P(0x88C7, preauth, "(802.11 Preauthentication)" );
	CASE_ETH_P(0x88CA, tipc, "(TIPC)" );
	CASE_ETH_P(0x88CC, lldp, "(Link Layer Discovery Protocol)" );
	CASE_ETH_P(0x88E3, mrp, "(Media Redundancy Protocol)" );
	CASE_ETH_P(0x88E5, macsec, "(802.1ae MACsec)" );
	CASE_ETH_P(0x88E7, 8021ah, "(802.1ah Backbone Service Tag)" );
	CASE_ETH_P(0x88F5, mvrp, "(802.1Q MVRP)" );
	CASE_ETH_P(0x88F7, 1588, "(IEEE 1588 Timesync)" );
	CASE_ETH_P(0x88F8, ncsi, "(NCSI protocol)" );
	CASE_ETH_P(0x88FB, prp, "(IEC 62439-3 PRP/HSRv0)" );
	CASE_ETH_P(0x8902, cfm, "(Connectivity Fault Management)" );
	CASE_ETH_P(0x8906, fcoe, "(Fibre Channel over Ethernet)" );
	CASE_ETH_P(0x8915, iboe, "(Infiniband over Ethernet)" );
	CASE_ETH_P(0x890D, tdls, "(TDLS)" );
	CASE_ETH_P(0x8914, fip, "(FCoE Initialization Protocol)" );
	CASE_ETH_P(0x8917, 80221, "(IEEE 802.21 Media Independent Handover Protocol)" );
	CASE_ETH_P(0x892F, hsr, "(IEC 62439-3 HSRv1)" );
	CASE_ETH_P(0x894F, nsh, "(Network Service Header)" );
	CASE_ETH_P(0x9000, loopback, "(Ethernet loopback packet, per IEEE 802.3)" );
	CASE_ETH_P(0x9100, qinq1, "(deprecated QinQ VLAN)" );
	CASE_ETH_P(0x9200, qinq2, "(deprecated QinQ VLAN)" );
	CASE_ETH_P(0x9300, qinq3, "(deprecated QinQ VLAN)" );
	CASE_ETH_P(0xDADA, edsa, "(Ethertype DSA)" );
	CASE_ETH_P(0xDADB, dsa_8021q, "(Fake VLAN Header for DSA)" );
	CASE_ETH_P(0xE001, dsa_a5psw, "(A5PSW Tag Value)" );
	CASE_ETH_P(0xED3E, ife, "(ForCES inter-FE LFB type)" );
	CASE_ETH_P(0xFBFB, af_iucv, "(IBM af_iucv)" );
	CASE_ETH_P(0x0001, 802_3, "(Dummy type for 802.3 frames)" );
	CASE_ETH_P(0x0002, ax25, "(Dummy protocol id for AX.25)" );
	CASE_ETH_P(0x0003, all, "(Every packet)" );
	CASE_ETH_P(0x0004, 802_2, "(802.2 frames)" );
	CASE_ETH_P(0x0005, snap, "(Internal only)" );
	CASE_ETH_P(0x0006, ddcmp, "(DEC DDCMP: Internal only)" );
	CASE_ETH_P(0x0007, wan_ppp, "(Dummy type for WAN PPP frames)" );
	CASE_ETH_P(0x0008, ppp_mp, "(Dummy type for PPP MP frames)" );
	CASE_ETH_P(0x0009, localtalk, "(Localtalk pseudo type)" );
	CASE_ETH_P(0x000C, can, "(CAN: Controller Area Network)" );
	CASE_ETH_P(0x000D, canfd, "(CANFD: CAN flexible data rate)" );
	CASE_ETH_P(0x000E, canxl, "(CANXL: eXtended frame Length)" );
	CASE_ETH_P(0x0010, ppptalk, "(Dummy type for Atalk over PPP)" );
	CASE_ETH_P(0x0011, tr_802_2, "(802.2 frames)" );
	CASE_ETH_P(0x0015, mobitex, "(Mobitex (kaz@cafe.net))" );
	CASE_ETH_P(0x0016, control, "(Card specific control frames)" );
	CASE_ETH_P(0x0017, irda, "(Linux-IrDA)" );
	CASE_ETH_P(0x0018, econet, "(Acorn Econet)" );
	CASE_ETH_P(0x0019, hdlc, "(HDLC frames)" );
	CASE_ETH_P(0x001A, arcnet, "(1A for ArcNet)" );
	CASE_ETH_P(0x001B, dsa, "(Distributed Switch Arch.)" );
	CASE_ETH_P(0x001C, trailer, "(Trailer switch tagging)" );
	CASE_ETH_P(0x00F5, phonet, "(Nokia Phonet frames)" );
	CASE_ETH_P(0x00F6, ieee802154, "(IEEE802.15.4 frame)" );
	CASE_ETH_P(0x00F7, caif, "(ST-Ericsson CAIF protocol)" );
	CASE_ETH_P(0x00F8, xdsa, "(Multiplexed DSA protocol)" );
	CASE_ETH_P(0x00F9, map, "(Qualcomm multiplexing and aggregation protocol)" );
	CASE_ETH_P(0x00FA, mctp, "(Management component transport protocol packets)" );
	};
	if( snprintf(s, sizeof(s), "0x%04X, UNKNOWN protocol", h_proto) > 0 )
		return s;
	return "UNKNOWN";
}

#if defined(__APPLE__) || defined(__FreeBSD__)
#define CASE_IFT(ift, info) \
	case ift: return #ift ", " #info

const char * ifttoname(uint8_t ift) // IFT_
{
	switch( ift ) {
	CASE_IFT( IFT_OTHER       , "(none of the following)" );
	CASE_IFT( IFT_1822        , "(old-style arpanet imp)" );
	CASE_IFT( IFT_HDH1822     , "(HDH arpanet imp)" );
	CASE_IFT( IFT_X25DDN      , "(x25 to imp)" );
	CASE_IFT( IFT_X25         , "(PDN X25 interface (RFC877))" );
	CASE_IFT( IFT_ETHER       , "(Ethernet CSMACD)" );
	CASE_IFT( IFT_ISO88023    , "(CMSA CD)" );
	CASE_IFT( IFT_ISO88024    , "(Token Bus)" );
	CASE_IFT( IFT_ISO88025    , "(Token Ring)" );
	CASE_IFT( IFT_ISO88026    , "(MAN)" );
	CASE_IFT( IFT_STARLAN     , "" );
	CASE_IFT( IFT_P10         , "(Proteon 10MBit ring)" );
	CASE_IFT( IFT_P80         , "(Proteon 80MBit ring)" );
	CASE_IFT( IFT_HY          , "(Hyperchannel)" );
	CASE_IFT( IFT_FDDI        , "" );
	CASE_IFT( IFT_LAPB        , "" );
	CASE_IFT( IFT_SDLC        , "" );
	CASE_IFT( IFT_T1          , "" );
	CASE_IFT( IFT_CEPT        , "(E1 - european T1)" );
	CASE_IFT( IFT_ISDNBASIC   , "" );
	CASE_IFT( IFT_ISDNPRIMARY , "" );
	CASE_IFT( IFT_PTPSERIAL   , "(Proprietary PTP serial)" );
	CASE_IFT( IFT_PPP         , "(RFC 1331)" );
	CASE_IFT( IFT_LOOP        , "(loopback)" );
	CASE_IFT( IFT_EON         , "(ISO over IP)" );
	CASE_IFT( IFT_XETHER      , "(obsolete 3MB experimental ethernet)" );
	CASE_IFT( IFT_NSIP        , "(XNS over IP)" );
	CASE_IFT( IFT_SLIP        , "(IP over generic TTY)" );
	CASE_IFT( IFT_ULTRA       , "(Ultra Technologies)" );
	CASE_IFT( IFT_DS3         , "(Generic T3)" );
	CASE_IFT( IFT_SIP         , "(SMDS)" );
	CASE_IFT( IFT_FRELAY      , "(Frame Relay DTE only)" );
	CASE_IFT( IFT_RS232       , "" );
	CASE_IFT( IFT_PARA        , "(parallel-port)" );
	CASE_IFT( IFT_ARCNET      , "" );
	CASE_IFT( IFT_ARCNETPLUS  , "" );
	CASE_IFT( IFT_ATM         , "(ATM cells)" );
	CASE_IFT( IFT_MIOX25      , "" );
	CASE_IFT( IFT_SONET       , "(SONET or SDH)" );
	CASE_IFT( IFT_X25PLE      , "" );
	CASE_IFT( IFT_ISO88022LLC , "" );
	CASE_IFT( IFT_LOCALTALK   , "" );
	CASE_IFT( IFT_SMDSDXI     , "" );
	CASE_IFT( IFT_FRELAYDCE   , "(Frame Relay DCE)" );
	CASE_IFT( IFT_V35         , "" );
	CASE_IFT( IFT_HSSI        , "" );
	CASE_IFT( IFT_HIPPI       , "" );
	CASE_IFT( IFT_MODEM       , "(Generic Modem)" );
	CASE_IFT( IFT_AAL5        , "(AAL5 over ATM)" );
	CASE_IFT( IFT_SONETPATH   , "" );
	CASE_IFT( IFT_SONETVT     , "" );
	CASE_IFT( IFT_SMDSICIP    , "(SMDS InterCarrier Interface)" );
	CASE_IFT( IFT_PROPVIRTUAL , "(Proprietary Virtual/internal)" );
	CASE_IFT( IFT_PROPMUX     , "(Proprietary Multiplexing)" );
/*
 * IFT_GIF, IFT_FAITH and IFT_6LOWPAN are not based on IANA assignments.
 * Note: IFT_STF has a defined ifType: 0xd7 (215), but we use 0x39.
*/
	CASE_IFT( IFT_GIF         , "" );            /*0xf0*/
	CASE_IFT( IFT_FAITH       , "" );            /*0xf2*/
	CASE_IFT( IFT_STF         , "" );            /*0xf3*/
	CASE_IFT( IFT_6LOWPAN     , "(IETF RFC 6282)" );
	CASE_IFT( IFT_L2VLAN      , "(Layer 2 Virtual LAN using 802.1Q)" );
	CASE_IFT( IFT_IEEE8023ADLAG , "(IEEE802.3ad Link Aggregate)" );
	CASE_IFT( IFT_IEEE1394    , "(IEEE1394 High Performance SerialBus)" );
	CASE_IFT( IFT_BRIDGE      , "(Transparent bridge interface)" );
	CASE_IFT( IFT_ENC         , "(Encapsulation)" );
	CASE_IFT( IFT_PFLOG       , "(Packet filter logging)" );
	CASE_IFT( IFT_PFSYNC      , "(Packet filter state syncing)" );
	CASE_IFT( IFT_CARP        , "(Common Address Redundancy Protocol)" );
	CASE_IFT( IFT_PKTAP       , "(Packet tap pseudo interface)" );
	CASE_IFT( IFT_CELLULAR    , "(Packet Data over Cellular)" );
	};
	return "UNKNOWN";
}

#define CASE_RTM(rtm, info) \
	case rtm: return #rtm ", " #info

const char * rtmtoname(uint8_t ift) // RTM_
{
	switch( ift ) {
	CASE_RTM(RTM_ADD         , "(Add Route)" );
	CASE_RTM(RTM_DELETE      , "(Delete Route)" );
	CASE_RTM(RTM_CHANGE      , "(Change Metrics or flags)" );
	CASE_RTM(RTM_GET         , "(Report Metrics)" );
	CASE_RTM(RTM_LOSING      , "(RTM_LOSING is no longer generated by xnu)" );
	CASE_RTM(RTM_REDIRECT    , "(Told to use different route)" );
	CASE_RTM(RTM_MISS        , "(Lookup failed on this address)" );
	CASE_RTM(RTM_LOCK        , "(fix specified metrics)" );
	CASE_RTM(RTM_OLDADD      , "(caused by SIOCADDRT)" );
	CASE_RTM(RTM_OLDDEL      , "(caused by SIOCDELRT)" );
	CASE_RTM(RTM_RESOLVE     , "(req to resolve dst to LL addr)" );
	CASE_RTM(RTM_NEWADDR     , "(address being added to iface)" );
	CASE_RTM(RTM_DELADDR     , "(address being removed from iface)" );
	CASE_RTM(RTM_IFINFO      , "(iface going up/down etc.)" );
	CASE_RTM(RTM_NEWMADDR    , "(mcast group membership being added to if)" );
	CASE_RTM(RTM_DELMADDR    , "(mcast group membership being deleted)" );
	CASE_RTM(RTM_IFINFO2     , "" );
	CASE_RTM(RTM_NEWMADDR2   , "" );
	CASE_RTM(RTM_GET2        , "" );
	};
	return "UNKNOWN";
}

#endif

uint32_t addflag(char * buf, const char * name, uint32_t flags, uint32_t flag)
{
	if( flags & flag ) {
		strcat(buf, name);
		if( (flags &= ~flag) )
			strcat(buf, ", ");
	}
	return flags;
}

uint32_t addflag_space_separator(char * buf, const char * name, uint32_t flags, uint32_t flag)
{
	if( flags & flag ) {
		strcat(buf, name);
		if( (flags &= ~flag) )
			strcat(buf, " ");
	}
	return flags;
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
int has_accept_ra_rt_table(void)
{
	static int init = 0;
	static int has = 0;
	if( !init ) {
		has = (access("/proc/sys/net/ipv6/conf/default/accept_ra_rt_table", F_OK) == 0);
		init = 1;
	}
	return has;
}

const char * vlanflags(uint32_t vflags)
{
	//https://ru.wikipedia.org/wiki/IEEE_802.1Q
	static char buf[sizeof("REORDER_HDR, GVRP, LOOSE_BINDING, MVRP, BRIDGE_BINDING")] = {0};
	buf[0] = '\0';
	vflags = addflag(buf, "REORDER_HDR", vflags, VLAN_FLAG_REORDER_HDR);
	vflags = addflag(buf, "GVRP", vflags, VLAN_FLAG_GVRP);
	vflags = addflag(buf, "LOOSE_BINDING", vflags, VLAN_FLAG_LOOSE_BINDING);
	vflags = addflag(buf, "MVRP", vflags, VLAN_FLAG_MVRP); // https://ru.wikipedia.org/wiki/Multiple_Registration_Protocol
	vflags = addflag(buf, "BRIDGE_BINDING", vflags, VLAN_FLAG_BRIDGE_BINDING);
	return buf;
}

const char * inet6flags(uint32_t iflags)
{
	static char buf[sizeof("RA_OTHERCONF, RA_MANAGED, RA_RCVD, RS_SENT, READY")] = {0};
	buf[0] = '\0';
	iflags = addflag(buf, "RA_OTHERCONF", iflags, IF_RA_OTHERCONF);
	iflags = addflag(buf, "RA_MANAGED", iflags, IF_RA_MANAGED);
	iflags = addflag(buf, "RA_RCVD", iflags, IF_RA_RCVD);
	iflags = addflag(buf, "RS_SENT", iflags, IF_RS_SENT);
	iflags = addflag(buf, "READY", iflags, IF_READY);
	return buf;
}

const char * ifaddrflags(uint32_t iflags)
{
	static char buf[sizeof("SECONDARY, NODAD, OPTIMISTIC, DADFAILED, HOMEADDRESS, DEPRECATED, TENTATIVE, PERMANENT, MANAGETEMPADDR, NOPREFIXROUTE, MCAUTOJOIN, STABLE_PRIVACY")] = {0};
	buf[0] = '\0';
	iflags = addflag(buf, "SECONDARY", iflags, IFA_F_SECONDARY);
	iflags = addflag(buf, "NODAD", iflags, IFA_F_NODAD);
	iflags = addflag(buf, "OPTIMISTIC", iflags, IFA_F_OPTIMISTIC);
	iflags = addflag(buf, "DADFAILED", iflags, IFA_F_DADFAILED);
	iflags = addflag(buf, "HOMEADDRESS", iflags, IFA_F_HOMEADDRESS);
	iflags = addflag(buf, "DEPRECATED", iflags, IFA_F_DEPRECATED);
	iflags = addflag(buf, "TENTATIVE", iflags, IFA_F_TENTATIVE);
	iflags = addflag(buf, "PERMANENT", iflags, IFA_F_PERMANENT);
	iflags = addflag(buf, "MANAGETEMPADDR", iflags, IFA_F_MANAGETEMPADDR);
	iflags = addflag(buf, "NOPREFIXROUTE", iflags, IFA_F_NOPREFIXROUTE);
	iflags = addflag(buf, "MCAUTOJOIN", iflags, IFA_F_MCAUTOJOIN);
	iflags = addflag(buf, "STABLE_PRIVACY", iflags, IFA_F_STABLE_PRIVACY);
	return buf;
}

const char * ipv6genmodeflags(uint8_t iflags) // IN6_ADDR_GEN_MODE_
{
	static char buf[sizeof("EUI64, NONE, STABLE_PRIVACY, RANDOM")] = {0};
	buf[0] = '\0';
	iflags = addflag(buf, "EUI64", iflags, IN6_ADDR_GEN_MODE_EUI64);
	iflags = addflag(buf, "NONE", iflags, IN6_ADDR_GEN_MODE_NONE);
	iflags = addflag(buf, "STABLE_PRIVACY", iflags, IN6_ADDR_GEN_MODE_STABLE_PRIVACY);
	iflags = addflag(buf, "RANDOM", iflags, IN6_ADDR_GEN_MODE_RANDOM);
	return buf;
}

const char * ifoperstate(uint8_t operstate)
{
	static const char * operstatename[] = {
		"IF_OPER_UNKNOWN",
		"IF_OPER_NOTPRESENT",
		"IF_OPER_DOWN",
		"IF_OPER_LOWERLAYERDOWN",
		"IF_OPER_TESTING",
		"IF_OPER_DORMANT",
		"IF_OPER_UP"
	};

	if( operstate < sizeof(operstatename)/sizeof(operstatename[0]) )
		return operstatename[operstate];
	return "UNKNOWN";
}

const char * iflinkmode(uint8_t link_mode)
{
	static const char * link_modename[] = {
		"IF_LINK_MODE_DEFAULT",
		"IF_LINK_MODE_DORMANT",
		"IF_LINK_MODE_TESTING"
	};
	if( link_mode < sizeof(link_modename)/sizeof(link_modename[0]) )
		return link_modename[link_mode];
	return "UNKNOWN";
}

#endif

uint8_t Ip6MaskToBits(const char * mask)
{
	int masklen = strlen(mask), bits = 0, i;
	if( masklen < sizeof(char) )
		return 0;

	if( masklen < sizeof("/128") && *mask != ':' ) {
		bits = (*mask == 0x2F) ? atoi(mask+1):atoi(mask);
		return bits <= 128 ? bits:0;
	}

	struct in6_addr addr;
	inet_pton(AF_INET6, mask, &addr);
	for( i = 0; i < 16; i++ ) {
		while (addr.s6_addr[i]) {
			bits += addr.s6_addr[i] & 1;
			addr.s6_addr[i] >>= 1;
		}
	}
	return (uint8_t)bits;
}

uint8_t IpMaskToBits(const char * mask)
{
	int masklen = strlen(mask), bits = 0;
	if( masklen < sizeof(char) )
		return 0;

	if( masklen < sizeof("/32") ) {
		bits = (*mask == 0x2F) ? atoi(mask+1):atoi(mask);
		return bits <= 32 ? bits:0;
	}

	struct in_addr addr;
	inet_pton(AF_INET, mask, &addr);
	while( addr.s_addr ) {
		bits += addr.s_addr & 1;
		addr.s_addr >>= 1;
	}
	return (uint8_t)bits;
}


const char * IpBitsToMask(uint32_t bits, char * buffer, size_t maxlen)
{
    int mask = 0xffffffff << (32 - bits);
    if( bits < 32 && snprintf(buffer, maxlen, "%d.%d.%d.%d", (mask >> 24) & 0xff, (mask >> 16) & 0xff, (mask >> 8) & 0xff, mask & 0xff) > 0 )
	return buffer;
    return "0.0.0.0";
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)

#define RTF_NOTCACHED   0x0400
#define RTF_EXPIRES     0x00400000

const char * RouteFlagsToString(uint32_t iflags, int ipv6)
{
	static char flags[64] = {0};
	/* Decode the flags. */
	flags[0] = '\0';

	if( !ipv6 ) {
	if (iflags & RTF_UP)
	    strcat(flags, "U");
	if (iflags & RTF_GATEWAY)
	    strcat(flags, "G");
#if defined RTF_REJECT
	if (iflags & RTF_REJECT)
	    strncpy(flags, "!", sizeof(flags));
#endif
	if (iflags & RTF_HOST)
	    strcat(flags, "H");
	if (iflags & RTF_REINSTATE)
	    strcat(flags, "R");
	if (iflags & RTF_DYNAMIC)
	    strcat(flags, "D");
	if (iflags & RTF_MODIFIED)
	    strcat(flags, "M");
	if (iflags & RTF_DEFAULT)
	    strcat(flags, "d");
	if (iflags & RTF_ALLONLINK)
	    strcat(flags, "a");
	if (iflags & RTF_ADDRCONF)
	    strcat(flags, "c");
	if (iflags & RTF_NONEXTHOP)
	    strcat(flags, "o");
	if (iflags & RTF_EXPIRES)
	    strcat(flags, "e");
	if (iflags & RTF_CACHE)
	    strcat(flags, "c");
	if (iflags & RTF_FLOW)
	    strcat(flags, "f");
	if (iflags & RTF_POLICY)
	    strcat(flags, "p");
	if (iflags & RTF_LOCAL)
	    strcat(flags, "l");
	if (iflags & RTF_MTU)
	    strcat(flags, "u");
	if (iflags & RTF_WINDOW)
	    strcat(flags, "w");
	if (iflags & RTF_IRTT)
	    strcat(flags, "i");
	if (iflags & RTF_NOTCACHED)
	    strcat(flags, "n");
	} else {
	if (iflags & RTF_UP)
	    strcat(flags, "U");
	if (iflags & RTF_REJECT)
	    strcat(flags, "!");
	if (iflags & RTF_GATEWAY)
	    strcat(flags, "G");
	if (iflags & RTF_HOST)
	    strcat(flags, "H");
	if (iflags & RTF_DEFAULT)
	    strcat(flags, "D");
	if (iflags & RTF_ADDRCONF)
	    strcat(flags, "A");
	if (iflags & RTF_CACHE)
	    strcat(flags, "C");
	if (iflags & RTF_ALLONLINK)
	    strcat(flags, "a");
	if (iflags & RTF_EXPIRES)
	    strcat(flags, "e");
	if (iflags & RTF_MODIFIED)
	    strcat(flags, "m");
	if (iflags & RTF_NONEXTHOP)
	    strcat(flags, "n");
	if (iflags & RTF_FLOW)
	    strcat(flags, "f");
	}
	return flags;
}

#else

#ifndef RTF_GLOBAL
#define RTF_GLOBAL      0x40000000      /* route to destination of the global internet */
#endif

const char * RouteFlagsToString(uint32_t iflags, int ipv6)
{
	static const struct bits {
		uint32_t	b_mask;
		char	b_val;
	} bits[] = {
		{ RTF_UP,	'U' },
		{ RTF_GATEWAY,	'G' },
		{ RTF_HOST,	'H' },
		{ RTF_REJECT,	'R' },
		{ RTF_DYNAMIC,	'D' },
		{ RTF_MODIFIED,	'M' },
		{ RTF_MULTICAST,'m' },
		{ RTF_DONE,	'd' }, /* Completed -- for routing messages only */
		{ RTF_CLONING,	'C' },
		{ RTF_XRESOLVE,	'X' },
		{ RTF_LLINFO,	'L' },
		{ RTF_STATIC,	'S' },
		{ RTF_PROTO1,	'1' },
		{ RTF_PROTO2,	'2' },
		{ RTF_WASCLONED,'W' },
		{ RTF_PRCLONING,'c' },
		{ RTF_PROTO3,	'3' },
		{ RTF_BLACKHOLE,'B' },
		{ RTF_BROADCAST,'b' },
		{ RTF_IFSCOPE,	'I' },
		{ RTF_IFREF,	'i' },
		{ RTF_PROXY,	'Y' },
		{ RTF_ROUTER,	'r' },
		{ RTF_GLOBAL,	'g' },
		{ 0 }
	};
	static char name[33];
	char *flags;
	const struct bits *p = bits;
	for( flags = name; p->b_mask; p++)
		if (p->b_mask & iflags)
			*flags++ = p->b_val;
	*flags = '\0';
	return name;
}

#endif

#ifdef MAIN_COMMON_NETUTILS

#include "log.h"

#define LOG_SOURCE_FILE "netutils.c"
#define LOG_FILE ""

int main(int argc, char * argv[])
{
	char buf[sizeof("255.255.255.255")] = {0};
	LOG_INFO("familyname(AF_INET6) %s\n", familyname(AF_INET6));
	LOG_INFO("familyname(255) %s\n", familyname((char)255));
	LOG_INFO("ifflagsname(-1) %s\n", ifflagsname((uint32_t)-1));
	LOG_INFO("ethprotoname(0x0800) %s\n", ethprotoname(0x0800));
	LOG_INFO("ethprotoname(0x86dd) %s\n", ethprotoname(0x86dd));
	LOG_INFO("ethprotoname(0xdddd) %s\n", ethprotoname(0xdddd));
	LOG_INFO("Ip6MaskToBits(\"ffff:ffff:ffff:ffff::\") = %u\n", Ip6MaskToBits("ffff:ffff:ffff:ffff::"));
	LOG_INFO("Ip6MaskToBits(\"ffff:ffff:ffff:ffff:ffff::\") = %u\n", Ip6MaskToBits("ffff:ffff:ffff:ffff:ffff::"));
	LOG_INFO("Ip6MaskToBits(\"ffff:ffff:ffff:ffff:ffff:ffff::\") = %u\n", Ip6MaskToBits("ffff:ffff:ffff:ffff:ffff:ffff::"));
	LOG_INFO("Ip6MaskToBits(\"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff\") = %u\n", Ip6MaskToBits("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
	LOG_INFO("IpBitsToMask(16) = %s\n", IpBitsToMask(16, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(24) = %s\n", IpBitsToMask(24, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(32) = %s\n", IpBitsToMask(32, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(1) = %s\n", IpBitsToMask(1, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(9) = %s\n", IpBitsToMask(9, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(33333) = %s\n", IpBitsToMask(33333, buf, sizeof(buf)));
	LOG_INFO("IpBitsToMask(\"255.255.255.0\") = %u\n", IpMaskToBits("255.255.255.0"));
	LOG_INFO("IpBitsToMask(\"255.255.255.255\") = %u\n", IpMaskToBits("255.255.255.255"));
	LOG_INFO("IpBitsToMask(\"255.255.255.128\") = %u\n", IpMaskToBits("255.255.255.128"));
	LOG_INFO("IpBitsToMask(\"24\") = %u\n", IpMaskToBits("24"));
	LOG_INFO("IpBitsToMask(\"/24\") = %u\n", IpMaskToBits("/24"));
	LOG_INFO("IpBitsToMask(\"abc\") = %u\n", IpMaskToBits("abc"));
	LOG_INFO("IpBitsToMask(\"124\") = %u\n", IpMaskToBits("124"));
	LOG_INFO("IpBitsToMask(\"64\") = %u\n", IpMaskToBits("64"));
	LOG_INFO("IpBitsToMask(\"/64\") = %u\n", IpMaskToBits("/64"));
	LOG_INFO("IpBitsToMask(\"/64\") = %u\n", IpMaskToBits("/64"));
	LOG_INFO("RouteFlagsToString(0xFFFFFFFF) = \"%s\"\n", RouteFlagsToString(0xFFFFFFFF, 0));
	LOG_INFO("RouteFlagsToString(0xFFFFFFFF) = \"%s\"\n", RouteFlagsToString(0xFFFFFFFF, 1));
	return 0;
}

#endif //MAIN_COMMON_NETUTILS
