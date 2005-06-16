#ifndef commonH
#define commonH
        #include <stdlib.h>
        #include <stdio.h>
        #include <time.h>
        #include <string.h>
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
  #include <winsock2.h>
  #include <process.h>
  #include <windows.h>
  #define sleep(x) Sleep((x)*1000)
  #define close(x) closesocket(x)
  #define delay(x) Sleep(x)
  #define snprintf _snprintf
  #define ERRNO_EQUALS(val) (WSAGetLastError() == (val))
  #define EINTR WSAEINTR
  #define ECONNRESET WSAECONNRESET
  #define EADDRINUSE WSAEADDRINUSE
  #define EAFNOSUPPORT WSAEAFNOSUPPORT
  #define ENOTSOCK WSAENOTSOCK
  #define EBADF WSAEINVAL
  #define ENOBUFS WSAEFAULT
  #define EMSGSIZE WSAEMSGSIZE
  #define LOG_USER
  #define LOG_INFO
  #define syslog(x, y) printf("%s\n", y)
  #define SET_ERRNO(val) (WSASetLastError(val))
  #define THREAD_FUNC unsigned __stdcall
#else
  #define delay(x) usleep(x*1000)
  #include <errno.h>
  #include <stdarg.h>
  #include <syslog.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
/*  #include <sys/select.h>*/
  #include <netinet/in.h>
  #include <arpa/inet.h>
        #include <pthread.h>
  # include <sys/ipc.h>
  typedef unsigned int SOCKET;
        typedef void * LPVOID;
        typedef unsigned long       DWORD;
  #define closesocket(x) close(x)
  #ifndef __cdecl
     #define __cdecl
  #endif
  #define ERRNO_EQUALS(val) (errno == (val))
  #define SET_ERRNO(val) (errno = (val))
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
  #define THREAD_FUNC void*
#endif
#include "error_handler.h"
#define BUFFER_SIZE (65*1024)
#define SELECT_TIMEOUT_SEC 0
#define SELECT_TIMEOUT_USEC 500000
#define FW_DELAY_MSEC 100

#if !defined(WIN32) && !defined(__WIN32__)
key_t get_key(void);
#endif

#ifndef PROXY_SOCKET_FUNCTIONS
#define PROXY_SOCKET_FUNCTIONS
#endif

#ifndef AUTH
#define AUTH
#endif

#ifndef OVER_TCP
#define OVER_TCP
#endif

extern int write_log(const char* format, ...);
extern int write_err(const char* format, ...);
extern int use_syslog;
extern char *logging_name;

#endif

