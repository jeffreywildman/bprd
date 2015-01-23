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

#include "netif.h"

#include <net/if.h>

/* Note that <netlink/route/link.h> includes <linux/if.h> which collides with <net/if.h>.
 * Instead of writing out our dependency on <net/if.h> by using the newer available functions
 * in libnl libraries, we will quickly provide a shim to access the if_nametoindex() and if_indextoname().
 */

/* Convert an interface name to an index, and vice versa.  */
unsigned int netif_nametoindex (const char *__ifname) {
  return if_nametoindex(__ifname);
}


char *netif_indextoname (unsigned int __ifindex, char *__ifname) {
  return if_indextoname(__ifindex, __ifname);
}
