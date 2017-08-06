#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "nss_command.h"

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>

using namespace std;

char* formatAddr(const char* addr)
{
	if (addr == NULL) return NULL;
	in_addr ipaddr;
	memcpy(&ipaddr, addr, sizeof(in_addr));
	return inet_ntoa(ipaddr);
}
void printHost(const hostent* host)
{
	if (host == NULL) return;
	cout << "h_name: " << host->h_name << endl;
	for (int i = 0; i < 100; i++)
	{
		if (host->h_aliases[i] == NULL) break;
		cout << "h_aliases[" << i << "]: " << host->h_aliases[i] << endl;
	}
	cout << "h_addrtype: " << host->h_addrtype << endl;
	cout << "h_length: " << host->h_length << endl;
	for (int i = 0; i < 100; i++)
	{
		if (host->h_addr_list[i] == NULL) break;
		cout << "h_addr_list[" << i << "]: " << formatAddr(host->h_addr_list[i]) << endl;
	}
	cout << "h_addr: " << formatAddr(host->h_addr) << endl;
}

bool operator == (const in_addr& lhs, const in_addr& rhs)
{
	return lhs.s_addr == rhs.s_addr;
}

TEST_CASE("_nss_command_gethostbyname_r returns host when called with buffer big enough")
{
	in_addr expectedAddress;
	inet_aton("127.0.0.1", &expectedAddress);
	const char* hostname = "somehostname";
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	nss_status returncode = _nss_command_gethostbyname_r(hostname, &result, buffer, bufferSize, &error, &herror);
	CHECK( returncode == NSS_STATUS_SUCCESS );
	CHECK( error == 0 );
	CHECK( herror == NETDB_SUCCESS );
	in_addr* resultAddress = (in_addr*) result.h_addr_list[0];
	CHECK( *resultAddress == expectedAddress );
	delete[] buffer;
}

TEST_CASE("_nss_command_gethostbyname_r returns TRYAGAIN when buffer is small")
{
	in_addr expectedAddress;
	inet_aton("127.0.0.1", &expectedAddress);
	const char* hostname = "somehostname";
	hostent result;
	size_t bufferSize = 16;
	char* buffer = new char[bufferSize];
	int error, herror;
	nss_status returncode = _nss_command_gethostbyname_r(hostname, &result, buffer, bufferSize, &error, &herror);
	CHECK( returncode == NSS_STATUS_TRYAGAIN );
	CHECK( error == ERANGE );
	CHECK( herror == ERANGE );
	delete[] buffer;
}
