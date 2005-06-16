#include "common.h"
#include "arraylist.h"

#if !defined(WIN32) && !defined(__WIN32__)
ARRAYLIST *keylist;

static int started = 0;

void clean_keylist(void){
	int i;
	for(i = 0; i < arraylist_size(keylist); i++){
  		char *tmp = (char*)arraylist_get(keylist, i);
  		unlink(tmp);
    	free(tmp);
  	}
  	free(keylist->list);
  	free(keylist);
}

key_t get_key(void){
	char temp[255];
  	strcpy(temp, "/tmp/pckeyXXXXXX");
  	if(mkstemp(temp) != -1){
    	if(!started){
    		keylist = arraylist(0);
      		started = 1;
    	}
    	arraylist_add(keylist, strdup(temp));
        return ftok(temp, getpid());
  	}
  	return -1;
}
#endif

int use_syslog = 0;
char *logging_name = NULL;

int write_log(const char* format, ...)
{
    char sz[1024], *tmp;
    int n;
    va_list ap;
    va_start(ap, format);
#if defined(WIN32) || defined(__WIN32__)
    n = _vsnprintf(sz, 1024, format, ap);
#else
    n = vsnprintf(sz, 1024, format, ap);
#endif
    va_end(ap);

    if(logging_name != NULL){
	    tmp = strdup(sz);
    	sprintf(sz, "[%s] %s", logging_name, tmp);
        free(tmp);
    }
    if(!use_syslog){
        printf("%s\n", sz);
        fflush(stdout);
    } else {
        syslog(LOG_USER|LOG_INFO,sz);
    }
    return 0;
}

int write_err(const char* format, ...)
{
    char sz[1024], *tmp;
    int n;
    va_list ap;
    va_start(ap, format);
#if defined(WIN32) || defined(__WIN32__)
    n = _vsnprintf(sz, 1024, format, ap);
#else
    n = vsnprintf(sz, 1024, format, ap);
#endif
    va_end(ap);

    if(logging_name != NULL){
	    tmp = strdup(sz);
    	sprintf(sz, "[%s] %s", logging_name, tmp);
        free(tmp);
    }

    if(!use_syslog){
        fprintf(stderr, "%s\n", sz);
        fflush(stderr);
    } else {
        syslog(LOG_ERR|LOG_INFO, sz);
    }
    return 0;
}


