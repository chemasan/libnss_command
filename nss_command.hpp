#ifndef _NSSCOMMAND_H
#define _NSSCOMMAND_H 1

#include <nss.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <vector>

extern "C" {
	enum nss_status  _nss_command_gethostbyname_r(const char* name, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);
	enum nss_status _nss_command_gethostbyname2_r(const char* name, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);
	enum nss_status _nss_command_gethostbyname3_r(const char* name, int af, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop, int32_t* ttlp, char** canonp);
	enum nss_status _nss_command_gethostbyname4_r(const char* name, struct gaih_addrtuple** pat, char* buffer, size_t bufferSize, int* errnop, int* herrnop, int32_t* ttlp);
	enum nss_status _nss_command_gethostbyaddr_r(const void* address, socklen_t addressSize, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);
}

bool operator == (const in_addr& lhs, const in_addr& rhs);

namespace nssCommand
{
	using namespace std;

	class HostEntry
	{
	public:
		HostEntry() = default;
		HostEntry(const HostEntry&) = default;
		HostEntry(HostEntry&&) = default;
		bool operator == (const HostEntry& rho)
		{
			if (name != rho.name) return false;
			if (aliases != rho.aliases) return false;
			if (addresses != rho.addresses) return false;
			return true;
		}
		string name;
		vector<string> aliases;
		vector<in_addr> addresses;
	};

	HostEntry parseCommandOutput(const string& text);
	int run(const string& cmd, string& output);
	size_t calculateBufferSize(const HostEntry& entry);
	size_t calculateGaihBufferSize(const HostEntry& entry);
	nss_status runNssCommandGethostbyname(const char* name, hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrorp, const char* command);
	nss_status runNssCommandGethostbyaddr(const void* address, socklen_t addressSize, int addressFamily, hostent* result, char* buffer, size_t
bufferSize, int* errnop, int* herrnop, const char* command);
	nss_status runNssCommandGethostbyname4(const char* name, gaih_addrtuple** pat, char* buffer, size_t bufferSize, int* errnop, int* herrorp, int32_t* ttlp, const char* command);
}

#endif
