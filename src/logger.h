#ifndef __LOGGER_H
#define __LOGGER_H

#define DUBP_LOG_ERR(x) log_err(__FILE__,__LINE__,(x))
#define DUBP_LOG_DBG(x) log_dbg(__FILE__,__LINE__,(x))

extern void log_init();
extern void log_cleanup();
extern void log_err(const char *file, const int line, const char *msg);
extern void log_dbg(const char *file, const int line, const char *msg);

#endif /* __LOGGER_H */
