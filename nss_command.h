#include <nss.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

enum nss_status  _nss_command_gethostbyname_r(const char* name, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);

enum nss_status _nss_command_gethostbyname2_r(const char* name, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);

enum nss_status _nss_command_gethostbyaddr_r(const void* address, socklen_t addressSize, int addressFamily, struct hostent* result, char* buffer, size_t bufferSize, int* errnop, int* herrnop);


#ifdef __cplusplus
}
#endif
