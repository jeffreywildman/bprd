#ifndef __LOGGER_H
#define __LOGGER_H

#include <syslog.h>

#define DUBP_LOG_ERR(fmt,...) logger_log(LOG_ERR,__FILE__,__LINE__,(fmt),##__VA_ARGS__)
#define DUBP_LOG_DBG(fmt,...) logger_log(LOG_DEBUG,__FILE__,__LINE__,(fmt),##__VA_ARGS__)

extern void logger_init();
extern void logger_cleanup();
extern void logger_log(int priority, 
                       const char *file, 
                       const int line, 
                       const char *fmt, ...) __attribute__ ((format (printf,4,5)));

#endif /* __LOGGER_H */
