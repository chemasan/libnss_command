#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

char* formatAddr(char* addr)
{
	if (addr == NULL) return NULL;
	in_addr ipaddr;
	memcpy(&ipaddr, addr, sizeof(in_addr));
	return inet_ntoa(ipaddr);
}

void printHost(hostent* host)
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

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		cerr << "usage: " << argv[0] << " <hostname>" << endl;
		return 1;
	}
	in_addr addr;
	if (inet_aton(argv[1], &addr) != 1)
	{
		cerr << "invalid address" << endl;
		return 1;
	}
	hostent* result = gethostbyaddr(&addr, sizeof(in_addr), AF_INET);
	if (result == NULL)
	{
		cerr << hstrerror(h_errno) << endl;
		return 1;
	}
	printHost(result); 
}

