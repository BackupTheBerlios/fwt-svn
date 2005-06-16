#include "client_handler.h"

int behind_firewall(CLIENT_DATA *cd);
int register_proxy(CLIENT_DATA *cd);
int find_proxy(CLIENT_DATA *cd);
int get_proxy(CLIENT_DATA *cd);
int disconnect(CLIENT_DATA *cd);
int reply_behind_firewall(CLIENT_DATA *cd);

int client_connected(CLIENT_DATA *cd){
//	int nsize;
	sprintf(buf, "%s", sHello);
	if(send(cd->sock, buf, strlen(buf), 0) < 0){
    	if(verbose){
    		handle_error(__FILE__, __LINE__);
	    }
    	return MASTER_ALL;
  	}
  	cd->step = CLIENT_REQUEST;
  	return 0;
}

int client_request(CLIENT_DATA *cd){
	int nsize;
	if((nsize = recv(cd->sock, buf, BUFFER_SIZE - 1, 0)) < 0){
    	if(verbose){
        	if(!ERRNO_EQUALS(ECONNRESET)){
    			handle_error(__FILE__, __LINE__);
            }
    	}
    	return MASTER_ALL;
  	}
  	if(nsize == 0){
  		return disconnect(cd);
  	}
  	if(cd->client_type == CLIENT_TYPE_CLIENT){
    	if(is_command(buf, getProxy)){
    		return get_proxy(cd);
    	}
  	}
  	else if(cd->client_type == CLIENT_TYPE_PROXY){
  	}
  	else if(cd->client_type == CLIENT_TYPE_UNKNOWN){
    	if(is_command(buf, isBehindFirewall)){
    		return behind_firewall(cd);
	    }
    	if(is_command(buf, registerProxy)){
    		return register_proxy(cd);
	    }
  	}
	return 0;
}

THREAD_FUNC test_connection(void* arg){
	CLIENT_DATA *cd = (CLIENT_DATA *)arg;
  	int client_addr_sz = sizeof(struct sockaddr_in);
   	write_log("Try to connect to test address %s:%d", inet_ntoa(cd->type.client.test_addr.sin_addr), ntohs(cd->type.client.test_addr.sin_port));

  	if(connect(cd->type.client.test_sock, (struct sockaddr *)&cd->type.client.test_addr, client_addr_sz) != 0){
  		cd->type.client.need_proxy = 1;
  	}
  	else {
  		cd->type.client.need_proxy = 0;
  	}
  	cd->type.client.tested = 1;
    if(verbose){printf("FWT : Client tested:need_proxy = %d\n", cd->type.client.need_proxy);}
  	add_socket(cd->sock, NULL, NULL, cd->master, MASTER_WRITE);
  	return 0;
}
int register_proxy(CLIENT_DATA *cd){
  	PROXY_DATA pd, *pd2;
  	unsigned i = 0;
  	int found = 0;
  	char *p;
printf("[%s, %d] %s\n", __FILE__, __LINE__, buf);
  	p = tokenize_cmd(buf);
  	i = 0;
  	pd.addr = cd->addr;
  	while((p != NULL) && (i < 3)){
  		switch(i){
    		case 0:
		        if(strcmp(p, "") != 0){
        			pd.addr.sin_addr.s_addr = inet_addr(p);
        		}
      			break;
    		case 1:
			  	pd.addr.sin_port = htons(atoi(p));
	      		break;
    		case 2:
			  	pd.nclients = atoi(p);
	      		break;
    	}
    	i++;
	  	p = tokenize_cmd(NULL);
  	}

  	pd.last_registered = time(NULL);

  	pd2 = (PROXY_DATA *)malloc(sizeof(PROXY_DATA));
  	pd2->addr = pd.addr;
  	pd2->nclients = pd.nclients;
  	pd2->last_registered = pd.last_registered;

  	for(i = 0; i < arraylist_size(proxy_data); i++){
  		PROXY_DATA *pd1 = (PROXY_DATA *)arraylist_get(proxy_data, i);
    	if(
    		(pd1->addr.sin_port == pd.addr.sin_port)
      	&& (pd1->addr.sin_addr.s_addr == pd.addr.sin_addr.s_addr)
    	){
    		found = 1;
      		arraylist_set(proxy_data, i, (char*)pd2);
      		break;
    	}
  	}
  	if(!found){
  		arraylist_add(proxy_data, (char*)pd2);
  	}
  	sprintf(buf, "%s: %d\n", registeredProxy, registration_period);
  	if(send(cd->sock, buf, strlen(buf), 0) == SOCKET_ERROR){
    	if(verbose){
      		handle_error(__FILE__, __LINE__);
    	}
    	return MASTER_ALL;
  	}

  	cd->client_type = CLIENT_TYPE_PROXY;
    cd->step = CLIENT_LAST_STEP;
 	add_socket(cd->sock, sock_data, cd, cd->master, MASTER_WRITE);
	write_log("Proxy from %s:%d registered successfully", inet_ntoa(pd2->addr.sin_addr), ntohs(pd2->addr.sin_port));
  	return 0;
}

int behind_firewall(CLIENT_DATA *cd){
  	struct sockaddr_in client_addr;
//  int client_addr_sz = sizeof(struct sockaddr_in);
  	unsigned int test_port;
  	char *p;
    char *test_port_str;
  	unsigned i = 0;
  	p = tokenize_cmd(buf);
  	while((p != NULL) && (i < 1)){
  		switch(i){
    		case 0:
                test_port_str = strdup(p);
				test_port = atoi(test_port_str);
                free(test_port_str);
      			break;
        }
    	i++;
	  	p = tokenize_cmd(NULL);
  	}

  	cd->client_type = CLIENT_TYPE_CLIENT;
  	cd->type.client.proxy_addr.sin_addr.s_addr = INADDR_NONE;
  	cd->type.client.tested = 0;
  	cd->type.client.test_sock = socket(AF_INET, SOCK_STREAM, 0);
  	if(
    		cd->type.client.test_sock == INVALID_SOCKET
  	){
 	   if(verbose){
    		handle_error(__FILE__, __LINE__);
	    }
    	return MASTER_ALL;
  	}
  	cd->type.client.test_addr.sin_family = AF_INET;
  	cd->type.client.test_addr.sin_port = htons(test_port);
  	cd->type.client.test_addr.sin_addr.s_addr = cd->addr.sin_addr.s_addr;

	{
#if defined(WIN32) || defined(__WIN32__)
    	unsigned thid;
		_beginthreadex(NULL, 0, &test_connection, cd, 0, &thid);
#else
  		pthread_t thread;
	  	pthread_create(&thread, NULL, &test_connection, cd);
#endif
  	}
	return 0;
}

int reply_behind_firewall(CLIENT_DATA *cd){
	char *format = "%s: %s; %s; %d\n";
  	int disconnect = 0;
	if(!cd->type.client.need_proxy && !always_proxy){
    	sprintf(
      		buf, format,
      		isBehindFirewall, "NO",
      		inet_ntoa(cd->addr.sin_addr), 0
    	);
        write_log(
            "Client request for Proxy from %s:%d => rejected: not behind firewall",
            inet_ntoa(cd->addr.sin_addr),
            ntohs(cd->addr.sin_port)
        );
  	}
  	else {
  		find_proxy(cd);
  		if(cd->type.client.proxy_addr.sin_addr.s_addr == INADDR_NONE){
      		sprintf(
        		buf, format,
        		isBehindFirewall, "YES",
        		noProxyAvailable,
        		0
      		);
	    	disconnect = 1;
            write_log(
                "Client request for Proxy from %s:%d => rejected: no proxy",
                inet_ntoa(cd->addr.sin_addr),
                ntohs(cd->addr.sin_port)
            );
    	}
    	else {
			char *p1;
			char *p2;
      		sprintf(
        		buf, format,
        		isBehindFirewall, "YES",
        		inet_ntoa(cd->type.client.proxy_addr.sin_addr),
        		ntohs(cd->type.client.proxy_addr.sin_port)
      		);
			p1 = strdup(inet_ntoa(cd->addr.sin_addr));
			p2 = strdup(inet_ntoa(cd->type.client.proxy_addr.sin_addr));
            write_log(
                "Client request for Proxy from %s:%d => assigned %s:%d",
                p1,
                ntohs(cd->addr.sin_port),
                p2,
                ntohs(cd->type.client.proxy_addr.sin_port)
            );
            free(p1);
            free(p2);
    	}
  	}
  	if(
    		send(cd->sock, buf, strlen(buf), 0) == SOCKET_ERROR
  	){
    	if(verbose){
      		handle_error(__FILE__, __LINE__);
        }
    	return MASTER_ALL;
  	}

  	if(disconnect){
  		cd->step = CLIENT_LAST_STEP;
		add_socket(cd->sock, sock_data, cd, cd->master, MASTER_WRITE);
	  	return 0;
  	}
  	return MASTER_WRITE;
}

int client_write(CLIENT_DATA *cd){
	if(cd->client_type == CLIENT_TYPE_CLIENT){
  		if(cd->type.client.tested){
      		return reply_behind_firewall(cd);
    	}
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

int client_handler(void *data, MASTER_FD* master, int what){
	int nsize;
	CLIENT_DATA *cd = (CLIENT_DATA *)data;

	if(cd->step == CLIENT_LAST_STEP){
    	return disconnect(cd);
  	}
  	cd->master = master;
//printf("[%s,%d] %d\n", __FILE__, __LINE__, what);
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

int find_proxy(CLIENT_DATA *cd){
	unsigned i;
  	int nclients = -1;
  	long now = time(NULL);
    int step_num;
  	cd->type.client.proxy_addr.sin_addr.s_addr = INADDR_NONE;
    for(step_num = 0; step_num < 2; step_num++){
        for(i = 0; i < arraylist_size(proxy_data); i++){
            PROXY_DATA *pd1 = (PROXY_DATA *)arraylist_get(proxy_data, i);
printf("[%s, %d] %d: %d\n", __FILE__, __LINE__, ntohs(pd1->addr.sin_port), pd1->nclients);
            if(now - pd1->last_registered > registration_period){
                if(step_num == 0){
                    continue;
                }
            }
            if(nclients == -1){
            	cd->type.client.proxy_addr = pd1->addr;
                nclients = pd1->nclients;
            }
            else if(nclients > pd1->nclients){
                int j;
                for(j = 0; j < cd->type.client.bp_cnt; j++){
                    if(
                        (cd->type.client.bad_proxies[j].sin_port == pd1->addr.sin_port)
                        && (cd->type.client.bad_proxies[j].sin_addr.s_addr == pd1->addr.sin_addr.s_addr)
                    ){
                        goto l1;
                    }
                }
            	cd->type.client.proxy_addr = pd1->addr;
                nclients = pd1->nclients;
                l1:;
            }
        }
        if(cd->type.client.proxy_addr.sin_addr.s_addr != INADDR_NONE){
        	break;
        }
    }
	return 0;
}

int get_proxy(CLIENT_DATA *cd){
	if(cd->type.client.proxy_addr.sin_addr.s_addr != INADDR_NONE){
	  	if(cd->type.client.bad_proxies == 0){
    	  	cd->type.client.bad_proxies = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
      		cd->type.client.bp_cnt = 0;
	    }
    	else {
	      	cd->type.client.bad_proxies = (struct sockaddr_in*)realloc(
    	 	 	cd->type.client.bad_proxies, sizeof(struct sockaddr_in)*(cd->type.client.bp_cnt + 1)
      		);
	    }
    	cd->type.client.bad_proxies[cd->type.client.bp_cnt] = cd->type.client.proxy_addr;
	    cd->type.client.bp_cnt++;
  	}
  	cd->type.client.need_proxy = 1;
  	return reply_behind_firewall(cd);
}

int disconnect(CLIENT_DATA *cd){
	if(cd->client_type == CLIENT_TYPE_CLIENT){
  		free(cd->type.client.bad_proxies);
  	}
  	return DISCONNECT;
}

