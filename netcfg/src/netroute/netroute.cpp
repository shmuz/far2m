#include "netroute.h"
#include "netrouteset.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/netutils.h>
#include <common/sizestr.h>

#include <memory>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
}

#define LOG_SOURCE_FILE "netroute.cpp"
extern const char * LOG_FILE;

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%lld"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%ld"
#else
    #error "Environment not 32 or 64-bit."
#endif

IpRouteInfo::IpRouteInfo()
{
	hoplimit = (uint32_t)-1;
	memset(&valid, 0, sizeof(valid));
	flags = 0;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	osdep.metric = 0;
	osdep.table = 0;
	osdep.tos = 0;
	osdep.protocol = 0;
	osdep.scope = 0;
	osdep.type = 0;
	osdep.icmp6pref = 0;
	memset(&osdep.enc, 0, sizeof(osdep.enc));
	memset(&osdep.rtcache, 0, sizeof(osdep.rtcache));
	memset(&osdep.rtmetrics, 0, sizeof(osdep.rtmetrics));
#else
	osdep.expire = 0;
	osdep.gateway_type = IpRouteInfo::IpGatewayType;
#endif
}
IpRouteInfo::~IpRouteInfo()
{
//	LOG_INFO("\n");
}
ArpRouteInfo::ArpRouteInfo()
{
	flags = 0;
	sa_family = AF_INET;
	#if !defined(__APPLE__) && !defined(__FreeBSD__)
	hwType = 0;
	type = 0;
	state = 0;
	ci = {0};
	valid = {0};
	flags_ext = 0;
	#endif
}
ArpRouteInfo::~ArpRouteInfo()
{
//	LOG_INFO("\n");
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
const char * IpRouteInfo::GetEncap(const Encap & enc) const
{
	static char buf[256];
	char * ptr = buf;
	size_t size = sizeof(buf), res = 0;
	buf[0] = 0;

	switch( enc.type ) {
	case LWTUNNEL_ENCAP_MPLS:
		if( (res = snprintf(ptr, size, "mpls")) < 0 || size < res )
			break;
		ptr += res;
		size -= res;
		res = 0;
		if( enc.data.mpls.valid.dst )
			if( (res = snprintf(ptr, size, " %s", enc.data.mpls.dst)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		if( enc.data.mpls.valid.ttl )
			snprintf(ptr, size, " ttl %u", enc.data.mpls.ttl);
		break;
	case LWTUNNEL_ENCAP_IP:
	case LWTUNNEL_ENCAP_IP6:
		if( (res = snprintf(ptr, size, "%s", enc.type == LWTUNNEL_ENCAP_IP ? "ip":"ip6")) < 0 || size < res )
			break;
		ptr += res;
		size -= res;
		res = 0;
		if( enc.data.ip.valid.id )
			if( (res = snprintf(ptr, size, " id " LLFMT, enc.data.ip.id)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.ttl )
			if( (res = snprintf(ptr, size, " ttl %u", enc.data.ip.ttl)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.hoplimit )
			if( (res = snprintf(ptr, size, " hoplimit %u", enc.data.ip.hoplimit)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.tos )
			if( (res = snprintf(ptr, size, " tos %u", enc.data.ip.tos)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.tc )
			if( (res = snprintf(ptr, size, " tc %u", enc.data.ip.tc)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.src )
			if( (res = snprintf(ptr, size, " src %s", enc.data.ip.src)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.dst )
			if( (res = snprintf(ptr, size, " dst %s", enc.data.ip.dst)) < 0 || size < res )
				break;
		ptr += res;
		size -= res;
		res = 0;

		if( enc.data.ip.valid.flags ) {
			//if( (res = snprintf(ptr, size, " flags 0x%04x (%s)\n", enc.data.ip.flags, tunnelflagsname(enc.data.ip.flags))) < 0 || size < res )
			//	break;

			if( (res = snprintf(ptr, size, " %s", tunnelflagsname(enc.data.ip.flags))) < 0 || size < res )
				break;

			ptr += res;
			size -= res;
			res = 0;

			if( enc.data.ip.flags & TUNNEL_GENEVE_OPT && enc.data.ip.valid.geneve ) {
				if( (res = snprintf(ptr, size, " %s", enc.data.ip.geneve_opts ) < 0 || size < res ) )
					break;
			} else if( enc.data.ip.flags & TUNNEL_VXLAN_OPT && enc.data.ip.valid.vxlan ) {
				if( enc.data.ip.valid.vxlan_gbp )
					if( (res = snprintf(ptr, size, " %u", enc.data.ip.vxlan_gbp )) < 0 || size < res )
						break;
			} else if( enc.data.ip.flags & TUNNEL_ERSPAN_OPT && enc.data.ip.valid.erspan ) {
				if( (res = snprintf(ptr, size, " %s", enc.data.ip.erspan_opts ) < 0 || size < res ) )
					break;
			}
		}
		ptr += res;
		size -= res;
		res = 0;

		break;
	}

	return buf;
}

const char * IpRouteInfo::GetNextHopes(void) const
{
	static char buf[1024];
	char * ptr = buf;
	size_t size = sizeof(buf), res = 0;
	buf[0] = 0;

	if( !valid.rtnexthop )
		return buf;

	for( const auto & item : osdep.nhs ) {

		if( !item.valid.nexthope )
			continue;

		if(  (res = snprintf(ptr, size, " nexthop")) < 0 || size < res  )
			break;

		ptr += res;
		size -= res;
		res = 0;
		

		if( item.valid.encap && ((res = snprintf(ptr, size, " encap %s", GetEncap(item.enc))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( item.valid.rtvia ) {
			if( (res = snprintf(ptr, size, " via %s %S", ipfamilyname(item.rtvia_family), item.rtvia_addr.c_str() )) < 0 || size < res )
				break;

		ptr += res;
		size -= res;
		res = 0;

		} else if( item.valid.gateway )
			if( (res = snprintf(ptr, size, " via %S", item.gateway.c_str() )) < 0 || size < res )
				break;

		ptr += res;
		size -= res;
		res = 0;

		if( item.valid.iface )
			if( (res = snprintf(ptr, size, " dev %S", item.iface.c_str() )) < 0 || size < res )
				break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && valid.type && flags & RTM_F_CLONED && osdep.type == RTN_MULTICAST )
			res = snprintf(ptr, size, " ttl %u", item.weight);
		else
			res = snprintf(ptr, size, " weight %u", item.weight+1);

		if( res < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( (res = snprintf(ptr, size, "%s%s", item.flags & RTNH_F_ONLINK ? " onlink":"", item.flags & RTNH_F_PERVASIVE ? " pervasive":"")) < 0 || size < res )
			break;
		ptr += res;
		size -= res;
		res = 0;
	}

	return buf;
}

#endif

void IpRouteInfo::Log(void) const
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	const char * fmt = sa_family == AF_INET ? \
		"from: %15S to: %18S via: %15S dev: %17S prefsrc: %15S table: %5s type: %11s proto: %6s scope: %7s metric: %4d perf: %6s tos: %3u flags: 0x%08X(%s) encap: %s %s\n":
		"from: %15S to: %29S via: %15S dev: %17S prefsrc: %29S table: %5s type: %11s proto: %6s scope: %7s metric: %4d perf: %6s tos: %3u flags: 0x%08X(%s) encap: %s %s\n";
	LOG_INFO(fmt,
		osdep.fromsrcIpandMask.empty() ? L"any":osdep.fromsrcIpandMask.c_str(),
		!destIpandMask.empty() ? destIpandMask.c_str():L"default",
		gateway.empty() ? L"default":gateway.c_str(),
		iface.c_str(),
		prefsrc.empty() ?	L"any":prefsrc.c_str(),
		rtruletable(osdep.table),
		rttype(osdep.type),
		rtprotocoltype(osdep.protocol),
		rtscopetype(osdep.scope),
		osdep.metric,
		rticmp6pref(osdep.icmp6pref),
		osdep.tos,
		flags, RouteFlagsToString(flags, sa_family == AF_INET6), valid.encap ? GetEncap(osdep.enc):"none", GetNextHopes());

	//LogRtCache();
#else
	const char * fmt = sa_family == AF_INET ? \
		"to: %18S via: %15S dev: %17S prefsrc: %15S expire: %4d flags: 0x%08X(%s) family %d(%s)\n":
		"to: %29S via: %15S dev: %17S prefsrc: %29S expire: %4d flags: 0x%08X(%s) family %d(%s)\n";
	LOG_INFO(fmt,
		!destIpandMask.empty() ? destIpandMask.c_str():L"default",
		gateway.empty() ? L"default":gateway.c_str(),
		iface.c_str(),
		prefsrc.empty() ?	L"any":prefsrc.c_str(),
		osdep.expire,
		flags, RouteFlagsToString(flags, sa_family == AF_INET6), sa_family, familyname(sa_family));

#endif
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
void RuleRouteInfo::Log(void) const
{
	LOG_INFO("%s) %5d: %S\n", familyname(family), priority, rule.c_str());
}

// towstr can use only for ASCII symbols
static std::wstring towstr(const char * name)
{
	std::string _s(name);
	return std::wstring(_s.begin(), _s.end());
}

RuleRouteInfo::RuleRouteInfo()
{
	type = 0;
	table = 0;
	flags = 0;
	protocol = 0;
	ip_protocol = 0;
	valid = {0};
}

RuleRouteInfo::~RuleRouteInfo()
{

}

void RuleRouteInfo::ToRuleString(bool skipExtInfo)
{
	const size_t maxlen = sizeof("18446744073709551615");
	char s[maxlen] = {0};

	if( !rule.empty() )
		return;

	if( !skipExtInfo && type == RTM_DELRULE )
		rule += L"Deleted ";

	if( flags & FIB_RULE_INVERT )
		rule += L"not ";

	if( valid.fromIpandMask )
		rule += L"from " + fromIpandMask;

	if( valid.toIpandMask )
		rule += L" to " + toIpandMask;

	if( tos ) {
		if( snprintf(s, maxlen, "0x%02x", tos) > 0 )
			rule += L" tos " + towstr(s);
	}
	if( valid.fwmark || valid.fwmask ) {
		rule += L" fwmark ";
		if( snprintf(s, maxlen, "0x%x", fwmark) > 0 )
			rule += towstr(s);
		if( valid.fwmask ) {
			if( snprintf(s, maxlen, "0x%x", fwmask) > 0 )
				rule += L"/" + towstr(s);
		}
	}
	if( valid.iiface ) {
		rule += L" iif ";
		rule += iiface;
		if( !skipExtInfo && flags & FIB_RULE_IIF_DETACHED )
			rule += L" [detached]";
	}
	if( valid.oiface ) {
		rule += L" oif " + oiface;
		if( !skipExtInfo && flags & FIB_RULE_OIF_DETACHED )
			rule += L" [detached]";
	}

	if( skipExtInfo && valid.priority )
		rule += L" pref " + std::to_wstring(priority);

	if( valid.l3mdev )
		rule += L" l3mdev";

	if( valid.uid_range )
		rule += L" uidrange "+ std::to_wstring(uid_range.start) + L"-" + std::to_wstring(uid_range.end);

	if( valid.ip_protocol )
		rule += L" ipproto "+ towstr(iprotocolname(ip_protocol));

	if( valid.sport_range ) {
		if( sport_range.start == sport_range.end )
			rule += L" sport "+ std::to_wstring(sport_range.start);
		else
			rule += L" sport "+ std::to_wstring(sport_range.start) + L"-" + std::to_wstring(sport_range.end);
	}
	if( valid.dport_range ) {
		if( dport_range.start == dport_range.end )
			rule += L" dport "+ std::to_wstring(dport_range.start);
		else
			rule += L" dport "+ std::to_wstring(dport_range.start) + L"-" + std::to_wstring(dport_range.end);
	}

	if( valid.tun_id )
		rule += L" tun_id " + std::to_wstring(tun_id);

	if( table ) {
		rule += L" table " + towstr(rtruletable(table));
		if( valid.suppress_prefixlength )
			rule += L" suppress_prefixlength " + std::to_wstring(suppress_prefixlength);
		if( valid.suppress_ifgroup )
			rule += L" suppress_ifgroup " + std::to_wstring(suppress_ifgroup);
	}

	if( valid.flowto ) {
		rule += L" realms ";
		if( valid.flowfrom )
			rule += std::to_wstring(flowfrom) + L"/";
		rule += std::to_wstring(flowto);
	}

	switch( action ) {
		case FR_ACT_TO_TBL:
			break;
		case FR_ACT_GOTO:
			if( valid.goto_priority )
				rule += L" goto " + std::to_wstring(goto_priority);
			else
				rule += L" goto none";
			if( !skipExtInfo && flags & FIB_RULE_UNRESOLVED )
				rule += L" [unresolved]";
			break;
		case RTN_NAT:
			if( !skipExtInfo ) {
				if( valid.gateway )
					rule += L" map-to "+ gateway;
				else
					rule += L" masquerade";
			}
			break;
		default:
			rule += L" " + towstr(fractionrule(action));
			break;
	};

	if( valid.protocol && protocol )
		rule += L" proto " + towstr(rtprotocoltype(protocol));
}

void IpRouteInfo::LogRtCache(void) const
{
	static int hz = 0;

	if( !hz )
		hz = sysconf(_SC_CLK_TCK);

	if( !hz )
		hz = 100;

	if( osdep.rtcache.rta_clntref ||
	    osdep.rtcache.rta_lastuse ||
	    osdep.rtcache.rta_expires ||
	    osdep.rtcache.rta_error ||
	    osdep.rtcache.rta_used ||
	    osdep.rtcache.rta_id ||
	    osdep.rtcache.rta_ts ||
	    osdep.rtcache.rta_tsage ) {
	LOG_INFO("    rta_clntref: %u\n", osdep.rtcache.rta_clntref);
	LOG_INFO("    rta_lastuse: %u ( %s)\n", osdep.rtcache.rta_lastuse, msec_to_str(SEC_TO_MS(osdep.rtcache.rta_lastuse/hz)));
	LOG_INFO("    rta_expires: %d ( %s)\n", osdep.rtcache.rta_expires, msec_to_str(SEC_TO_MS(osdep.rtcache.rta_expires/hz)));
	LOG_INFO("    rta_error:   %d (%s)\n", osdep.rtcache.rta_error, errorname(osdep.rtcache.rta_error < 0 ? (-osdep.rtcache.rta_error):osdep.rtcache.rta_error));
	LOG_INFO("    rta_used:    %u\n", osdep.rtcache.rta_used);
	LOG_INFO("    rta_id:      0x%08X\n", osdep.rtcache.rta_id);
	LOG_INFO("    rta_ts:      %u\n", osdep.rtcache.rta_ts);
	LOG_INFO("    rta_tsage:   %u ( %s)\n", osdep.rtcache.rta_tsage, msec_to_str(SEC_TO_MS(osdep.rtcache.rta_tsage)));
	}
}

bool IpRouteInfo::CreateIpRoute(void)
{
	LOG_INFO("\n");

	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	int size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	do {
		switch(sa_family) {
		case AF_INET:
		case AF_INET6:
			res = snprintf(ptr, size, "ip %sroute add", sa_family == AF_INET6 ? "-6 ":"");
			break;
		case RTNL_FAMILY_IPMR:
		case RTNL_FAMILY_IP6MR:
			//res = snprintf(ptr, size, "ip %smroute add", sa_family == RTNL_FAMILY_IP6MR ? "-6 ":"");
			//break;
		default:
			LOG_ERROR("unsupported family: %u (%s)\n", sa_family, ipfamilyname(sa_family));
			return false;
		};
       
		if( res < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.type && ((res = snprintf(ptr, size, " %s", rttype(osdep.type))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( (res = snprintf(ptr, size, " %S", valid.destIpandMask ? destIpandMask.c_str():L"default")) < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.tos && osdep.tos && ((res = snprintf(ptr, size, " tos %u", osdep.tos)) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.table && ((res = snprintf(ptr, size, " table %u", osdep.table)) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.protocol && ((res = snprintf(ptr, size, " proto %s", rtprotocoltype(osdep.protocol))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.scope && ((res = snprintf(ptr, size, " scope %s", rtscopetype(osdep.scope))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.metric && ((res = snprintf(ptr, size, " metric %u", osdep.metric)) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		// TODO: RTA_TTL_PROPAGATE

		if( valid.nhid ) {

			if( (res = snprintf(ptr, size, " nhid %u", osdep.nhid)) < 0 || size < res )
				break;

			ptr += res;
			size -= res;
			res = 0;

		} else {

		if( valid.encap && ((res = snprintf(ptr, size, " encap %s", GetEncap(osdep.enc))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.gateway && ((res = snprintf(ptr, size, " via %s %S", ipfamilyname(sa_family), gateway.c_str())) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.iface && ((res = snprintf(ptr, size, " dev %S", iface.c_str())) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && ((res = snprintf(ptr, size, "%s%s", flags & RTNH_F_ONLINK ? " onlink":"", flags & RTNH_F_PERVASIVE ? " pervasive":"")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		// TODO: OPTIONS FLAGS

		if( valid.rtnexthop && ((res = snprintf(ptr, size, "%s", GetNextHopes())) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		}

		return RootExec(buf.get()) == 0;

	} while(0);

	return false;
}

bool IpRouteInfo::DeleteIpRoute(void)
{

	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	int size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	do {
		switch(sa_family) {
		case AF_INET:
		case AF_INET6:
			res = snprintf(ptr, size, "ip %sroute del", sa_family == AF_INET6 ? "-6 ":"");
			break;
		case RTNL_FAMILY_IPMR:
		case RTNL_FAMILY_IP6MR:
			//res = snprintf(ptr, size, "ip %smroute del", sa_family == RTNL_FAMILY_IP6MR ? "-6 ":"");
			//break;
		default:
			LOG_ERROR("unsupported family: %u (%s)\n", sa_family, ipfamilyname(sa_family));
			return false;
		};
       
		if( res < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;
		if( valid.type && ((res = snprintf(ptr, size, " %s", rttype(osdep.type))) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;
		if( (res = snprintf(ptr, size, " %S", valid.destIpandMask ? destIpandMask.c_str():L"default")) < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;
		if( valid.table && ((res = snprintf(ptr, size, " table %u", osdep.table)) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;
		if( valid.nhid ) {
			if( (res = snprintf(ptr, size, " nhid %u", osdep.nhid)) < 0 || size < res )
				break;
			ptr += res;
			size -= res;
			res = 0;
		}

		return RootExec(buf.get()) == 0;

	} while(0);

	return false;
}

bool RuleRouteInfo::DeleteRule(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	const size_t size = MAX_CMD_LEN+1;
	rule.clear();
	ToRuleString(true);
	int res = 0;
	switch(family) {
	case AF_INET:
	case AF_INET6:
		res = snprintf(buf.get(), size, "ip %srule del %S", family == AF_INET6 ? "-6 ":"", rule.c_str());
		break;
	case RTNL_FAMILY_IPMR:
	case RTNL_FAMILY_IP6MR:
		res = snprintf(buf.get(), size, "ip %smrule del %S", family == RTNL_FAMILY_IP6MR ? "-6 ":"", rule.c_str());
		break;
	default:
		return false;
	};

	return res > 0 ? (RootExec(buf.get()) == 0):false;
}

bool RuleRouteInfo::CreateRule(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	const size_t size = MAX_CMD_LEN+1;
	rule.clear();
	ToRuleString(true);
	int res = 0;

	switch(family) {
	case AF_INET:
	case AF_INET6:
		res = snprintf(buf.get(), size, "ip %srule add %S", family == AF_INET6 ? "-6 ":"", rule.c_str());
		break;
	case RTNL_FAMILY_IPMR:
	case RTNL_FAMILY_IP6MR:
		res = snprintf(buf.get(), size, "ip %smrule add %S", family == RTNL_FAMILY_IP6MR ? "-6 ":"", rule.c_str());
		break;
	default:
		return false;
	};

	return res > 0 ? (RootExec(buf.get()) == 0):false;
}

bool RuleRouteInfo::ChangeRule(RuleRouteInfo & ipr)
{
	if( DeleteRule() ) {
		if( ipr.CreateRule() ) {
			*this = ipr;
			return true;
		}
	}
	return false;
}

bool ArpRouteInfo::Create(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	int size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	do {
		if( (res = snprintf(ptr, size, "ip %sneigh add", sa_family == AF_INET6 ? "-6 ":"")) < 0 || size < res )
			break;

		ptr += res;
		size -= res;
		res = 0;


		if( valid.flags && flags & NTF_PROXY ) {
			if( valid.ip && ((res = snprintf(ptr, size, " proxy %S", ip.c_str())) < 0 || size < res) )
				break;
		} else {
			if( valid.ip && ((res = snprintf(ptr, size, " %S", ip.c_str())) < 0 || size < res) )
				break;

			ptr += res;
			size -= res;
			res = 0;

			if( valid.mac && ((res = snprintf(ptr, size, " lladdr %S", mac.c_str())) < 0 || size < res) )
				break;

			ptr += res;
			size -= res;
			res = 0;

			if( valid.state && ((res = snprintf(ptr, size, " nud %s", ndmsgstate(state))) < 0 || size < res) )
				break;

		}

		ptr += res;
		size -= res;
		res = 0;

		if( valid.iface && ((res = snprintf(ptr, size, " dev %S", iface.c_str())) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;


		if( valid.flags && flags & NTF_ROUTER && ((res = snprintf(ptr, size, " router")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && flags & NTF_USE && ((res = snprintf(ptr, size, " use")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags_ext && flags_ext & NTF_EXT_MANAGED && ((res = snprintf(ptr, size, " managed")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && flags & NTF_EXT_LEARNED && ((res = snprintf(ptr, size, " extern_learn")) < 0 || size < res) )
			break;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.protocol && ((res = snprintf(ptr, size, " protocol %s", rtprotocoltype(protocol))) < 0 || size < res) )
			break;

		return RootExec(buf.get()) == 0;

	} while(0);

	return false;
}

bool ArpRouteInfo::Delete(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	int size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	if( (res = snprintf(ptr, size, "ip neigh del%s", (valid.flags && flags & NTF_PROXY) ? " proxy":"")) < 0 || size < res )
		return false;

	ptr += res;
	size -= res;
	res = 0;

	if( valid.ip && ((res = snprintf(ptr, size, " %S", ip.c_str())) < 0 || size < res) )
		return false;

	ptr += res;
	size -= res;
	res = 0;

	if( valid.iface && ((res = snprintf(ptr, size, " dev %S", iface.c_str())) < 0 || size < res) )
		return false;

	return RootExec(buf.get()) == 0;
}

#else
bool IpRouteInfo::CreateIpRoute(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	int size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	switch(sa_family) {
	case AF_INET:
		if( (res = snprintf(ptr, size, "route -n add -inet %S", destIpandMask.c_str())) < 0 || size < res )
			return false;
		break;
	case AF_INET6:
		if( (res = snprintf(ptr, size, "route -n add -inet6 %S", destIpandMask.c_str())) < 0 || size < res )
			return false;
		break;
	default:
		LOG_ERROR("unsupported family %u(%s)\n", sa_family, familyname(sa_family));
		return false;
	};

	ptr += res;
	size -= res;
	res = 0;

	if( valid.gateway ) {
		switch( osdep.gateway_type ) {
		case IpGatewayType:
			if( (res = snprintf(ptr, size, " %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		case InterfaceGatewayType:
			if( (res = snprintf(ptr, size, " -interface %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		case MacGatewayType:
			if( (res = snprintf(ptr, size, " -link %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		default:
			break;
		}
	}

	ptr += res;
	size -= res;
	res = 0;

	if( valid.flags && ((res = snprintf(ptr, size, "%s%s%s%s%s%s%s%s",
		flags & RTF_BLACKHOLE ? " -blackhole":"",
		flags & RTF_REJECT ? " -reject":"",
		flags & RTF_STATIC ? " -static":" -nostatic",
		flags & RTF_CLONING ? " -cloning":"",
		flags & RTF_PROTO1 ? " -proto1":"",
		flags & RTF_PROTO2 ? " -proto2":"",
		flags & RTF_XRESOLVE ? " -xresolve":"",
		flags & RTF_LLINFO ? " -llinfo":"")) < 0 || size < res) )
		return false;

	ptr += res;
	size -= res;
	res = 0;

	if( valid.flags && valid.iface && (flags & RTF_IFSCOPE) )
		if( (res = snprintf(ptr, size, " -ifscope %S", iface.c_str() )) < 0 || size < res )
			return false;

	return RootExec(buf.get()) == 0;
}

bool IpRouteInfo::DeleteIpRoute(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	size_t size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	switch(sa_family) {
	case AF_INET:
		if( (res = snprintf(ptr, size, "route -n delete -inet %S", destIpandMask.c_str() )) < 0 || size < res )
			return false;
		break;
	case AF_INET6:
		if( (res = snprintf(ptr, size, "route -n delete -inet6 %S", destIpandMask.c_str() )) < 0 || size < res )
			return false;
		break;
	default:
		LOG_ERROR("unsupported family %u(%s)\n", sa_family, familyname(sa_family));
		return false;
	};

	ptr += res;
	size -= res;
	res = 0;

	if( valid.gateway ) {
		switch( osdep.gateway_type ) {
		case IpGatewayType:
			if( (res = snprintf(ptr, size, " %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		case InterfaceGatewayType:
			if( (res = snprintf(ptr, size, " -interface %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		case MacGatewayType:
			if( (res = snprintf(ptr, size, " -link %S", gateway.c_str())) < 0 || size < res )
				return false;
			break;
		default:
			break;
		}
	}

	ptr += res;
	size -= res;
	res = 0;

	if( valid.flags && valid.iface && (flags & RTF_IFSCOPE) )
		if( (res = snprintf(ptr, size, " -ifscope %S", iface.c_str() )) < 0 || size < res )
			return false;

	return RootExec(buf.get()) == 0;
}

bool ArpRouteInfo::Create(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	size_t size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	switch(sa_family) {
	case AF_INET:
		if( (res = snprintf(ptr, size, "arp -S -n %S %S", ip.c_str(), mac.c_str())) < 0 || size < res )
			return false;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && ((res = snprintf(ptr, size, "%s%s%s",
			flags & RTF_BLACKHOLE ? " blackhole":"",
			flags & RTF_REJECT ? " reject":"",
			flags & RTF_ANNOUNCE ? " pub":"")) < 0 || size < res) )
			return false;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && valid.iface && (flags & RTF_IFSCOPE) )
			if( (res = snprintf(ptr, size, " ifscope %S", iface.c_str() )) < 0 || size < res )
				return false;
		break;
	case AF_INET6:
		if( (res = snprintf(ptr, size, "ndp -s -n %S", ip.c_str())) < 0 || size < res )
			return false;
		ptr += res;
		size -= res;
		res = 0;

		if( valid.iface && ((res = snprintf(ptr, size, "%%%S", iface.c_str())) < 0 || size < res) )
			return false;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.mac && ((res = snprintf(ptr, size, " %S", mac.c_str())) < 0 || size < res) )
			return false;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && ((res = snprintf(ptr, size, "%s",
			flags & RTF_ANNOUNCE ? " proxy":"")) < 0 || size < res) )
			return false;
		break;
	default:
		LOG_ERROR("unsupported family %u(%s)\n", sa_family, familyname(sa_family));
		return false;
	};

	return RootExec(buf.get()) == 0;
}

bool ArpRouteInfo::Delete(void)
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	char * ptr = buf.get();
	size_t size = MAX_CMD_LEN+1;
	int res = 0;
	*ptr = 0;

	switch(sa_family) {
	case AF_INET:
		if( (res = snprintf(ptr, size, "arp -n -d %S", ip.c_str())) < 0 || size < res )
			return false;

		ptr += res;
		size -= res;
		res = 0;

		if( valid.flags && valid.iface && (flags & RTF_IFSCOPE) )
			if( (res = snprintf(ptr, size, " ifscope %S", iface.c_str() )) < 0 || size < res )
				return false;
		break;
	case AF_INET6:
		if( (res = snprintf(ptr, size, "ndp -d -n %S", ip.c_str())) < 0 || size < res )
			return false;
		ptr += res;
		size -= res;
		res = 0;

		if( valid.iface && ((res = snprintf(ptr, size, "%%%S", iface.c_str())) < 0 || size < res) )
			return false;

		break;
	default:
		LOG_ERROR("unsupported family %u(%s)\n", sa_family, familyname(sa_family));
		return false;
	};

	return RootExec(buf.get()) == 0;
}

#endif

bool ArpRouteInfo::Change(ArpRouteInfo & arpr)
{
	if( Delete() ) {
		if( arpr.Create() ) {
			*this = arpr;
			return true;
		}
	}
	return false;
}

bool IpRouteInfo::ChangeIpRoute(IpRouteInfo & ipr)
{
	if( DeleteIpRoute() ) {
		if( ipr.CreateIpRoute() ) {
			*this = ipr;
			return true;
		}
	}
	return false;
}

void ArpRouteInfo::Log(void) const
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	LOG_INFO("%8S: %28S      -> %26S       0x%08X(%s) 0x%02X\n", iface.c_str(), ip.c_str(), mac.c_str(), flags, RouteFlagsToString(flags, 0), hwType);
#else
	LOG_INFO("%8S: %28S      -> %26S       0x%08X(%s) %d\n", iface.c_str(), ip.c_str(), mac.c_str(), flags, RouteFlagsToString(flags, 0), ifnameIndex);
#endif
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
void ArpRouteInfo::LogCacheInfo(void) const
{
	static int hz = 0;

	if( !hz )
		hz = sysconf(_SC_CLK_TCK);

	if( !hz )
		hz = 100;

	if( valid.ci ) {
		LOG_INFO("    ndm_confirmed: %u ( %s)\n", ci.ndm_confirmed, msec_to_str(SEC_TO_MS(ci.ndm_confirmed/hz)));
		LOG_INFO("    ndm_used:      %u ( %s)\n", ci.ndm_used, msec_to_str(SEC_TO_MS(ci.ndm_used/hz)));
		LOG_INFO("    ndm_updated:   %u ( %s)\n", ci.ndm_updated, msec_to_str(SEC_TO_MS(ci.ndm_updated/hz)));
		LOG_INFO("    ndm_refcnt:    %u\n", ci.ndm_refcnt);
	}
}
#endif
