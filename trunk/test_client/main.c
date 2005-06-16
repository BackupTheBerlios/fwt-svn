#include "common.h"
#include "proxy_socket.h"
#include "error_handler.h"
#include "input_params.h"

struct sockaddr_in server_addr;
struct sockaddr_in peer_address;
void __cdecl read_param(char *selector, int argc, char **argv){
	int i;
  if(!strcmp(selector, "-server-address") || !strcmp(selector, "A")){
	  server_addr.sin_addr.s_addr = inet_addr(argv[0]);
  }
  else if(!strcmp(selector, "-server-port") || !strcmp(selector, "P")){
	  server_addr.sin_port = htons(atoi(argv[0]));
  }
  else if(!strcmp(selector, "-peer-address") || !strcmp(selector, "a")){
	  peer_address.sin_addr.s_addr = inet_addr(argv[0]);
  }
  else if(!strcmp(selector, "-peer-port") || !strcmp(selector, "p")){
	  peer_address.sin_port = htons(atoi(argv[0]));
  }
  else if(!strcmp(selector, "-user-name") || !strcmp(selector, "u")){
    set_username(argv[0]);
  }
  else if(!strcmp(selector, "-verbose")){
    verbose = 1;
  }
}

int main(int argc, char* argv[])
{
	int i;
  SOCKET my_sock, my_sock1;
  struct sockaddr_in local_addr, *local_addr1, cl_addr;
  int local_addr_len = sizeof(struct sockaddr_in), cl_addr_len;
	struct timeval timeout;
	fd_set rset, eset;
  int maxs;
	char buff[BUFFER_SIZE];
  int nsize;
  unsigned short prt = 20002;
  int rbsz = BUFFER_SIZE - 1;
  long last_sendto = 0l, now;

  debug = stdout;
  logging_name = "Test client";

#if defined(WIN32) || defined(__WIN32__)
  if (WSAStartup(0x0202,(WSADATA *) &buff[0]))
  {
    handle_error(__FILE__, __LINE__);
    return -1;
  }
#endif

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8890);
  server_addr.sin_addr.s_addr = inet_addr("172.16.17.78");

  peer_address.sin_family = AF_INET;
  peer_address.sin_port = htons(30000);
  peer_address.sin_addr.s_addr = inet_addr("172.16.17.78");

  cl_addr_len = sizeof(struct sockaddr_in);

  parse_input_params(argc, argv, read_param);

  cl_addr = peer_address;

  set_server_address(server_addr);

  my_sock = proxy_socket(AF_INET, SOCK_DGRAM, 0);
  if(proxy_bind(my_sock, (struct sockaddr*)&local_addr, &local_addr_len) != 0){
        handle_error(__FILE__, __LINE__);
        proxy_close(my_sock);
        return 1;
  }

printf("%s:%d %s\n",
inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port),
(get_proxy_over_tcp() ? "(over TCP)" : "")
);
delay(100);
      sprintf(buff, "q");
      if(
      		(nsize=proxy_sendto(my_sock,&buff[0],
                        strlen(buff),0, (struct sockaddr*)&cl_addr, cl_addr_len)) == SOCKET_ERROR
      ){
        handle_error(__FILE__, __LINE__);
        proxy_close(my_sock);
        return 1;
      }
  timeout.tv_sec=0;
  timeout.tv_usec = 500000;
  FD_ZERO(&rset);
  FD_SET(my_sock, &rset);
  maxs = my_sock + 1;

	while(1){
  	int res = proxy_select(maxs, &rset, NULL, NULL, &timeout);
    if(res == SOCKET_ERROR){
			if(!ERRNO_EQUALS(EINTR)){
        handle_error(__FILE__, __LINE__);
				break;
			}
  	}
   	if(FD_ISSET(my_sock, &rset)){
      if(
      		(nsize=proxy_recvfrom(my_sock,&buff[0],
                        BUFFER_SIZE-1,0, (struct sockaddr*)&cl_addr, &cl_addr_len)) == SOCKET_ERROR
      ){
        handle_error(__FILE__, __LINE__);
        break;
      }
      if(nsize == 0){
        break;
      }
      // ставим завершающий ноль в конце строки
      buff[nsize]=0;
      if(nsize > 0){
				printf("[%s:%d] %s\n", inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port), buff);
				printf("received: %d\n", nsize);
        if(
	    	    (nsize=proxy_sendto(my_sock,&buff[0],
                          strlen(buff),0, (struct sockaddr*)&cl_addr, cl_addr_len)) == SOCKET_ERROR
        ){
          handle_error(__FILE__, __LINE__);
	        break;
        }
	    }
    }
    timeout.tv_sec=0;
    timeout.tv_usec = 500000;
    FD_ZERO(&rset);
    FD_SET(my_sock, &rset);
    maxs = my_sock + 1;
  }
printf("[%s,%d]\n", __FILE__,__LINE__);
	proxy_close(my_sock);
	return 0;
}

