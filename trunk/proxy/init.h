//---------------------------------------------------------------------------

#ifndef initH
#define initH

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "error_handler.h"
#include "server_loop.h"
#include "protocol.h"
#include "borzoi_c.h"

extern unsigned int listen_port;
extern unsigned int console_port;
extern char *listen_address;
extern struct sockaddr_in server_addr;
extern char *program_name;
extern char *proxyname;
extern int registration_period;
extern int registration_timeout;
extern int verbose;
extern char buf[];    			// buffer for client data
extern HASHMAP *sock_data;
extern MASTER_FD master;

#define SERVER_PORT 8888
#define LISTEN_PORT 1235
#define CONSOLE_PORT 2000
#define LISTEN_ADDRESS ""
#define MAX_CLIENTS 10

#define CLIENT_CONNECTED 0
#define CLIENT_REQUEST 1
#define CLIENT_LAST_STEP 0xFF

#define CLIENT_TYPE_TIMER 1
#define CLIENT_TYPE_CLIENT 2
#define CLIENT_TYPE_SERVER 3
#define CLIENT_TYPE_UDP 4

#define PROXY_SRC_PORT_MIN 20000
#define PROXY_SRC_PORT_MAX 21000

extern int has_key_pair;
extern char public_key[];
extern char private_key[];

extern int init(int argc, char* argv[]);
extern char *hello(void);

typedef struct clnt_data {
	SOCKET sock;
  	struct sockaddr_in addr;
  	int step;
  	int client_type;
  	MASTER_FD *master;
	int authorized;
    int closed;
  	char key[SYMMETRIC_KEY_LEN];
	struct sockaddr_in proxy_ip;
  	union cd {
  		struct client {
      		char buff[64];
      		int nsize;
      		int over_tcp;
    	} client;
    	struct udp {
    		struct clnt_data *control;
			SOCKET client_socket;
    	} udp;
  	} type;
} CLIENT_DATA;



extern void *get_client_data(SOCKET sock, struct sockaddr_in addr);
extern struct sockaddr_in get_addr(void* client_data);
#endif
