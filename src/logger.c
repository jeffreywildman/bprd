#include "logger.h"

#include <stdarg.h>     /* for vsnprintf() */
#include <stdio.h>      /* for snprintf() */
#include <stdlib.h>     /* for exit() */
#include <syslog.h>


static enum logger_state {
    LOGGER_DOWN = 0,
    LOGGER_UP   = 1
} state = LOGGER_DOWN;

static char *logger_prioritynames[] = {
    "EMERG",
    "ALERT",
    "CRTCL",
    "ERROR",
    "WRNNG",
    "NOTCE",
    "INFO ",
    "DEBUG",
    NULL
};
#define LOGGER_MSGSTRLEN 256
static char logger_msgstr[LOGGER_MSGSTRLEN];


/**
 * Initialize the logger.
 */
void logger_init() {
  
    if (state == LOGGER_DOWN) {
        openlog(NULL, LOG_PID | LOG_PERROR | LOG_NDELAY, LOG_USER);
        state = LOGGER_UP;
    }
}


/**
 * Cleanup the logger.
 */
void logger_cleanup() {

    if (state == LOGGER_UP) {
        closelog();
        state = LOGGER_DOWN;
    }
}


/**
 * Log a formatted message.  Exit if an error message.
 *
 * \note This function is not thread-safe.
 * \todo Make this function thread-safe (either mutex or unique copy of logger_msgstr inside function).
 *
 * \param priority A syslog priority level.
 * \param file Name of file originating log message.
 * \param line Line number of file originating log message.
 * \param fmt Printf-style format string.
 * \param ... Arguments corresponding to format string fmt.
 */
void logger_log(int priority, const char *file, const int line, 
                const char *fmt, ...) {

    int n, m = 0;

    if (state == LOGGER_DOWN) {
        logger_init();
    }

    /* write first part of message */
    n = snprintf(logger_msgstr, LOGGER_MSGSTRLEN, "%s %s:%d ", logger_prioritynames[LOG_PRI(priority)], file, line);

    /* if room, write second part of message*/
    if (n < LOGGER_MSGSTRLEN) {
        va_list fmtargs;
        va_start(fmtargs, fmt);
        m = vsnprintf(logger_msgstr+n, LOGGER_MSGSTRLEN-n, fmt, fmtargs);
        va_end(fmtargs);
    }

    /**
     * \todo Decide whether this function is responsible for also tacking on an error message. 
     */
//    #define MAX_ERROR_MSG_SIZE 256
//    int priority = LOG_ERR;
//    char errbuf[MAX_ERROR_MSG_SIZE];
//    /* thread-safe error message grabbing */
//    /* also, don't print Success when reporting an error, duh */
//    if ((errno != 0) && (strerror_r(errno, errbuf, MAX_ERROR_MSG_SIZE) == 0)) {
//        syslog(priority, "ERROR %s:%d %s: %s", file, line, msg, errbuf);
//    } else {
//        syslog(priority, "ERROR %s:%d %s", file, line, msg);
//    }

    if (n + m < LOGGER_MSGSTRLEN) {
        /* no truncation occurred, good! */
        syslog(priority, "%s\n", logger_msgstr);
    } else {
        /* properly terminate message string before logging */
        snprintf(logger_msgstr+LOGGER_MSGSTRLEN-1,1,"%s","");
        syslog(priority, "%s\n", logger_msgstr);
        syslog(LOG_WARNING, 
               "%s %s:%d Previous log message truncated due to length\n", 
               logger_prioritynames[LOG_WARNING], file, line);
    }

    /* special handling of an error */
    if (LOG_PRI(priority) == LOG_ERR) {
        exit(1);
    }
}
