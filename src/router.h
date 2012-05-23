#ifndef __ROUTER_H
#define __ROUTER_H

#include <sys/socket.h>     /* for struct sockaddr */

void router_init();
void router_cleanup();
void router_route_update(struct sockaddr *dst, struct sockaddr *nexthop, unsigned int ifindex);

#endif /* __ROUTER_H */
