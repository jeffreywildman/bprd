#ifndef __ROUTER_H
#define __ROUTER_H

#include <sys/socket.h>     /* for struct sockaddr */

extern int router_init(unsigned int ifindex, unsigned int family);
extern void router_cleanup();
extern void router_update();
extern void router_release(unsigned int count);
extern void router_route_update(struct sockaddr *dst,
                                struct sockaddr *nh,
                                unsigned int family,
                                unsigned int ifindex);

#endif /* __ROUTER_H */
