#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "logger.h"
#include "dubp.h"

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

    if (strerror_r(errno, errbuf, MAX_ERROR_MSG_SIZE) == 0) {
        syslog(priority, "%s:%d ERROR %s: %s", file, line, msg, errbuf);
    } else {
        syslog(priority, "%s:%d ERROR %s", file, line, msg);
    }
    
    exit(1);

}


/* log debug */
void log_dbg(const char *file, const int line, const char *msg) {

    int priority = LOG_DEBUG;
    
    syslog(priority, "%s:%d DEBUG %s", file, line, msg);
}


/* thread-safe customized error printing */
void print_error(const char *func, const char *msg) {

    fprintf(stderr, "[%s] %s(): Error", program, func);
    if (msg != NULL) {
        fprintf(stderr, ": %s", msg);
    }
    fprintf(stderr, "\n");
}


/* thread-safe customized error printing */
void print_error2(const char *func, const char *msg, const int err) {

    char errbuf[MAX_ERROR_MSG_SIZE];
    
    fprintf(stderr, "[%s] %s(): Error", program, func);
    if (msg != NULL) {
        fprintf(stderr, ": %s", msg);
    }
    if (strerror_r(err, errbuf, MAX_ERROR_MSG_SIZE) == 0) {
        fprintf(stderr, ": %s", errbuf);
    }
    fprintf(stderr, "\n");
}


