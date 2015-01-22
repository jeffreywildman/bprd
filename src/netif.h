#ifndef __NETIF_H
#define __NETIF_H

/* Function signatures and definitions from <net/if.h> */

/* Convert an interface name to an index, and vice versa.  */
extern unsigned int netif_nametoindex (const char *__ifname);
extern char *netif_indextoname (unsigned int __ifindex, char *__ifname);

/* Length of interface name.  */
#define NETIF_NAMESIZE	16

#endif /* __NETIF_H */
