#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "proxy_socket.h"
#include "protocol.h"
#include "error_handler.h"
#include "vect.h"

#if defined(WIN32) || defined(__WIN32__)
	#include <process.h>
#endif

char *username = NULL;

#include "authenticating.h"


#define TEST_BUFFER_SIZE_TIMEOUT 1
#define MAX_DATAGRAM_SIZE 32707
#define TEST_DATAGRAM_SIZE 1024

typedef struct proxy_sock {
	unsigned real_socket;
	unsigned proxy_socket;
	unsigned short proxy_port;
  	int type;
  	int protocol;
  	int bound;
    int closed;
} PROXY_SOCKET;

unsigned proxy_ping_period = 10;
FILE *debug = NULL;
int verbose = 0;

unsigned proxy_socket_functions = 1;

void set_proxy_socket_functions(unsigned proxy_socket_func){
	proxy_socket_functions  = proxy_socket_func;
}

unsigned get_proxy_socket_functions(void){
	return proxy_socket_functions;
}

#ifdef PROXY_SOCKET_FUNCTIONS

//unsigned short listen_port;
void __cdecl ping_proxy(void*, unsigned, unsigned, void*);
SOCKET connectToServer(void);
#define INITIAL_POOL_CAPACITY 1

VECTOR *pool = NULL;

char *proxy_address = NULL;
unsigned short proxy_port;
int behind_firewall = 0;
struct sockaddr_in server_address;

struct sockaddr_in proxy_sa;
unsigned proxy_over_tcp = 0;

SOCKET proxy = -1;
int started = 0;

THREAD_FUNC proxy_pinger(LPVOID data){
	struct timeval timeout;
	fd_set rset, eset;
  	int maxs;
	char buff[BUFFER_SIZE];
  	int nsize;

  	while(1){
    	sleep(proxy_ping_period);
    	vector_walk(pool, &ping_proxy, NULL);
  	}
  	return 0;
}

int __cdecl compare_proxy_socket(const void *_pd1, const void *_pd2){
	PROXY_SOCKET *pd1 = (PROXY_SOCKET*)_pd1;
	PROXY_SOCKET *pd2 = (PROXY_SOCKET*)_pd2;
  	return (pd1->real_socket < pd2->real_socket ? -1 :
  	(pd1->real_socket > pd2->real_socket ? 1 : 0)
  );
}

void __cdecl proxy_stop(void){
  	delete_vector(pool);
}

int proxy_start(void){
	if(debug == NULL){
    	debug = stdout;
  	}
	atexit(&proxy_stop);
  	pool = create_vector(INITIAL_POOL_CAPACITY, sizeof(PROXY_SOCKET), &compare_proxy_socket);
	started  = 1;
  	return 0;
}

PROXY_SOCKET find_socket(int sockfd){
  	PROXY_SOCKET ps, bf;
  	ps.real_socket = sockfd;
	if(vector_bsearch(pool, &ps, &bf, NULL, NULL, NULL, 0) == 0){
  		return bf;
  	}
  	bf.real_socket = -1;
  	return bf;
}

int __cdecl set_socket_fields(void* _old, void* _new){
	PROXY_SOCKET *o = (PROXY_SOCKET *)_old;
	PROXY_SOCKET *n = (PROXY_SOCKET *)_new;
  	o->proxy_socket = n->proxy_socket;
  	o->proxy_port = n->proxy_port;
  	o->real_socket = n->real_socket;
  	o->bound = n->bound;
  	return 0;
}

int connect_to_proxy(void){
  	int nsize;
  	char buff[BUFFER_SIZE];
	if(!behind_firewall){
		return 0;
  	}
  	if((proxy_port == 0) || (proxy_address == NULL)){
  		return 1;
  	}
  	if(proxy == -1){
  		proxy = socket(AF_INET, SOCK_STREAM, 0);
    	if (
       		proxy == INVALID_SOCKET
	    ){
      		handle_error(__FILE__, __LINE__);
	    	proxy = -1;
      		return 1;
    	}
  	}
  	sprintf(buff, "%s:\n", proxyPing);
  	if(
   		(nsize = send(proxy, &buff[0], strlen(buff), 0)) != SOCKET_ERROR
  	){

    	if(
   		    (nsize=recv(proxy,&buff[0],
                     BUFFER_SIZE-1,0)) == SOCKET_ERROR
    	){
      		handle_error(__FILE__, __LINE__);
      		closesocket(proxy);
	    	proxy = -1;
      		return 1;
    	}
    	return 0;
  	}

  	proxy_sa.sin_family=AF_INET;
  	proxy_sa.sin_port=htons(proxy_port);

  	proxy_sa.sin_addr.s_addr=inet_addr(proxy_address);

  	if (connect(proxy,(struct sockaddr *)&proxy_sa,
              sizeof(proxy_sa))){
//	    handle_error(__FILE__, __LINE__);
    	closesocket(proxy);
    	proxy = -1;
    	return 1;
  	}

  	if(
	    (nsize=recv(proxy,&buff[0],
                   BUFFER_SIZE-1,0)) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
    	closesocket(proxy);
    	proxy = -1;
    	return 1;
  	}
  	// ставим завершающий ноль в конце строки
  	buff[nsize]=0;

  	if(!authenticating(proxy, username)){
        write_log(
            "Authentication at Proxy %s:%d failed",
            inet_ntoa(proxy_sa.sin_addr), proxy_port
        );
  		closesocket(proxy);
    	return 1;
  	}

    write_log(
        "Authenticated at Proxy %s:%d",
        inet_ntoa(proxy_sa.sin_addr), proxy_port
    );


	{
#if defined(WIN32) || defined(__WIN32__)
	    unsigned proxy_pinger_pid;
  		_beginthreadex(NULL, 0, &proxy_pinger, NULL, 0, &proxy_pinger_pid);
#else
	  	pthread_t thread;
  		pthread_create(&thread, NULL, &proxy_pinger, NULL);
#endif
	}

	return 0;
}

SOCKET connectToServer(){
  	SOCKET my_sock;
  	char buff[BUFFER_SIZE];
  	struct sockaddr_in dest_addr;
  	my_sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (
   		my_sock == INVALID_SOCKET
  	){
      	handle_error(__FILE__, __LINE__);
      	return SOCKET_ERROR;
  	}
  	dest_addr.sin_family = AF_INET;
  	dest_addr.sin_port = server_address.sin_port;

    dest_addr.sin_addr = server_address.sin_addr;

  	if (connect(my_sock,(struct sockaddr *)&dest_addr,
              sizeof(dest_addr))){
	    handle_error(__FILE__, __LINE__);
#if defined(WIN32) || defined(__WIN32__)
    	return SOCKET_ERROR;
#else
    	return -1;
#endif
  	}

  	if(
	    recv(my_sock,&buff[0],
                    BUFFER_SIZE-1,0) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
      	closesocket(my_sock);
      	return SOCKET_ERROR;
  	}

  	return my_sock;

}

// returns: 0 - Success, 1 - socket error, 2 - no server, 3 - no proxy, 4 - authorization failed
int ask_for_proxy(void){
  	SOCKET my_sock = -1;
  	SOCKET test_sock;
  	struct sockaddr_in local_addr;
  	int nsize;
  	int test_port;
  	char buff[BUFFER_SIZE];
  	DWORD thID;
	struct timeval timeout;
  	int maxs;
	fd_set master, rset;

  	test_sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (
   		test_sock == INVALID_SOCKET
  	){
      	handle_error(__FILE__, __LINE__);
      	closesocket(my_sock);
      	return 1;
  	}
  	local_addr.sin_family=AF_INET;
  	local_addr.sin_addr.s_addr=INADDR_ANY;

//  for(test_port = TEST_PORT_MIN; test_port < TEST_PORT_MAX; test_port++){
	srand(time(NULL));
  	while(1){

    	test_port = TEST_PORT_MIN + rand()*(TEST_PORT_MAX - TEST_PORT_MIN)/RAND_MAX;
        local_addr.sin_port=htons(test_port);

    	if (bind(test_sock,(struct sockaddr *) &local_addr, sizeof(local_addr)) == 0){
      		if(listen(test_sock, 0x100) != 0){
        		handle_error(__FILE__, __LINE__);
        		closesocket(my_sock);
        		closesocket(test_sock);
        		return 1;
      		}
      		break;
    	}
  	}

  	if(
   		(my_sock = connectToServer()) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
    	proxy_address = NULL;
    	close(my_sock);
    	return 2;
  	}
	write_log("Asking server %s:%d for a proxy", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
  	sprintf(buff, "%s: %d\n", isBehindFirewall, test_port);

  	if(
	    (nsize = send(my_sock, &buff[0], strlen(buff), 0)) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
    	closesocket(my_sock);
    	return 2;
  	}


  	timeout.tv_sec = SELECT_TIMEOUT_SEC;
  	timeout.tv_usec = SELECT_TIMEOUT_USEC;
  	FD_ZERO(&master);
  	FD_SET(my_sock, &master);
  	FD_SET(test_sock, &master);
  	maxs = (test_sock > my_sock ? test_sock : my_sock);
  	rset = master;

  	while(
	    select(maxs + 1, &rset, NULL, NULL, &timeout) != SOCKET_ERROR
  	){
		if(FD_ISSET(my_sock, &rset)){
    		if(
        		    (nsize=recv(my_sock,&buff[0],
                        BUFFER_SIZE-1,0)) == SOCKET_ERROR
	      	){
        		handle_error(__FILE__, __LINE__);
    	    	closesocket(test_sock);
	        	closesocket(my_sock);
        		return 2;
      		}

      		if(nsize > 0){
	      		buff[nsize]=0;
        		if(is_command(buff, isBehindFirewall)){
          			char *p = tokenize_cmd(buff);
          			unsigned i = 0;
          			while((p != NULL) && (i < 3)){
            			switch(i){
              				case 0:
                				if(!strcmp(p, "YES")){
            	      				behind_firewall = 1;
        	        			}
    	            			else {
	                  				behind_firewall = 0;
                  					proxy_port = 0;
                				}
                				break;
              				case 1:
			    	      		proxy_address = (char*)strdup(p);
            	    			break;
			       			case 2:
              					if(behind_firewall){
                  					proxy_port = atoi(p);
                				}
                				break;
                	    }
            			i++;
        	    		p = tokenize_cmd(NULL);
    	      		}
	          		if(!strcmp(proxy_address, noProxyAvailable)){
                		write_log(
                        	"Server reply for Proxy from %s:%d => rejected: no proxy",
                            inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port)
                        );
            			return 3;
          			}
					proxy_sa.sin_addr.s_addr=inet_addr(proxy_address);
        	  		if(!behind_firewall){
          				// проверим не DMZ ли у нас
    	        		SOCKET test_sock_1 = socket(AF_INET, SOCK_STREAM, 0);
	            		if (
                       		test_sock_1 == INVALID_SOCKET
            			){
            	  			handle_error(__FILE__, __LINE__);
        	      			closesocket(test_sock);
    	          			closesocket(test_sock_1);
	              			closesocket(my_sock);
              				return 1;
            			}
                	    local_addr.sin_family = AF_INET;
						local_addr.sin_addr.s_addr = inet_addr(proxy_address);
						local_addr.sin_port = htons(test_port);
    	        		if(connect(test_sock_1, (struct sockaddr *)&local_addr, sizeof(local_addr)) != 0){
            			// все же за FW
	              			free(proxy_address);
              				proxy_address = NULL;
              				behind_firewall = 1;
							sprintf(buff, "%s:\n", getProxy);
            	  			if(
               	            	(nsize = send(my_sock, &buff[0], strlen(buff), 0)) == SOCKET_ERROR
	              			){
                				handle_error(__FILE__, __LINE__);
                				closesocket(test_sock);
                				closesocket(test_sock_1);
                				closesocket(my_sock);
            	    			return 2;
        	      			}

	            		}
            			closesocket(test_sock_1);
          			}
          			if(!behind_firewall){
                		write_log(
                        	"Server reply for Proxy from %s:%d => rejected: not behind firewall",
                            inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port)
                        );
          				break;
          			}
          			else {
                    	char *p1;
                    	char *p2;
                        p1 = strdup(inet_ntoa(server_address.sin_addr));
                        p2 = strdup(inet_ntoa(proxy_sa.sin_addr));
                		write_log(
                        	"Server reply for Proxy from %s:%d => assigned: %s:%d",
                            p1, ntohs(server_address.sin_port),
                            p2 , proxy_port
                        );
                        free(p1);
                        free(p2);
            			if(connect_to_proxy() != 0){
	                        p1 = strdup(inet_ntoa(server_address.sin_addr));
    	                    p2 = strdup(inet_ntoa(proxy_sa.sin_addr));
                            write_log(
                                "Connection to Proxy failed. Asking Server for another.",
                                p1, ntohs(server_address.sin_port),
                                p2, proxy_port
                            );
	                        free(p1);
    	                    free(p2);
              				proxy_address = NULL;
							sprintf(buff, "%s:\n", getProxy);
              				if(
	                            (nsize = send(my_sock, &buff[0], strlen(buff), 0)) == SOCKET_ERROR
              				){
                				handle_error(__FILE__, __LINE__);
                				closesocket(test_sock);
                				closesocket(my_sock);
                				return 1;
              				}
            			}
            			else {
							sprintf(buff, "%s:\n", proxyOK);
              				if(
                           		(nsize = send(my_sock, &buff[0], strlen(buff), 0)) == SOCKET_ERROR
              				){
                				handle_error(__FILE__, __LINE__);
                				closesocket(test_sock);
            	    			closesocket(my_sock);
        	        			if(!strcmp(proxy_address, noProxyAvailable)){
    	              				return 3;
	                			}
                				return 1;
              				}
                            write_log(
                                "Connected to Proxy %s:%d",
                                inet_ntoa(proxy_sa.sin_addr), proxy_port
                            );
          					break;
            			}
          			}
        		}

      		}

    	}
		timeout.tv_sec = SELECT_TIMEOUT_SEC;
  		timeout.tv_usec = SELECT_TIMEOUT_USEC;
    	rset = master;
	}

  	closesocket(test_sock);
	closesocket(my_sock);

	return 0;
}

void __cdecl ping_proxy(void *_ps, unsigned i, unsigned j, void* data){
	PROXY_SOCKET *ps = (PROXY_SOCKET *)_ps;
  	struct sockaddr_in proxy_listen_sa;
  	int nsize;
  	char buff[BUFFER_SIZE];
  	if((ps->real_socket == -1) || (!ps->bound)){
  		return;
  	}
  	((struct sockaddr_in*)buff)->sin_addr.s_addr = INADDR_NONE;
  	sprintf(buff + sizeof(struct sockaddr_in), "%s: %d\n", proxyBoundOK, ps->proxy_socket);
  	proxy_listen_sa.sin_family = AF_INET;
  	proxy_listen_sa.sin_port = htons(ps->proxy_port);
  	proxy_listen_sa.sin_addr.s_addr = inet_addr(proxy_address);
  	if(
	    sendto(ps->real_socket, &buff[0], strlen(buff + sizeof(struct sockaddr_in)) + sizeof(struct sockaddr_in), 0, (struct sockaddr*)&proxy_listen_sa, sizeof(struct sockaddr_in)) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
  	}
}

#endif //#ifdef PROXY_SOCKET_FUNCTIONS

int proxy_socket(int domain, int type, int protocol){
#ifndef PROXY_SOCKET_FUNCTIONS
	return socket(domain, type, protocol);
#else //#ifndef PROXY_SOCKET_FUNCTIONS
  	int i;
  	int sock;
	if(proxy_socket_functions == 0){
  		return socket(domain, type, protocol);
  	}
  	if(!started){
  		proxy_start();
  	}
    while(1){
        sock = socket(domain, type, protocol);
        if(
            sock != INVALID_SOCKET
        ){
            int replaced;
            PROXY_SOCKET ps = find_socket(sock);
            if(ps.real_socket != -1){
            	continue;
            }
            ps.real_socket = sock;
            ps.type = type;
            ps.protocol = protocol;
            ps.bound = 0;

            vector_bsearch(pool, &ps, NULL, &set_socket_fields, &ps, &replaced, 1);
            break;
        }
    }
  	return sock;

#endif //#ifndef PROXY_SOCKET_FUNCTIONS
}

int proxy_bind(int  sockfd, struct sockaddr *my_addr, int *addrlen){
#ifdef PROXY_SOCKET_FUNCTIONS
	char buff[BUFFER_SIZE];
  	int nsize;
  	unsigned short test_port;
  	struct sockaddr_in local_addr;
  	int replaced = 0;
  	int res;
  	int test_over_tcp = 0;
  	printf("FWT: Try to bind\n");
  	if(proxy_socket_functions && !started){
  		proxy_start();
  	}
  	while(proxy_socket_functions && ((proxy_address == NULL) || (connect_to_proxy() != 0))){

		if((res = ask_for_proxy()) != 0){
			write_log("Server can not get proxy");
    		return res;
    	}
  	}
  	local_addr.sin_family = AF_INET;
  	local_addr.sin_addr.s_addr = INADDR_ANY;
  	local_addr.sin_port = ((struct sockaddr_in*)my_addr)->sin_port;
  	// Try to fix Leksi BUG
  	for(test_port = SRC_PORT_MIN; test_port < SRC_PORT_MAX; test_port++){
    	if (proxy_socket_functions) {
      		local_addr.sin_port = htons(test_port);
    	}

    	if ((bind(sockfd,(struct sockaddr *)&local_addr,
      		sizeof(struct sockaddr_in))) == 0){
      		break;
    	}
    	if(
        	!ERRNO_EQUALS(EADDRINUSE)
		){
			write_log("Internal bind error=%s", error_id());
      		return 1;
    	}
  	}
  	if(test_port == SRC_PORT_MAX){
       	SET_ERRNO(EADDRINUSE);
    	handle_error(__FILE__, __LINE__);
		write_log("test port too big?");
    	return SOCKET_ERROR;
  	}
  	((struct sockaddr_in*)my_addr)->sin_family = local_addr.sin_family;
  	((struct sockaddr_in*)my_addr)->sin_port = local_addr.sin_port;
  	if(proxy_socket_functions){
		((struct sockaddr_in*)my_addr)->sin_addr.s_addr = inet_addr(proxy_address);
  	}
  	else {
		((struct sockaddr_in*)my_addr)->sin_addr = local_addr.sin_addr;
  	}
  	*addrlen = sizeof(struct sockaddr_in);
  	if(!proxy_socket_functions){
		return getsockname(sockfd, my_addr, addrlen);
  	}

	if(behind_firewall){
  		PROXY_SOCKET ps = find_socket(sockfd);
    	if(ps.real_socket == -1){
        	SET_ERRNO(ENOTSOCK);
      		handle_error(__FILE__, __LINE__);
      		return 1;
    	}

    	sprintf(buff, "%s: %d; %d; %d; %s\n", proxyBind, ps.real_socket, ps.type, ps.protocol, proxy_address);
    	if(
       		(nsize = send(proxy, &buff[0], strlen(buff), 0)) == SOCKET_ERROR
    	){
      		handle_error(__FILE__, __LINE__);
      		return 1;
    	}
		while(1){
      		if(
           		(nsize = recv(proxy, &buff[0], BUFFER_SIZE - 1, 0)) == SOCKET_ERROR
      		){
        		handle_error(__FILE__, __LINE__);
        		return 1;
      		}
      		if(nsize > 0){
        		buff[nsize]=0;
        		if(is_command(buff, proxyAuth1)){
                    write_log(
                        "Authentication at Proxy %s:%d failed",
                        inet_ntoa(proxy_sa.sin_addr), proxy_port
                    );
        			return 4;
        		}
        		else if(is_command(buff, proxyBound)){
          			struct sockaddr_in proxy_listen_sa;
          			int i = 0;
          			char *p = tokenize_cmd(buff);
          			while((p != NULL) && (i < 3)){
            			switch(i){
              				case 0:
			          			ps.proxy_socket = atoi(p);
                				break;
              				case 1:
			          			ps.proxy_port = atoi(p);
                				break;
              				case 2:
			          			test_over_tcp = (strcmp(p, "OVER_TCP=Y") == 0);
                				break;

            			}
            			i++;
            			p = tokenize_cmd(NULL);
          			}

          			ps.bound = 1;
          			ping_proxy(&ps, 0, 0, NULL);
          			ps.bound = 0;

					break;
        		}
      		}
		}
    	l1:
    	ps.bound = 1;

    	vector_bsearch(pool, &ps, NULL, &set_socket_fields, &ps, &replaced, 0);
    	((struct sockaddr_in*)my_addr)->sin_port = htons(ps.proxy_port);

    	// Проверяем, выпускает ли FW UDP
    	if(test_over_tcp){
    		struct timeval timeout;
      		int maxs;
      		fd_set rset, eset;
      		long last;
      		int ntry = 0;
      		int len = TEST_DATAGRAM_SIZE;

			timeout.tv_sec = SELECT_TIMEOUT_SEC;
			timeout.tv_usec = SELECT_TIMEOUT_USEC;

      		while(1){
        		l2:
           		write_log("Try UDP send through FW");
        		FD_ZERO(&rset);
        		FD_SET(ps.real_socket, &rset);
        		maxs = ps.real_socket + 1;

        		((struct sockaddr_in*)buff)->sin_addr.s_addr = INADDR_NONE;
        		sprintf(buff + sizeof(struct sockaddr_in), "%s:\n", testOverTCP);
        		if(
               		(nsize = sendto(ps.real_socket, buff, len , 0, my_addr, sizeof(struct sockaddr_in))) == SOCKET_ERROR
        		){
          			handle_error(__FILE__, __LINE__);
          			return 1;
        		}

        		last = time(NULL);

        		while(
	                select(maxs, &rset, NULL, NULL, &timeout) != SOCKET_ERROR
        		){
          			if(FD_ISSET(ps.real_socket, &rset)){
          				struct sockaddr_in tmp_sa;
            			int tmp_sa_len = sizeof(struct sockaddr_in);
            			if(
       		                (nsize = recvfrom(ps.real_socket, &buff[0], BUFFER_SIZE - 1, 0, (struct sockaddr*)&tmp_sa, &tmp_sa_len)) == SOCKET_ERROR
            			){
              				handle_error(__FILE__, __LINE__);
              				return 1;
            			}
            			if(nsize > 0){
	            			proxy_over_tcp = 0;
              				goto l3;
            			}
          			}

					write_log("timeout=%d", ntry);
          			if(time(NULL) - last > OVER_TCP_TEST_TIMEOUT){
            			proxy_over_tcp = 1;
            			if(++ntry < OVER_TCP_TEST_TRIES){
            				goto l2;
            			}
	            		goto l3;
    	    		}
					timeout.tv_sec = SELECT_TIMEOUT_SEC;
					timeout.tv_usec = SELECT_TIMEOUT_USEC;
          			FD_ZERO(&rset);
          			FD_SET(ps.real_socket, &rset);
          			maxs = ps.real_socket + 1;
				}
      		}
      		l3:;
    	}

		set_proxy_over_tcp(proxy_over_tcp);
        {
            char *p1;
            char *p2;
            p1 = strdup(inet_ntoa(proxy_sa.sin_addr));
            p2 = strdup(inet_ntoa(proxy_sa.sin_addr));
            write_log(
                "Bound with Proxy %s:%d as %s:%d (%s)",
                p1, proxy_port, p2, ps.proxy_port,
                proxy_over_tcp ? "TCP" : "UDP"
            );
            free(p1);
            free(p2);
        }
  	}
	return 0;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS



#ifndef PROXY_SOCKET_FUNCTIONS

	char buff[BUFFER_SIZE];
  	int nsize;
  	unsigned short test_port;
  	struct sockaddr_in local_addr;
  	int replaced = 0;
  	int res;
  	int test_over_tcp = 0;
  	printf("FWT: Try to bind\n");
  	local_addr.sin_family = AF_INET;
  	local_addr.sin_addr.s_addr = INADDR_ANY;
  	for(test_port = SRC_PORT_MIN; test_port < SRC_PORT_MAX; test_port++){

    	local_addr.sin_port = htons(test_port);

    	if (bind(sockfd,(struct sockaddr *)&local_addr,
      		sizeof(struct sockaddr_in)) == 0){
      		break;
    	}
    	if(
        	!ERRNO_EQUALS(EADDRINUSE)
		){
      		handle_error(__FILE__, __LINE__);
			write_log("Internal bind error=%s", error_id());

      		return 1;
    	}
  	}
  	if(test_port == SRC_PORT_MAX){
    	SET_ERRNO(EADDRINUSE);
    	handle_error(__FILE__, __LINE__);
		write_log("test port too big?");
    	return 1;
  	}
  	((struct sockaddr_in*)my_addr)->sin_family = local_addr.sin_family;
  	((struct sockaddr_in*)my_addr)->sin_port = local_addr.sin_port;
	((struct sockaddr_in*)my_addr)->sin_addr = local_addr.sin_addr;
	return 0;

#endif //#ifndef PROXY_SOCKET_FUNCTIONS
}

int proxy_close(int d){
#ifdef PROXY_SOCKET_FUNCTIONS
	int replaced;

  	if(proxy_socket_functions == 0){
		return closesocket(d);
  	}

  	if(!started){
  		proxy_start();
  	}
	if(behind_firewall){
		char buff[BUFFER_SIZE];
  		PROXY_SOCKET ps = find_socket(d);
    	if(ps.real_socket == -1){
	       	SET_ERRNO(ENOTSOCK);
	      	handle_error(__FILE__, __LINE__);
    	  	return 1;
    	}

    	sprintf(buff, "%s: %d\n", proxyClose, ps.proxy_socket);
    	if(
       		send(proxy, &buff[0], strlen(buff), 0) == SOCKET_ERROR
    	){
        	if(ERRNO_EQUALS(ENOTSOCK)){
            	if(ps.bound){
	                write_log(
    		            "Proxy %s:%d dead",
            	        inet_ntoa(proxy_sa.sin_addr), proxy_port
                	);
                }
            }
            else {
      			handle_error(__FILE__, __LINE__);
            }
    	}
    	ps.bound = 0;
        ps.closed = 1;
    	vector_bsearch(pool, &ps, NULL, &set_socket_fields, &ps, &replaced, 0);

		write_log("Unregistered at Proxy %s:%d",
          	        inet_ntoa(proxy_sa.sin_addr), proxy_port);
  	}
 	return closesocket(d);
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
#ifndef PROXY_SOCKET_FUNCTIONS
 	return closesocket(d);
#endif //#ifndef PROXY_SOCKET_FUNCTIONS
}

int proxy_sendto(int s, const char *msg, size_t len, int flags, const struct sockaddr *to, int tolen){
#ifndef PROXY_SOCKET_FUNCTIONS
	return sendto(s, msg, len, flags, to, tolen);
#endif //#ifndef PROXY_SOCKET_FUNCTIONS


#ifdef PROXY_SOCKET_FUNCTIONS
	char buff[BUFFER_SIZE];
  	int nsize;
	PROXY_SOCKET ps;
  	struct sockaddr_in dst_sa = proxy_sa;

  	if(proxy_socket_functions == 0){
		return sendto(s, msg, len, flags, to, tolen);
  	}
  	if(!started){
  		proxy_start();
  	}
  	if(len == 0){
  		return len;
  	}
  	if(len > MAX_DATAGRAM_SIZE){
    	SET_ERRNO(EMSGSIZE);

  		return SOCKET_ERROR;
  	}
	if(!behind_firewall){
  		return sendto(s, msg, len, flags, to, tolen);
  	}
	ps = find_socket(s);
  	if(ps.real_socket != -1){
  		if(proxy_over_tcp){
      		int sent, rest;
      		int data_start;
      		sprintf(buff, "%s: %d; %d\n", overTCP, ps.proxy_socket, len + sizeof(struct sockaddr_in));
			data_start = strlen(buff);
	      	memcpy(buff + data_start, to,  sizeof(struct sockaddr_in));
    	  	memcpy(buff + data_start + sizeof(struct sockaddr_in), (char*)msg, len);
	      	sent = 0;
    	  	rest = data_start + len + sizeof(struct sockaddr_in);
	      	while(rest > 0){
       			if(
	    	      		(
    			      		nsize = send(proxy, buff + sent, rest , 0)
		        		) == SOCKET_ERROR
    	    	){
		        	handle_error(__FILE__, __LINE__);
	    	    	return SOCKET_ERROR;
	        	}
        		sent += nsize;
        		rest -= nsize;
    	  	}
	    }
	    else {
	    	if(proxy == -1){
	    	    return SOCKET_ERROR;
	      	}
    		memcpy(buff, to,  sizeof(struct sockaddr_in));
	  		dst_sa.sin_port = htons(ps.proxy_port);

    		memcpy(buff + sizeof(struct sockaddr_in), (char*)msg, len);
    	  	if(
                	(
        				nsize = sendto(ps.real_socket, buff,
							len + sizeof(struct sockaddr_in)
	        				, 0,
    		      		(struct sockaddr*)&dst_sa, sizeof(struct sockaddr_in))
	    	  		) == SOCKET_ERROR
	      	){
        		handle_error(__FILE__, __LINE__);
        		return 1;
    	  	}
	    }
    	return len;
  	}
  	else {
  		return SOCKET_ERROR;
  	}
  	return 0;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
}

int proxy_recvfrom(int  s,  char  *buf,  size_t len, int flags, struct sockaddr *from, int *fromlen){
#ifndef PROXY_SOCKET_FUNCTIONS
	return recvfrom(s,  buf,  len, flags, from, fromlen);
#endif //#ifndef PROXY_SOCKET_FUNCTIONS


#ifdef PROXY_SOCKET_FUNCTIONS
  	int nsize;
  	char buff[BUFFER_SIZE];

  	if(proxy_socket_functions == 0){
		return recvfrom(s,  buf,  len, flags, from, fromlen);
  	}
  	if(!started){
  		proxy_start();
  	}
  	if(!behind_firewall){
  		return recvfrom(s, buf, len, flags, from, fromlen);
  	}
  	if(proxy_over_tcp){
    	int ln;
    	int data_start;
    	int pos;

    	l1:
    	if(
       		(nsize = recv(proxy, buf, len, MSG_PEEK)) == SOCKET_ERROR
    	){
      		return nsize;
    	}
    	if(is_command(buf, overTCP)){
      		int i = 0;
      		char *p;
      		if((p = (char*)strchr(buf, '\n')) == NULL){
        		return 0;
      		}
      		*p++ = 0;
      		data_start = p - (char*)buf;
      		p = tokenize_cmd(buf);
      		while((p != NULL) && (i < 2)){
        		switch(i){
          			case 0:
            			break;
          			case 1:
			      		ln = atoi(p);
            			break;
        		}
            	i++;
        		p = tokenize_cmd(NULL);
      		}
  			recv(proxy, buf, nsize, 0);
      		pos = nsize - data_start;
      		memmove(buf, (char*)buf + data_start, pos);
      		while(pos < ln){
        		if(
               		(nsize = recv(proxy, (char*)buf + pos, ln - pos, 0)) == SOCKET_ERROR
        		){
          			return nsize;
        		}
        		pos += nsize;
      		}
      		nsize = ln;
    	}
    	else {
    		if(is_command(buf, proxyPing) || is_command(buf, proxyBoundOK)){
    			recv(proxy, buf, len, 0);
                if(is_command(buf, proxyBoundOK) && verbose){
                	write_log(
                    	"Heartbeat successfull to Proxy %s:%d",
	          	        inet_ntoa(proxy_sa.sin_addr), proxy_port
                    );
                }
      		}
      		else {
      		}
  			goto l1;
    	}
  	}
  	else {
    	if(proxy == -1){
      		return SOCKET_ERROR;
    	}
	  	nsize = recvfrom(s, buf, len, flags, from, fromlen);
  	}
	if(
   		nsize == SOCKET_ERROR
  	){
  		return nsize;
  	}
    memcpy((struct sockaddr_in*)from, (char*)buf, sizeof(struct sockaddr_in));
  	nsize -= sizeof(struct sockaddr_in);
  	memmove(buf, (char*)buf + sizeof(struct sockaddr_in), nsize);
  	return nsize;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
}

int proxy_getsockname(int s, struct sockaddr *name, int *namelen){
#ifndef PROXY_SOCKET_FUNCTIONS
	return getsockname(s,  name,  namelen);
#endif //#ifndef PROXY_SOCKET_FUNCTIONS


#ifdef PROXY_SOCKET_FUNCTIONS
 	PROXY_SOCKET ps;
  	if(proxy_socket_functions == 0 || behind_firewall == 0){
		return getsockname(s,  name,  namelen);
  	}

  	if(*namelen < sizeof(struct sockaddr_in)){
    	SET_ERRNO(ENOBUFS);
	    return SOCKET_ERROR;
  	}
 	ps = find_socket(s);
  	if(ps.real_socket == -1){
    	SET_ERRNO(ENOTSOCK);
	    return SOCKET_ERROR;
	}
  	if(ps.bound == 0){
    	SET_ERRNO(EBADF);
    	return SOCKET_ERROR;
  	}
  	((struct sockaddr_in*)name)->sin_family = AF_INET;
  	((struct sockaddr_in*)name)->sin_addr.s_addr = proxy_sa.sin_addr.s_addr;
  	((struct sockaddr_in*)name)->sin_port = ps.proxy_port;
  	*namelen = sizeof(struct sockaddr_in);
	return 0;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
}

int  proxy_select(int  n,  fd_set  *readfds,  fd_set  *writefds,
       fd_set *exceptfds, struct timeval *timeout){

#ifndef PROXY_SOCKET_FUNCTIONS
	return select(n,  readfds,  writefds,
       exceptfds, timeout);
#endif //#ifndef PROXY_SOCKET_FUNCTIONS

#ifdef PROXY_SOCKET_FUNCTIONS
  	int nsize;
  	char buff[BUFFER_SIZE];
  	int res, res1;
  	fd_set rfd;
  	int i;

  	if(proxy_socket_functions == 0){
    	return select(n,  readfds,  writefds, exceptfds, timeout);
  	}
  	if(!started){
  		proxy_start();
  	}
	if(!behind_firewall){
  		return select(n, readfds, writefds, exceptfds, timeout);
  	}
  	if(readfds != NULL){
  		FD_SET(proxy, readfds);
  	}
  	if(n <= (int)proxy){
  		n = proxy + 1;
  	}
  	rfd = *readfds;

  	res1 = res = select(n, readfds, writefds, exceptfds, timeout);

	if(res > 0){

    	for(i = 0; i < n; i++){
      		if((i != (int)proxy) && FD_ISSET(i, readfds) && proxy_over_tcp){
        		res1--;
        		FD_CLR((unsigned)i, readfds);
      		}
        }
    	if(FD_ISSET(proxy, readfds)){
      		res1--;
      		nsize = recv(proxy, buff, BUFFER_SIZE - 1, MSG_PEEK);
      		if((nsize == 0) || (
           		nsize == SOCKET_ERROR
      		)){
    	    	nsize = recv(proxy, buff, BUFFER_SIZE - 1, 0);
	        	proxy = -1;
    	    	return SOCKET_ERROR;
    	  	}
	      	else if(is_command(buff, overTCP)){
    	    	SOCKET sock;
	        	int i = 0;
        		char *p;
        		if((p = (char*)strchr(buff, '\n')) == NULL){
    	      		return 0;
	        	}
        		*p++ = 0;
        		p = tokenize_cmd(buff);
    	    	while((p != NULL) && (i < 2)){
	          		switch(i){
            			case 0:
        	      			sock = atoi(p);
    	          			break;
	            		case 1:
              				break;
          			}
          			i++;
        	  		p = tokenize_cmd(NULL);
    	    	}
	        	if((readfds != NULL) && FD_ISSET(sock, &rfd) && !FD_ISSET(sock, readfds)){
          			FD_SET(sock, readfds);
        		}
      		}
    	  	else {
	        	if(is_command(buff, proxyPing) || is_command(buff, proxyBoundOK)){
          			nsize = recv(proxy, buff, BUFFER_SIZE - 1, 0);
        	  		FD_CLR(proxy, readfds);
                    if(is_command(buff, proxyBoundOK) && verbose){
                        write_log(
                            "Heartbeat successfull to Proxy %s:%d",
                            inet_ntoa(proxy_sa.sin_addr), proxy_port
                        );
                    }
    	    	}
	        	else {
        		}
      		}
    	}
  	}
  	if((res != 0) && (res1 == 0)){
    	SET_ERRNO(EINTR);
        return SOCKET_ERROR;
  	}
  	return res1;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
}

void set_proxy_over_tcp(unsigned over_tcp){
#ifdef PROXY_SOCKET_FUNCTIONS
	char buff[BUFFER_SIZE];
  	proxy_over_tcp = over_tcp;
  	sprintf(buff, "%s: %s\n", isOverTCP, (over_tcp ? "Yes" : "No"));
  	if(
		send(proxy, &buff[0], strlen(buff), 0) == SOCKET_ERROR
  	){
    	handle_error(__FILE__, __LINE__);
    	return;
  	}

#endif //#ifdef PROXY_SOCKET_FUNCTIONS
}

unsigned get_proxy_over_tcp(void){
#ifdef PROXY_SOCKET_FUNCTIONS
	return proxy_over_tcp;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
#ifndef PROXY_SOCKET_FUNCTIONS
	return 0;
#endif //#ifndef PROXY_SOCKET_FUNCTIONS
}


struct sockaddr_in get_server_address(void){
#ifdef PROXY_SOCKET_FUNCTIONS
	return server_address;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS
#ifndef PROXY_SOCKET_FUNCTIONS
	struct sockaddr_in sa;
  	sa.sin_family = AF_INET;
  	sa.sin_port = htons(0);
  	sa.sin_addr.s_addr = INADDR_NONE;
  	return sa;
#endif //#ifndef PROXY_SOCKET_FUNCTIONS
}

void set_server_address(struct sockaddr_in sa){
#ifdef PROXY_SOCKET_FUNCTIONS
	server_address = sa;
  	proxy = -1;
  	proxy_address = NULL;
#endif //#ifdef PROXY_SOCKET_FUNCTIONS

}

void set_username(char* anusername){
	username = anusername;
}




