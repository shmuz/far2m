#ifndef __NETIFS_H__
#define __NETIFS_H__

#include "netif.h"
#include <vector>

class NetInterfaces {
	private:
		std::map<std::wstring, NetInterface *> ifs;

		bool UpdateByProcNet(void);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
		bool UpdateByNetlink(void);
#endif

		NetInterface * Add(const char * name);

		// copy and assignment not allowed
		NetInterfaces(const NetInterfaces&) = delete;
		void operator=(const NetInterfaces&) = delete;

	public:
		typedef std::map<std::wstring, NetInterface *>::const_iterator const_iterator;
		const_iterator begin() const { return ifs.begin(); };
		const_iterator end() const { return ifs.end(); };
		const_iterator find(std::wstring name) const { return ifs.find(name); };
		int size(void) const { return ifs.size(); };

		NetInterfaces();
		~NetInterfaces();

		// API
		NetInterface * FindByIndex(uint32_t ifindex);

		// tcpdump
		std::vector<std::wstring> tcpdump;
		bool TcpDumpStart(const wchar_t * iface, const wchar_t * file, bool promisc);
		void TcpDumpStop(const wchar_t * iface);

		bool Update(void);
		void Log(void);
		void Clear(void);
};

#endif /* __NETIFS_HS__ */
