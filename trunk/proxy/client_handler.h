#ifndef client_handlerH
#define client_handlerH

#include "common.h"
#include "server_loop.h"
#include "init.h"
#include "protocol.h"
#include "over_tcp.h"
#include "auth.h"

extern int client_handler(void *data, MASTER_FD* master, int what);
extern int timer_event(void*);
extern int resend_packet(CLIENT_DATA *cd, char *buff, int nsize);
extern int disconnect(CLIENT_DATA *cd);

#endif
