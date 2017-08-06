#include <nss.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nss_command.h"

enum nss_status  _nss_command_gethostbyname_r(const char* name, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	const int addressSize = sizeof(struct in_addr);
	const size_t minbuffer = (
		sizeof(struct hostent)
		+ (addressSize * 2)
		+ ((sizeof(char*) * 3) * 2 )
		+ (5 * 2)
		+ 6 + 1
	);
	if (bufferSize < minbuffer)
	{
		*errnop = ERANGE;
		*herrnop = ERANGE;
		return NSS_STATUS_TRYAGAIN;
	}
	memset(buffer, 0, bufferSize);
	struct hostent* resolved = (struct hostent*) buffer;
	struct in_addr* address1 = (struct in_addr*) buffer + sizeof(struct hostent);
	struct in_addr* address2 = (struct in_addr*) address1 + addressSize;
	char** addresses = (char**) address2 + addressSize;
	char* alias1 = (char*) addresses + (sizeof(char*)*3);
	char* alias2 = (char*) alias1 + 5;
	char** aliases = (char**) alias2 + 5;
	char* hostname = (char*) aliases + (sizeof(char*)*3);
	char* endbuffer = (char*) hostname +1;

	*endbuffer = '\0';
	strncpy(hostname, "eeepa", 6);
	strncpy(alias1, "blah", 5);
	strncpy(alias2, "euee", 5);
	aliases[0] = alias1;
	aliases[1] = alias2;
	aliases[2] = NULL;
	inet_aton("127.0.0.1", address1);
	inet_aton("127.0.0.2", address2);
	addresses[0] = (char*) address1;
	addresses[1] = (char*) address2;
	addresses[2] = NULL;
	resolved->h_name = hostname;
	resolved->h_aliases = aliases;
	resolved->h_addrtype = AF_INET;
	resolved->h_length = addressSize;
	resolved->h_addr_list = addresses;
	resolved->h_addr = resolved->h_addr_list[0];
	if (result == NULL) result = resolved;
	else memcpy(result, resolved, sizeof(struct hostent));
	*herrnop = NETDB_SUCCESS;
	*errnop = 0;
	return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_command_gethostbyname2_r(const char* name, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	if (addressFamily != AF_INET)
	{
		*herrnop = NETDB_INTERNAL;
		*errnop = EAFNOSUPPORT;
		return NSS_STATUS_RETURN;
	}
	return _nss_command_gethostbyname_r(name, result, buffer, bufferSize, errnop, herrnop);
}

enum nss_status _nss_command_gethostbyaddr_r(const void* address, socklen_t addressSize, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	if (addressFamily != AF_INET || addressSize != 4)
	{
		*herrnop = NETDB_INTERNAL;
		*errnop = EAFNOSUPPORT;
		return NSS_STATUS_RETURN;
	}
	return _nss_command_gethostbyname_r("", result, buffer, bufferSize, errnop, herrnop);
}

/*
To be implemented

nss_status _nss_command_sethostent(int stayopen)
{}

nss_status _nss_command_gethostent_r(hostent* host, char* buffer, size_t bufferSize, int* errnop, int* herrnop, int addressFamily, int flags)
{}

nss_status _nss_command_gethostent_r(hostent* host, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{}

nss_status _nss_command_endhostent(void)
{}
*/
