/*
 Copyright (c) 2017 Jose Manuel Sanchez Madrid.
 This file is licensed under MIT license. See file LICENSE for details.
*/

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "nss_command.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <vector>

using namespace std;
using namespace nssCommand;

char* formatAddr(const char* addr)
{
	if (addr == nullptr) return nullptr;
	in_addr ipaddr;
	memcpy(&ipaddr, addr, sizeof(in_addr));
	return inet_ntoa(ipaddr);
}
void printHost(const hostent* host)
{
	if (host == nullptr) return;
	cout << "h_name: " << host->h_name << endl;
	for (int i = 0; i < 100; i++)
	{
		if (host->h_aliases[i] == nullptr) break;
		cout << "h_aliases[" << i << "]: " << host->h_aliases[i] << endl;
	}
	cout << "h_addrtype: " << host->h_addrtype << endl;
	cout << "h_length: " << host->h_length << endl;
	for (int i = 0; i < 100; i++)
	{
		if (host->h_addr_list[i] == nullptr) break;
		cout << "h_addr_list[" << i << "]: " << formatAddr(host->h_addr_list[i]) << endl;
	}
	cout << "h_addr: " << formatAddr(host->h_addr) << endl;
}

TEST_CASE("parseCommandOutput returns the parsed HostEntry of the given string")
{
	string tobeparsed = "name: myhost.domain.tld.\nalias: myalias.domain.tld.\nip4:127.0.0.3\nip4:127.0.0.4\n";
	HostEntry expected;
	expected.name = "myhost.domain.tld.";
	expected.aliases = { "myalias.domain.tld." };
	in_addr expectedAddress1;
	in_addr expectedAddress2;
	inet_aton("127.0.0.3", &expectedAddress1);
	inet_aton("127.0.0.4", &expectedAddress2);
	expected.addresses = { expectedAddress1, expectedAddress2 };

	HostEntry result = parseCommandOutput(tobeparsed);

	CHECK( result == expected );
}

TEST_CASE("run executes the commands in a subshell and fills the output argument with the execution output")
{
	string command = "echo \"hello world\"; echo \"bye world\"; exit 1";
	string expectedOutput = "hello world\nbye world\n";
	int expectedCode = 1;
	string returnedOutput;

	int returnedCode = run(command, returnedOutput);

	CHECK(returnedCode == expectedCode);
	CHECK(returnedOutput == expectedOutput);
}

TEST_CASE("calculateBufferSize returns the nesessary bytes to store the data in the given entry")
{
	HostEntry entry;
	entry.name = "myhost.domain.tld."; //size 18 + 1
	entry.aliases = { "myalias.domain.tld." }; // size 19 + 1
	in_addr address1;
	in_addr address2;
	inet_aton("127.0.0.3", &address1);
	inet_aton("127.0.0.4", &address2);
	entry.addresses = { address1, address2 }; //ipv4 addresses is fixed size of 4 bytes
	size_t expected = 47 + (sizeof(char*) * 5); //size of data + size of the null terminated vectors

	size_t result = calculateBufferSize(entry);

	CHECK( result == expected );
}

TEST_CASE("runNssCommandGethostbyname executes the given command and returns the resolved hostent when given buffer is big enough")
{
	in_addr expectedAddress1;
	in_addr expectedAddress2;
	inet_aton("127.0.0.1", &expectedAddress1);
	inet_aton("127.0.0.2", &expectedAddress2);
	string expectedName = "myhost.local.";
	string expectedAlias1 = "myhost";
	string expectedAlias2 = "myalias.local.";

	const char* hostname = "myhost";
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname(hostname, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_SUCCESS );
	REQUIRE( error == 0 );
	REQUIRE( herror == NETDB_SUCCESS );
	string resultName(result.h_name);
	string resultAlias1(result.h_aliases[0]);
	string resultAlias2(result.h_aliases[1]);
	in_addr* resultAddress1 = (in_addr*) result.h_addr_list[0];
	in_addr* resultAddress2 = (in_addr*) result.h_addr_list[1];
	in_addr* resultAddress = (in_addr*) result.h_addr;
	CHECK( resultName == expectedName );
	CHECK( resultAlias1 == expectedAlias1 );
	CHECK( resultAlias2 == expectedAlias2 );
	CHECK( *resultAddress1 == expectedAddress1 );
	CHECK( *resultAddress2 == expectedAddress2 );
	CHECK( *resultAddress == expectedAddress1 );
	CHECK(result.h_addrtype == AF_INET);
	CHECK(result.h_length == sizeof(in_addr));
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname returns TRYAGAIN when the provided buffer is not big enough")
{
	const char* hostname = "myhost";
	hostent result;
	size_t bufferSize = 16;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname(hostname, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_TRYAGAIN );
	REQUIRE( error == ERANGE );
	REQUIRE( herror == NETDB_INTERNAL );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname returns NOTFOUND when command returns with code 1")
{
	const char* hostname = "somethingthatdoesntexist";
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname(hostname, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_NOTFOUND );
	REQUIRE( error == 0 );
	REQUIRE( herror == HOST_NOT_FOUND );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname returns UNAVAILABLE when the command fails to execute")
{
	const char* hostname = "somethingthatdoesntexist";
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/nonexistantcommand.sh";
	nss_status returncode = runNssCommandGethostbyname(hostname, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_UNAVAIL );
	REQUIRE( error == 0 );
	REQUIRE( herror == NO_RECOVERY );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyaddr returns NOTFOUND if asked for a ipv6 address")
{
	const char* address = "::1";
	in6_addr ip6address;
	inet_pton(AF_INET6, address, &ip6address);
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyaddr.sh";
	nss_status returncode = runNssCommandGethostbyaddr(&ip6address, sizeof(ip6address), AF_INET6, &result, buffer, bufferSize, &error, &herror, command);
	REQUIRE( returncode == NSS_STATUS_NOTFOUND );
	REQUIRE( error == 0 );
	REQUIRE( herror == HOST_NOT_FOUND );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyaddr executes the given command and returns the resolved hostent when given buffer is big enough")
{
	in_addr expectedAddress1;
	in_addr expectedAddress2;
	inet_aton("127.0.0.1", &expectedAddress1);
	inet_aton("127.0.0.2", &expectedAddress2);
	string expectedName = "myhost.local.";
	string expectedAlias1 = "myhost";
	string expectedAlias2 = "myalias.local.";

	const char* address = "127.0.0.2";
	in_addr ip4address;
	inet_pton(AF_INET, address, &ip4address);
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyaddr.sh";
	nss_status returncode = runNssCommandGethostbyaddr(&ip4address, sizeof(ip4address), AF_INET, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_SUCCESS );
	REQUIRE( error == 0 );
	REQUIRE( herror == NETDB_SUCCESS );
	string resultName(result.h_name);
	string resultAlias1(result.h_aliases[0]);
	string resultAlias2(result.h_aliases[1]);
	in_addr* resultAddress1 = (in_addr*) result.h_addr_list[0];
	in_addr* resultAddress2 = (in_addr*) result.h_addr_list[1];
	in_addr* resultAddress = (in_addr*) result.h_addr;
	CHECK( resultName == expectedName );
	CHECK( resultAlias1 == expectedAlias1 );
	CHECK( resultAlias2 == expectedAlias2 );
	CHECK( *resultAddress1 == expectedAddress1 );
	CHECK( *resultAddress2 == expectedAddress2 );
	CHECK( *resultAddress == expectedAddress1 );
	CHECK(result.h_addrtype == AF_INET);
	CHECK(result.h_length == sizeof(in_addr));
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyaddr returns NOTFOUND when command returns with code 1")
{
	const char* address = "127.0.0.3";
	in_addr ip4address;
	inet_pton(AF_INET, address, &ip4address);
	hostent result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyaddr.sh";
	nss_status returncode = runNssCommandGethostbyaddr(&ip4address, sizeof(ip4address), AF_INET, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_NOTFOUND );
	REQUIRE( error == 0 );
	REQUIRE( herror == HOST_NOT_FOUND );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyaddr returns TRYAGAIN when the provided buffer is not big enough")
{
	const char* address = "127.0.0.2";
	in_addr ip4address;
	inet_pton(AF_INET, address, &ip4address);
	hostent result;
	size_t bufferSize = 24;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/test_gethostbyaddr.sh";
	nss_status returncode = runNssCommandGethostbyaddr(&ip4address, sizeof(ip4address), AF_INET, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_TRYAGAIN );
	REQUIRE( error == ERANGE );
	REQUIRE( herror == NETDB_INTERNAL );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyaddr returns UNAVAILABLE when the command fails to execute")
{
	const char* address = "127.0.0.2";
	in_addr ip4address;
	inet_pton(AF_INET, address, &ip4address);
	hostent result;
	size_t bufferSize = 24;
	char* buffer = new char[bufferSize];
	int error, herror;
	const char* command = "./resources/somethingdoesntexists.sh";
	nss_status returncode = runNssCommandGethostbyaddr(&ip4address, sizeof(ip4address), AF_INET, &result, buffer, bufferSize, &error, &herror, command);

	REQUIRE( returncode == NSS_STATUS_UNAVAIL );
	REQUIRE( error == 0 );
	REQUIRE( herror == NO_RECOVERY );
	delete[] buffer;
}

TEST_CASE("calculateGaihBufferSize returns the nesessary bytes to store the data in the given entry")
{
	HostEntry entry;
	entry.name = "myhost.domain.tld."; //size 18 + 1
	entry.aliases = { "myalias.domain.tld." }; //doesn't count for gaih_addrtuple
	in_addr address1;
	in_addr address2;
	inet_aton("127.0.0.3", &address1);
	inet_aton("127.0.0.4", &address2);
	entry.addresses = { address1, address2 }; //ipv4 addresses is fixed size of 4 bytes
	size_t expected = 19 + (sizeof(gaih_addrtuple) * 2); //size of data + size of gaih_addrtuple * number of ip addres

	size_t result = calculateGaihBufferSize(entry);

	CHECK( result == expected );
}
TEST_CASE("runNssCommandGethostbyname4 returns NOTFOUND when command returns with code 1")
{
	const char* hostname = "somethingthatdoesntexist";
	gaih_addrtuple* result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror, ttlp;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname4(hostname, &result, buffer, bufferSize, &error, &herror, &ttlp, command);

	REQUIRE( returncode == NSS_STATUS_NOTFOUND );
	REQUIRE( error == 0 );
	REQUIRE( herror == HOST_NOT_FOUND );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname4 executes the given command and returns the resolved entries when given buffer is big enough")
{
	in_addr expectedAddress1;
	in_addr expectedAddress2;
	inet_aton("127.0.0.1", &expectedAddress1);
	inet_aton("127.0.0.2", &expectedAddress2);
	string expectedName = "myhost.local.";

	const char* hostname = "myhost";
	gaih_addrtuple* result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror, ttlp;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname4(hostname, &result, buffer, bufferSize, &error, &herror, &ttlp, command);

	REQUIRE( returncode == NSS_STATUS_SUCCESS );
	REQUIRE( error == 0 );
	REQUIRE( herror == NETDB_SUCCESS );

	gaih_addrtuple* result1 = result;
	gaih_addrtuple* result2 = result1->next;

	CHECK(string(result1->name) == expectedName );
	CHECK( result1->addr[0] == expectedAddress1.s_addr );
	CHECK(result1->family == AF_INET);

	CHECK(string(result2->name) == expectedName );
	CHECK( result2->addr[0] == expectedAddress2.s_addr );
	CHECK(result2->family == AF_INET);
	CHECK(result2->next == nullptr);

	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname4 returns TRYAGAIN when the provided buffer is not big enough")
{
	const char* hostname = "myhost";
	gaih_addrtuple* result;
	size_t bufferSize = 16;
	char* buffer = new char[bufferSize];
	int error, herror, ttlp;
	const char* command = "./resources/test_gethostbyname.sh";
	nss_status returncode = runNssCommandGethostbyname4(hostname, &result, buffer, bufferSize, &error, &herror, &ttlp, command);

	REQUIRE( returncode == NSS_STATUS_TRYAGAIN );
	REQUIRE( error == ERANGE );
	REQUIRE( herror == NETDB_INTERNAL );
	delete[] buffer;
}
TEST_CASE("runNssCommandGethostbyname4 returns UNAVAILABLE when the command fails to execute")
{
	const char* hostname = "somethingthatdoesntexist";
	gaih_addrtuple* result;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int error, herror, ttlp;
	const char* command = "./resources/nonexistantcommand.sh";
	nss_status returncode = runNssCommandGethostbyname4(hostname, &result, buffer, bufferSize, &error, &herror, &ttlp, command);

	REQUIRE( returncode == NSS_STATUS_UNAVAIL );
	REQUIRE( error == 0 );
	REQUIRE( herror == NO_RECOVERY );
	delete[] buffer;
}
