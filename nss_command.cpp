/*
 Copyright (c) 2017 Jose Manuel Sanchez Madrid.
 This file is licensed under MIT license. See file LICENSE for details.
*/

#include <nss.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nss_command.hpp"
#include <cstring>
#include <regex>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char* DEFAULT_GETHOSTBYNAME_COMMAND = "/usr/local/sbin/nsscommand_gethostbyname";
const char* DEFAULT_GETHOSTBYADDR_COMMAND = "/usr/local/sbin/nsscommand_gethostbyaddr";

using namespace std;


bool operator == (const in_addr& lhs, const in_addr& rhs)
{
	return lhs.s_addr == rhs.s_addr;
}

namespace nssCommand
{
	bool fileHasRightPerms(const string& filename)
	{
		struct stat fileProperties;
		int statResult = stat(filename.c_str(), &fileProperties);
		if (statResult != 0)  return false;
		if (fileProperties.st_mode != 0100755) 	return false;
		if (fileProperties.st_uid != 0) return false;
		return true;
	}
	const regex namere("^name:\\s*([a-zA-Z0-9\\-\\.]+)$");
	const regex aliasre("^alias:\\s*([a-zA-Z0-9\\-\\.]+)$");
	const regex ip4re("^ip4:\\s*([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)$");
	HostEntry parseCommandOutput(const string& text)
	{
		stringstream textStream(text);
		HostEntry result;
		string currentLine;
		while (getline( textStream, currentLine))
		{
			smatch matchResult;
			if (regex_match(currentLine, matchResult, namere))
			{
				result.name = matchResult[1];
			}
			else if (regex_match(currentLine, matchResult, aliasre))
			{
				result.aliases.emplace_back(matchResult[1]);
			}
			else if (regex_match(currentLine, matchResult, ip4re))
			{
				in_addr ip;
				if ( 0 != inet_aton(matchResult[1].str().c_str(), &ip))  result.addresses.emplace_back(ip);
			}
		}
		return result;
	}
	string getErrorDescription(int errorcode)
	{
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		strerror_r(errorcode, buffer, sizeof(buffer));
		return string(buffer);
	}
	int run(const string& cmd, string& output)
	{
		char line[256];
		memset(line,0,256);
		FILE* p = popen(cmd.c_str(),"r");
		if (p == NULL) throw runtime_error("No memory left");
		output.clear();
		while (fgets(line,255,p)) output.append(line);
		int returnValue = pclose(p);
		if (returnValue == -1) throw runtime_error(getErrorDescription(errno));
		if (WIFEXITED(returnValue)) returnValue = WEXITSTATUS(returnValue);
		return returnValue;
	}
	size_t calculateBufferSize(const HostEntry& entry)
	{
		size_t result = 0;
		result = entry.name.size() +1; // size for h_name
		for (auto& alias : entry.aliases) result += (alias.size() +1); // size for each entry in h_aliases
		result += (entry.addresses.size() * sizeof(in_addr));  //size for each entry in h_addr_list
		result += ((entry.aliases.size()+1) * sizeof(char*)); // size of the h_aliases vector itself
		result += ((entry.addresses.size()+1) * sizeof(char*)); //size of the h_addr_list itself
		return result;
	}
	size_t calculateGaihBufferSize(const HostEntry& entry)
	{
		size_t result = 0;
		result = entry.name.size() +1;
		result += sizeof(gaih_addrtuple) * entry.addresses.size();
		return result;
	}
	nss_status smallBufferExit(int* errnop, int* herrorp)
	{
			*errnop = ERANGE;
			*herrorp = NETDB_INTERNAL;
			return NSS_STATUS_TRYAGAIN;
	}
	nss_status noDataExit(int* errnop, int* herrorp)
	{
			*errnop = 0;
			*herrorp = NO_DATA;
			return NSS_STATUS_UNAVAIL;
	}
	nss_status notFoundExit(int* errnop, int* herrorp)
	{
			*errnop = 0;
			*herrorp = HOST_NOT_FOUND;
			return NSS_STATUS_NOTFOUND;
	}
	nss_status tryAgainExit(int* errnop, int* herrorp)
	{
			*errnop = 0;
			*herrorp = TRY_AGAIN;
			return NSS_STATUS_TRYAGAIN;
	}
	nss_status notAvailableExit(int* errnop, int* herrorp)
	{
			*errnop = 0;
			*herrorp = NO_RECOVERY;
			return NSS_STATUS_UNAVAIL;
	}
	nss_status successfulExit(int* errnop, int* herrorp)
	{
		*errnop=0;
		*herrorp=NETDB_SUCCESS;
		return NSS_STATUS_SUCCESS;
	}
	nss_status unsuccessfulCommandExit(int commandReturnCode, int* errnop, int* herrorp)
	{
		switch (commandReturnCode)
		{
			case 1:
				return notFoundExit(errnop, herrorp);
			case 2:
				return tryAgainExit(errnop, herrorp);
			case 4:
				return noDataExit(errnop, herrorp);
			case 3:
			default:
				return notAvailableExit(errnop, herrorp);
		}
	}
	void copyHostEntryToBuffer(const HostEntry& parsedEntry, hostent* result, char* buffer, size_t bufferSize)
	{
		memset(buffer, 0, bufferSize);
		hostent* resultInBuffer = (hostent*) buffer;
		resultInBuffer->h_addrtype = AF_INET;
		resultInBuffer->h_length = sizeof(in_addr);
		resultInBuffer->h_aliases = (char**) resultInBuffer + sizeof(hostent);
		resultInBuffer->h_addr_list = (char**) resultInBuffer->h_aliases + ((parsedEntry.aliases.size() +1) * sizeof(char*));
		resultInBuffer->h_name = (char*) resultInBuffer->h_addr_list + ((parsedEntry.addresses.size() +1) * sizeof(char*));
		strncpy(resultInBuffer->h_name, parsedEntry.name.c_str(), parsedEntry.name.size());
		char* lastPointer = resultInBuffer->h_name + (parsedEntry.name.size() +1);
		for (int i = 0; i<parsedEntry.aliases.size(); i++)
		{
			auto& alias = parsedEntry.aliases[i];
			strncpy(lastPointer, alias.c_str(), alias.size());
			resultInBuffer->h_aliases[i] = lastPointer;
			lastPointer += (alias.size() + 1);
		}
		for (int i = 0; i<parsedEntry.addresses.size(); i++)
		{
			auto& address = parsedEntry.addresses[i];
			in_addr* addressPointer = (in_addr*) lastPointer;
			*addressPointer = address;
			resultInBuffer->h_addr_list[i] = lastPointer;
			lastPointer += sizeof(in_addr);
		}
		memcpy(result, buffer, sizeof(hostent));
	}
	void copyHostEntryToGaihBuffer(const HostEntry& parsedEntry, gaih_addrtuple** result, char* buffer, size_t bufferSize)
	{
		memset(buffer, 0, bufferSize);
		gaih_addrtuple* resultsBuffer = (gaih_addrtuple*) buffer;
		char* hostnamePointer = buffer + (sizeof(gaih_addrtuple) * parsedEntry.addresses.size());
		strncpy(hostnamePointer, parsedEntry.name.c_str(), parsedEntry.name.size());
		for (int i=0; i< parsedEntry.addresses.size(); i++)
		{
			resultsBuffer[i].next = (resultsBuffer + i +1);
			resultsBuffer[i].family = AF_INET;
			resultsBuffer[i].name = hostnamePointer;
			resultsBuffer[i].scopeid = 0;
			resultsBuffer[i].addr[0] = parsedEntry.addresses[i].s_addr;
		}
		resultsBuffer[parsedEntry.addresses.size()-1].next = nullptr;
		*result = resultsBuffer;
	}
	nss_status runNssCommandGethostbyname(const char* name, hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrorp, const char* command)
	{
		string commandAndArgs = string(command) + " \'" + string(name) + "\' 2>/dev/null";
		string commandOutput;
		int commandReturnCode = run(commandAndArgs, commandOutput);
		if (commandReturnCode != 0)  return unsuccessfulCommandExit(commandReturnCode, errnop, herrorp);
		HostEntry parsedEntry = parseCommandOutput(commandOutput);
		if (parsedEntry.addresses.empty())  return noDataExit(errnop, herrorp);
		size_t necessaryBuffer = sizeof(hostent) + calculateBufferSize(parsedEntry);
		if (necessaryBuffer > bufferSize)  return smallBufferExit(errnop, herrorp);
		copyHostEntryToBuffer(parsedEntry, result, buffer, bufferSize);
		return successfulExit(errnop, herrorp);
	}
	nss_status runNssCommandGethostbyname4(const char* name, gaih_addrtuple** pat, char* buffer, size_t bufferSize, int* errnop, int* herrorp, int32_t* ttlp, const char* command)
	{
		string commandAndArgs = string(command) + " \'" + string(name) + "\' 2>/dev/null";
		string commandOutput;
		int commandReturnCode = run(commandAndArgs, commandOutput);
		if (commandReturnCode != 0)  return unsuccessfulCommandExit(commandReturnCode, errnop, herrorp);
		HostEntry parsedEntry = parseCommandOutput(commandOutput);
		if (parsedEntry.addresses.empty())  return noDataExit(errnop, herrorp);
		size_t necessaryBuffer = calculateGaihBufferSize(parsedEntry);
		if (necessaryBuffer > bufferSize)  return smallBufferExit(errnop, herrorp);
		copyHostEntryToGaihBuffer(parsedEntry, pat, buffer, bufferSize);
		return successfulExit(errnop, herrorp);
	}
	string ip4ToString(const void* address)
	{
		socklen_t bufferSize = 512;
		char* buffer = new char[bufferSize];
		while(inet_ntop(AF_INET, address, buffer, bufferSize) == nullptr && errno == ENOSPC)
		{
			bufferSize *= 2;
			delete[] buffer;
			buffer = new char[bufferSize];
		}
		string result(buffer);
		delete[] buffer;
		return result;
	}
	nss_status runNssCommandGethostbyaddr(const void* address, socklen_t addressSize, int addressFamily, hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop, const char* command)
	{
		if (addressFamily == AF_INET6) return notFoundExit(errnop, herrnop);
		string commandAndArgs = string(command) + " \'" + ip4ToString(address) + "\' 2>/dev/null";
		string commandOutput;
		int commandReturnCode = run(commandAndArgs, commandOutput);
		if (commandReturnCode != 0)  return unsuccessfulCommandExit(commandReturnCode, errnop, herrnop);
		HostEntry parsedEntry = parseCommandOutput(commandOutput);
		if (parsedEntry.name.empty())  return noDataExit(errnop, herrnop);
		size_t necessaryBuffer = sizeof(hostent) + calculateBufferSize(parsedEntry);
		if (necessaryBuffer > bufferSize)  return smallBufferExit(errnop, herrnop);
		copyHostEntryToBuffer(parsedEntry, result, buffer, bufferSize);
		return successfulExit(errnop, herrnop);
	}
}

using namespace nssCommand;

enum nss_status  _nss_command_gethostbyname_r(const char* name, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	if (! fileHasRightPerms(DEFAULT_GETHOSTBYNAME_COMMAND)) return notAvailableExit(errnop, herrnop);
	return nssCommand::runNssCommandGethostbyname(name, result, buffer, bufferSize, errnop, herrnop, DEFAULT_GETHOSTBYNAME_COMMAND);
}

enum nss_status _nss_command_gethostbyname2_r(const char* name, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	if (addressFamily != AF_INET) return nssCommand::notFoundExit(errnop, herrnop);
	if (! fileHasRightPerms(DEFAULT_GETHOSTBYNAME_COMMAND)) return notAvailableExit(errnop, herrnop);
	return nssCommand::runNssCommandGethostbyname(name, result, buffer, bufferSize, errnop, herrnop, DEFAULT_GETHOSTBYNAME_COMMAND);
}

enum nss_status _nss_command_gethostbyname3_r(const char* name, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop, int32_t* ttlp, char** canonp)
{
	if (addressFamily != AF_INET) return nssCommand::notFoundExit(errnop, herrnop);
	if (! fileHasRightPerms(DEFAULT_GETHOSTBYNAME_COMMAND)) return notAvailableExit(errnop, herrnop);
	nss_status ret = nssCommand::runNssCommandGethostbyname(name, result, buffer, bufferSize, errnop, herrnop, DEFAULT_GETHOSTBYNAME_COMMAND);
	if (canonp != nullptr) *canonp = result->h_name;
	return ret;
}
enum nss_status _nss_command_gethostbyname4_r(const char* name, struct gaih_addrtuple** pat, char* buffer, size_t bufferSize, int* errnop, int* herrnop, int32_t* ttlp)
{
	if (! fileHasRightPerms(DEFAULT_GETHOSTBYNAME_COMMAND)) return notAvailableExit(errnop, herrnop);
	return nssCommand::runNssCommandGethostbyname4(name, pat, buffer, bufferSize, errnop, herrnop, ttlp, DEFAULT_GETHOSTBYNAME_COMMAND);
}

enum nss_status _nss_command_gethostbyaddr_r(const void* address, socklen_t addressSize, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop)
{
	if (! fileHasRightPerms(DEFAULT_GETHOSTBYADDR_COMMAND)) return notAvailableExit(errnop, herrnop);
	return nssCommand::runNssCommandGethostbyaddr(address, addressSize, addressFamily, result, buffer, bufferSize, errnop, herrnop, DEFAULT_GETHOSTBYADDR_COMMAND);
}

//enum nss_status _nss_command_gethostbyaddr2_r(const void* addr, socklen_t len, int af, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* h_errhop, int32_t* ttlp)
//{}
