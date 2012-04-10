#ifndef __PIDFILE_H
#define __PIDFILE_H

extern int pidfile_create(const char *pathname);
extern int pidfile_destroy(const char *pathname);

#endif /* __PIDFILE_H */
