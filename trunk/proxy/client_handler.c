#include "client_handler.h"

unsigned nclients = 0;

int timer_disabled = 0;
long last_registration = 0;

int console_sock = 0;

int server_read(CLIENT_DATA *cd);
int client_write(CLIENT_DATA *cd);
int client_exception(CLIENT_DATA *cd);
int client_read(CLIENT_DATA *cd);
int client_connected(CLIENT_DATA *cd);
int client_request(CLIENT_DATA *cd);
int register_on_server(void);
int proxy_ping_reply(CLIENT_DATA *cd);
int proxy_bind_reply(CLIENT_DATA *cd);
int proxy_close_reply(CLIENT_DATA *cd);
int proxy_bound_ok_replay(CLIENT_DATA *cd, int nsize, struct sockaddr_in sa);
int receive_packet(CLIENT_DATA *cd, int nsize, struct sockaddr_in sa);

#define BUFFSIZE 1024

THREAD_FUNC console_session(void* arg){
    int nsize;
  	char buff[BUFFSIZE];
    struct sockaddr_in client;
  	int client_addr_sz = sizeof(struct sockaddr_in);

    int sock = 0;
    if((sock = accept(console_sock, (struct sockaddr*)&client, &client_addr_sz)) != INVALID_SOCKET){
        int n = 0;
        int ptr = 0;
        char *rpos = 0;
        write_log("Console client connected");
	    sprintf(buff, "Peerio Proxy Console\n\r> ");
    	send(sock, buff, strlen(buff), 0);
        while(n = recv(sock, buff + ptr, BUFFSIZE - 1 - ptr, 0) > 0){
        	buff[ptr + n] = 0;
    		send(sock, buff + ptr, strlen(buff + ptr), 0);
            rpos = (char*)strchr(buff + ptr, '\r');
            if(rpos != NULL){
            	*rpos = 0;
                ptr = 0;
		   		send(sock, "\n\r", 2, 0);
	            if(strncmp(buff, "quit", 1) == 0){
				    sprintf(buff, "Bye\n\r");
			   		send(sock, buff, strlen(buff), 0);
    	        	break;
        	    }
	            if(strncmp(buff, "stop", 1) == 0){
				    sprintf(buff, "Stopping\n\r");
			   		send(sock, buff, strlen(buff), 0);
    	        	exit(-1);
        	    }
                if(console_cmd(buff) != 0){
				    sprintf(buff, "- unrecognized command");
                }
                strcpy(buff + strlen(buff), "\n\r> ");
		   		send(sock, buff, strlen(buff), 0);
            }
            else {
	        	ptr += n;
            }
        }
	    closesocket(sock);
        write_log("Console client disconnected");
    }
    else {
   		handle_error(__FILE__, __LINE__);
    }
  	return NULL;
}

int console_cmd(char* buffer){
	if(strncmp(buffer, "clients", 1) == 0){
        hashmap_snprint(buffer, BUFFSIZE, sock_data, NULL);
    	return 0;
    }
	if(strncmp(buffer, "master", 1) == 0){
        snprint_master(buffer, BUFFSIZE, &master);
    	return 0;
    }
	if(strncmp(buffer, "?", 1) == 0){
    	snprintf(buffer, BUFFSIZE, "q(uit)\t- close session\n\r");
    	snprintf(buffer + strlen(buffer), BUFFSIZE - strlen(buffer), "s(top)\t- stopping proxy\n\r");
    	snprintf(buffer + strlen(buffer), BUFFSIZE - strlen(buffer), "c(lients)\t- list clients' sockets\n\r");
    	snprintf(buffer + strlen(buffer), BUFFSIZE - strlen(buffer), "m(aster)\t- list select'ed sockets\n\r");
    	return 0;
    }
	return 1;
}

int client_handler(void *data, MASTER_FD* master, int what){
    CLIENT_DATA *cd = (CLIENT_DATA *)data;
    if(console_sock == 0){
    	struct sockaddr_in lsa;
        int yes = 1;
        lsa.sin_family = AF_INET;
        lsa.sin_addr.s_addr = INADDR_ANY;
        lsa.sin_port = htons(console_port);
    	if(
        	((console_sock = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET)
  			&& (setsockopt(console_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == 0)
            && (bind(console_sock, (struct sockaddr*)&lsa, sizeof(struct sockaddr_in)) != SOCKET_ERROR)
            && (listen(console_sock, 0x100) != SOCKET_ERROR)
        ){
	    	void *data_item = get_client_data(console_sock, lsa);
		  	add_socket(console_sock, sock_data, data_item, master, MASTER_READ);
	        write_log("Console started");
       	}
       	else {
       		console_sock = 0;
       	}
    }
    if(cd->sock == console_sock){
        {
#if defined(WIN32) || defined(__WIN32__)
            unsigned thid;
            _beginthreadex(NULL, 0, &console_session, NULL, 0, &thid);
#else
            pthread_t thread;
            pthread_create(&thread, NULL, &console_session, NULL);
#endif
        }
        return  0;
    }
    if(cd->step == CLIENT_LAST_STEP){
	    return disconnect(cd);
  	}
  	cd->master = master;
  	switch(what){
        case WHAT_READ:
    	    return client_read(cd);
        case WHAT_WRITE:
        	return client_write(cd);
        case WHAT_EXCEPTION:
        	return client_exception(cd);
  	}
  	return MASTER_ALL;

}

int master_walker(void* data, void *data1){
    CLIENT_DATA *cd = (CLIENT_DATA *)data;
    CLIENT_DATA *cd1 = (CLIENT_DATA *)data1;
  	if(
        (cd->client_type == CLIENT_TYPE_CLIENT) && (cd1->type.udp.control == cd)
        || (cd->client_type == CLIENT_TYPE_UDP) && (cd->type.udp.control == cd1)
  	){
        return 1;
  	}
  	return 0;
}

int disconnect(CLIENT_DATA *cd){
  	if(nclients > 0){
        nclients--;
  	}
  	last_registration = 0;
  	walk_master(sock_data, cd, &master, &master_walker);
  	return DISCONNECT;
}

int client_write(CLIENT_DATA *cd){
    int nsize;
    if(cd->type.client.nsize > 0){
    	if(send(cd->sock, cd->type.client.buff, cd->type.client.nsize, 0) < 0){
      		if(verbose){
        		handle_error(__FILE__, __LINE__);
      		}
    	}
    	cd->type.client.nsize = 0;
        return MASTER_WRITE;
  	}
    return 0;
}

THREAD_FUNC server_registration(void* arg){
    int nsize;
  	char buff[1024];
    CLIENT_DATA *cd = (CLIENT_DATA *)arg;
  	int client_addr_sz = sizeof(struct sockaddr_in);
  	if(connect(cd->sock, (struct sockaddr *)&cd->addr, client_addr_sz) != 0){
   	 	if(verbose){
    	    handle_error(__FILE__, __LINE__);
    	}
  	}
  	else {
        add_socket(cd->sock, sock_data, cd, &master, MASTER_READ);
  	}
  	return 0;
}

CLIENT_DATA *server;

int register_on_server(void){
  	SOCKET server_sock;

  	server_sock = socket(AF_INET, SOCK_STREAM, 0);
  	if(
   		server_sock == INVALID_SOCKET
  	){
        return 1;
  	}
  	server = (CLIENT_DATA *)get_client_data(server_sock, server_addr);
  	server->client_type = CLIENT_TYPE_SERVER;
  	server->step = CLIENT_REQUEST;
    {
#if defined(WIN32) || defined(__WIN32__)
    	unsigned thid;
        _beginthreadex(NULL, 0, &server_registration, server, 0, &thid);
#else
        pthread_t thread;
        pthread_create(&thread, NULL, &server_registration, server);
#endif
  	}

   	return 0;
}


int client_exception(CLIENT_DATA *cd){
    return 0;
}

int client_read(CLIENT_DATA *cd){
  	if(cd->step == CLIENT_CONNECTED){
    	return client_connected(cd);
  	}
  	else if(cd->step == CLIENT_REQUEST){
    	return client_request(cd);
  	}
  	else {
  	}
  	return 0;
}

int server_read(CLIENT_DATA *cd){
  	int nsize;
    if(strcmp(buf, sHello) == 0){
        sprintf(buf, "%s: %s; %d; %d\n", registerProxy, listen_address, listen_port, nclients);
printf("[%s, %d] %s\n", __FILE__, __LINE__, buf);
    	if(send(cd->sock, buf, strlen(buf), 0) < 0){
        	if(verbose){
        		handle_error(__FILE__, __LINE__);
      		}
        }
        return 0;
  	}

  	else if(is_command(buf, registeredProxy)){
    	int i = 0;
	    char *p = tokenize_cmd(buf);
    	while((p != NULL) && (i < 1)){
      		switch(i){
        		case 0:
                     registration_period = atoi(p);
                     break;
            }
      		i++;
      		p = tokenize_cmd(NULL);
        }
    	timer_disabled = 0;
        write_log("Registered sucessfully on server %s:%d", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
        return MASTER_ALL;
  	}
  	return 0;
}

int client_connected(CLIENT_DATA *cd){
  	sprintf(buf, "%s", pHello);
  	if(send(cd->sock, buf, strlen(buf), 0) < 0){
    	if(verbose){
        	handle_error(__FILE__, __LINE__);
    	}
    	return MASTER_ALL;
  	}
  	nclients++;
  	last_registration = 0;

  	cd->step = CLIENT_REQUEST;
  	return 0;
}

int rerecv_cmd(CLIENT_DATA *cd, char *buf, int nsize){
    char *p = strchr(buf, '\r');
  	if(p != NULL){
        nsize = p - buf + 1;
  	}
    nsize = recv(cd->sock, buf, nsize, 0);
  	return nsize;
}

int client_request(CLIENT_DATA *cd){
    int nsize;
  	if(cd->client_type != CLIENT_TYPE_UDP){
    	if((nsize = recv(cd->sock, buf, BUFFER_SIZE - 1, MSG_PEEK)) < 0){
            if(ERRNO_EQUALS(ECONNRESET)){
            	if(!cd->closed){
            		return proxy_close_reply(cd);
                }
            }
            else {
	        	if(verbose){
    	            handle_error(__FILE__, __LINE__);
      			}

            }
    	}
    	if(nsize <= 0){
        	recv(cd->sock, buf, BUFFER_SIZE - 1, 0);
      		return disconnect(cd);
    	}
        nsize = rerecv_cmd(cd, buf, nsize);
    	buf[nsize] = 0;
    	if(cd->client_type == CLIENT_TYPE_SERVER){
      		return server_read(cd);
    	}
    	else if(cd->client_type == CLIENT_TYPE_CLIENT){
      	 	if(is_command(buf, proxyPing)){
        		return proxy_ping_reply(cd);
      		}
      		else if(is_command(buf, proxyBind)){
                if(!cd->authorized){
        			write_log("Client %s:%d authentication failed (wrong face)", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
                	return unauthorized(cd, proxyAuth1);
                }
        		return proxy_bind_reply(cd);
      		}
      		else if(is_command(buf, proxyClose)){
        		return proxy_close_reply(cd);
      		}
      		else if(is_command(buf, isOverTCP)){
        		return is_over_tcp_reply(cd);
      		}
      		else if(is_command(buf, overTCP)){
                if(!cd->authorized){
        			write_log("Client %s:%d authentication failed (wrong face)", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
                	return unauthorized(cd, proxyAuth1);
                }
        		return send_over_tcp(cd, nsize);
      		}
      		else if(is_command(buf, proxyAuth1)){
            	int res = auth_1(cd, nsize);
                if(res == DISCONNECT){
	      			write_log("Client %s:%d authentication failed (wrong face)", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
                }
                return res;
      		}
      		else if(is_command(buf, proxyAuth2)){
            	int res = auth_2(cd, nsize);
                if(!cd->authorized){
	      			write_log("Client %s:%d authentication failed (wrong face)", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
                }
                return res;
      		}
    	}
  	}
  	else {
        struct sockaddr_in sa;
#if defined(WIN32) || defined(__WIN32__)
	    int sz;
#else
    	unsigned sz;
#endif
    	sz = sizeof(struct sockaddr_in);
    	if((nsize = recvfrom(cd->sock, buf, BUFFER_SIZE - 1, 0, (struct sockaddr*)&sa, &sz)) == SOCKET_ERROR){
            	if(verbose){
            		handle_error(__FILE__, __LINE__);
      			}
    	}
    	if(nsize <= 0){
            return disconnect(cd);
    	}
    	buf[nsize] = 0;
    	if(
      		(sa.sin_addr.s_addr == cd->addr.sin_addr.s_addr)
      		&& is_command(buf + sizeof(struct sockaddr_in), testOverTCP)
      		&& (((struct sockaddr_in*)buf)->sin_addr.s_addr == INADDR_NONE)

    	){
      		return test_over_tcp_replay(cd, nsize, sa);
    	}
    	else if(
      		(sa.sin_addr.s_addr == cd->addr.sin_addr.s_addr)
      		&& is_command(buf + sizeof(struct sockaddr_in), proxyBoundOK)
      		&& (((struct sockaddr_in*)buf)->sin_addr.s_addr == INADDR_NONE)
        ){
        	return proxy_bound_ok_replay(cd, nsize, sa);
    	}
    	else if(
        	(sa.sin_addr.s_addr == cd->addr.sin_addr.s_addr)
        	&& (sa.sin_port == cd->addr.sin_port)
	    ){ // from our client
            if(!cd->type.udp.control->authorized){
      			write_log("Client %s:%d authentication failed (wrong face)", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
                return unauthorized(cd, proxyAuth1);
        	}
        	return resend_packet(cd, buf, nsize);
    	}
    	else { // to our client
        	return receive_packet(cd, nsize, sa);
    	}
  	}
  	return 0;

}

int timer_event(void* arg){
    if(!timer_disabled){
    	if((now - last_registration) > registration_period){
        	timer_disabled = 1;
          	last_registration = now;
            register_on_server();
    	}
  	}
  	else {
        if((now - last_registration) > registration_timeout){
            timer_disabled = 0;
            closesocket(server->sock);
      		rm_socket(server->sock, sock_data, &master, MASTER_ALL);
    	}
  	}
    return 0;
}

int proxy_ping_reply(CLIENT_DATA *cd){
    int nsize;

//  delay(FW_DELAY_MSEC);   /* delay 10 ms */
  	if(send(cd->sock, buf, strlen(buf), 0)
      	!= SOCKET_ERROR
  	){
  	}
  	return 0;
}

THREAD_FUNC binding(void* arg){
    int nsize;
  	char buff[1024];
    CLIENT_DATA *cd = (CLIENT_DATA *)arg;
  	CLIENT_DATA *control = cd->type.udp.control;
  	struct sockaddr_in my_addr;
  	unsigned short proxy_port;
  	my_addr.sin_family = AF_INET;
  	my_addr.sin_addr.s_addr = INADDR_ANY;
  	for(proxy_port = PROXY_SRC_PORT_MIN; proxy_port < PROXY_SRC_PORT_MAX; proxy_port++){
        int j;
        for(j = 0; j < 1024; j++){
            HASHMAP_ENTRY *ptr = sock_data->list[j];
            while(ptr != NULL){
                if(ptr->value != NULL){
                    if(((CLIENT_DATA*)ptr->value)->proxy_ip.sin_port == htons(proxy_port)){
                        goto l1;
                    }
                }
                ptr = ptr->next;
            }
        }
    	my_addr.sin_port = htons(proxy_port);
    	if (
        	bind(cd->sock,(struct sockaddr *)&my_addr,
      		sizeof(struct sockaddr_in)) != SOCKET_ERROR
        ){
      		break;
    	}
    	if(!ERRNO_EQUALS(EADDRINUSE)){
     	 	sprintf(buff, "%s: %s; %d\n", proxyBindError, error_id(), __LINE__);
        	closesocket(cd->sock);
	        free(cd);
    	  	goto end;
    	}
    	l1:;
  	}
  	if(proxy_port == PROXY_SRC_PORT_MAX){
      	SET_ERRNO(EADDRINUSE);
		sprintf(buff, "%s: %s; %d\n", proxyBindError, error_id(), __LINE__);
    	closesocket(cd->sock);
    	free(cd);
  	}
  	else {
    	sprintf(buff, "%s: %d; %d; %s\n", proxyBound, cd->sock, proxy_port,
           "OVER_TCP=Y"
    	);
    	cd->step = CLIENT_REQUEST;
        control->proxy_ip.sin_port = htons(proxy_port);
    	add_socket(cd->sock, sock_data, cd, &master, MASTER_READ);
		{
			char *p1 = strdup(inet_ntoa(cd->addr.sin_addr));
			char *p2 = strdup(inet_ntoa(control->proxy_ip.sin_addr));
        	write_log(
        		"Client %s:%d registered => assigned %s:%d",
	            p1, ntohs(cd->addr.sin_port),
    	        p2, proxy_port
        	);
            free(p1);
            free(p2);
		}
        register_on_server();
  	}
  	end:
  	memcpy(control->type.client.buff, buff, strlen(buff));

  	control->type.client.nsize = strlen(buff);
    control->proxy_ip.sin_port = htons(proxy_port);
  	add_socket(control->sock, sock_data, control, &master, MASTER_WRITE);
  	return 0;
}

int proxy_bind_reply(CLIENT_DATA *cd){
  	SOCKET client_socket;
  	SOCKET sock;
  	int type;
  	int protocol;
  	struct sockaddr_in client_sa;

  	CLIENT_DATA *udp_client;
  	int i = 0;
  	char *p = tokenize_cmd(buf);
  	while((p != NULL) && (i < 4)){
    	switch(i){
      		case 0:
                client_socket = atoi(p);
        		break;
      		case 1:
                type = atoi(p);
        		break;
      		case 2:
                protocol = atoi(p);
        		break;
      		case 3:
                cd->proxy_ip.sin_addr.s_addr = inet_addr(p);
        		break;
    	}
    	i++;
    	p = tokenize_cmd(NULL);
  	}
  	client_sa.sin_family = AF_INET;
  	client_sa.sin_addr = cd->addr.sin_addr;
  	client_sa.sin_port = cd->addr.sin_port;
  	if(
   		(sock = socket(AF_INET, type, protocol)) == INVALID_SOCKET
  	){
    	sprintf(buf, "%s: %s; %d\n", proxyBindError, error_id(), __LINE__);
    	delay(FW_DELAY_MSEC);   /* delay 10 ms */
    	if(send(cd->sock, buf, strlen(buf), 0) < 0){
            if(verbose){
            	handle_error(__FILE__, __LINE__);
      		}
        }
    	return 0;
  	}

  	udp_client = (CLIENT_DATA *)get_client_data(sock, client_sa);
  	udp_client->type.udp.control = cd;
  	udp_client->type.udp.client_socket = client_socket;
  	udp_client->client_type = CLIENT_TYPE_UDP;
    {
#if defined(WIN32) || defined(__WIN32__)
    	unsigned thid;
        _beginthreadex(NULL, 0, &binding, udp_client, 0, &thid);
#else
        pthread_t thread;
        pthread_create(&thread, NULL, &binding, udp_client);
#endif
  	}
  	return 0;
}

int proxy_close_reply(CLIENT_DATA *cd){
  	int i = 0;
  	CLIENT_DATA *udp = NULL;
  	char *p = tokenize_cmd(buf);
  	while((p != NULL) && (i < 1)){
    	switch(i){
      		case 0:
            	udp = (CLIENT_DATA *)hashmap_get(sock_data, atoi(p));
        		break;
    	}
    	i++;
    	p = tokenize_cmd(NULL);
  	}
    cd->closed = 1;
  	if(udp != NULL){
		{
			char *p1 = strdup(inet_ntoa(cd->addr.sin_addr));
			char *p2 = strdup(inet_ntoa(cd->proxy_ip.sin_addr));
        	write_log(
	        	"Client %s:%d unregistered, address %s:%d freed",
	            p1, ntohs(cd->addr.sin_port),
    	        p2, ntohs(cd->proxy_ip.sin_port)
        	);
            free(p1);
            free(p2);
		}
    	rm_socket(udp->sock, sock_data, &master, MASTER_ALL);
  	}
  	return 0;
}

int proxy_bound_ok_replay(CLIENT_DATA *cd, int nsize, struct sockaddr_in sa){
  	cd->addr = sa;
  	if(!cd->type.udp.control->authorized){
    	return unauthorized(cd, proxyAuth1);
    }
  	delay(FW_DELAY_MSEC);   /* delay 10 ms */
  	if(
	    (nsize = send(cd->type.udp.control->sock, buf + sizeof(struct sockaddr_in),
   			strlen(buf + sizeof(struct sockaddr_in)), 0)) != SOCKET_ERROR
  	){
  	}
  	else {
    	if(verbose){
        	handle_error(__FILE__, __LINE__);
    	}
  	}

  	if(verbose){
		write_log("Client %s:%d heartbeat success", inet_ntoa(cd->addr.sin_addr), ntohs(cd->addr.sin_port));
    }
  	return 0;
}

int resend_packet(CLIENT_DATA *cd, char *buff, int nsize){
  	struct sockaddr_in sa;
  	memcpy(&sa, buff, sizeof(struct sockaddr_in));

    if(ntohs(sa.sin_port) <= 1024){
        return 0;
  	}

  	nsize = sendto(cd->sock, buff + sizeof(struct sockaddr_in),
    	nsize - sizeof(struct sockaddr_in), 0,
      	(struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  	if(
	    nsize != SOCKET_ERROR
  	){
  	}
  	else {
    	if(ERRNO_EQUALS(EAFNOSUPPORT)){
        }
        else {
	    	if(verbose){
    	    	handle_error(__FILE__, __LINE__);
    		}
        }
  	}
  	return 0;
}

int receive_packet(CLIENT_DATA *cd, int nsize, struct sockaddr_in sa){
    if(cd->type.udp.control->type.client.over_tcp){
        return receive_over_tcp(cd, nsize, sa);
  	}
  	else  {
    	memmove(buf + sizeof(struct sockaddr_in), buf, nsize);
    	memcpy(buf, &sa, sizeof(struct sockaddr_in));
    	if(
   			(
   				nsize = sendto(cd->sock, buf,
   					nsize + sizeof(struct sockaddr_in)
     				, 0,
  	    			(struct sockaddr*)&cd->addr, sizeof(struct sockaddr_in))
   			) != SOCKET_ERROR
    	){
    	}
    	else {
        	if(verbose){
            	handle_error(__FILE__, __LINE__);
      		}
    	}
  	}
  	return 0;
}








