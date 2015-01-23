/**
 * The BackPressure Routing Daemon (bprd).
 *
 * Copyright (c) 2012 Jeffrey Wildman <jeffrey.wildman@gmail.com>
 * Copyright (c) 2012 Bradford Boyle <bradford.d.boyle@gmail.com>
 *
 * bprd is released under the MIT License.  You should have received
 * a copy of the MIT License with this program.  If not, see
 * <http://opensource.org/licenses/MIT>.
 */

#ifndef __LOGGER_H
#define __LOGGER_H

#include <syslog.h>     /* for priority definitions, ex. LOG_ERR */

#define BPRD_LOG_INFO(fmt,...) logger_log(LOG_INFO,NULL,0,(fmt),##__VA_ARGS__)
#define BPRD_LOG_ERR(fmt,...) logger_log(LOG_ERR,__FILE__,__LINE__,(fmt),##__VA_ARGS__)
#define BPRD_LOG_DBG(fmt,...) logger_log(LOG_DEBUG,__FILE__,__LINE__,(fmt),##__VA_ARGS__)

extern void logger_init();
extern void logger_cleanup();
extern void logger_log(int priority, 
                       const char *file, 
                       const int line, 
                       const char *fmt, ...) __attribute__ ((format (printf,4,5)));

#endif /* __LOGGER_H */
