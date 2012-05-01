#include "logger.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>


#define MAX_ERROR_MSG_SIZE 256


void log_init() {
   
    openlog(NULL, LOG_PID | LOG_PERROR | LOG_NDELAY, LOG_USER);

}


void log_cleanup() {

    closelog();

}


/* log error and get out of here */
void log_err(const char *file, const int line, const char *msg) {

    int priority = LOG_ERR;
    char errbuf[MAX_ERROR_MSG_SIZE];

    /* thread-safe error message grabbing */
    /* also, don't print Success when reporting an error, duh */
    if ((errno != 0) && (strerror_r(errno, errbuf, MAX_ERROR_MSG_SIZE) == 0)) {
        syslog(priority, "ERROR %s:%d %s: %s", file, line, msg, errbuf);
    } else {
        syslog(priority, "ERROR %s:%d %s", file, line, msg);
    }
    
    exit(1);

}


/* log debug */
void log_dbg(const char *file, const int line, const char *msg) {

    int priority = LOG_DEBUG;
    
    syslog(priority, "DEBUG %s:%d %s", file, line, msg);
}
