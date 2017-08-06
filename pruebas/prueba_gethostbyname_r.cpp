#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

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

main(int argc, char** argv)
{
	if (argc <= 1)
	{
		cerr << "usage: " << argv[0] << " <hostname>" << endl;
		return 1;
	}
/*
	int gethostbyname_r(
		const char* name,
		hostent* ret,
		char* buffer,
		size_t bufferSize,
		hostent** result,
		int* h_errnop
	)
*/
	const char* hostname = argv[1];
	hostent ret;
	size_t bufferSize = 1024;
	char* buffer = new char[bufferSize];
	hostent* result = NULL;
	int error = 0;
	int returnCode = gethostbyname_r(hostname, &ret, buffer, bufferSize, &result, &error);

	while (returnCode == ERANGE)
	{
		cerr << " * increasing buffer size" << endl;
		delete[] buffer;
		bufferSize *= 2;
		buffer = new char[bufferSize];
		returnCode = gethostbyname_r(hostname, &ret, buffer, bufferSize, &result, &error);
	}
	// notice that both ret and result can be used
	if (returnCode == 0 && result != NULL) printHost(result);
	else cerr << hstrerror(error) << endl;
	delete[] buffer;
	return returnCode;
}
