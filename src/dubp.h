#ifndef __DUBP_H
#define __DUBP_H

#define ETH_ALEN 6

/* RFC 5498 - IANA Allocations for Mobile Ad Hoc Network (MANET) Protocols */
#define MANET_LINKLOCAL_ROUTERS_V4 "224.0.0.109"
#define MANET_LINKLOCAL_ROUTERS_V6 "FF02::6D"
#define IPPROTO_MANET 138  /* if running over SOCK_RAW */
#define IPPORT_MANET 269   /* if running over SOCK_DGRAM */

#define DUBP_INTERFACE "eth0"
#define DUBP_HELLO_INTERVAL 1  /* seconds */

#endif /* __DUBP_H */
