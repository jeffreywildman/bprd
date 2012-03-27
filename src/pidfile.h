#ifndef __PIDFILE_H
#define __PIDFILE_H

extern void pidfile_create(const char *pathname);
extern void pidfile_destroy(const char *pathname);

#endif /* __PIDFILE_H */
