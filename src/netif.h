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

#ifndef __NETIF_H
#define __NETIF_H

/* Function signatures and definitions from <net/if.h> */

/* Convert an interface name to an index, and vice versa.  */
extern unsigned int netif_nametoindex (const char *__ifname);
extern char *netif_indextoname (unsigned int __ifindex, char *__ifname);

/* Length of interface name.  */
#define NETIF_NAMESIZE	16

#endif /* __NETIF_H */
