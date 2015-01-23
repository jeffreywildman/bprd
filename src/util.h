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

#ifndef __UTIL_H
#define __UTIL_H

#include <arpa/inet.h>

#define BIT(x) (1ULL<<(x))

extern int mac_addr_a2n(unsigned char *mac_addr, char *arg);
extern void mac_addr_n2a(char *mac_addr, unsigned char *arg);

typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr_in6 sockaddr_in6_t;

extern int addr2str(const sockaddr_t *saddr, char *host, size_t hostlen);
extern void print_args(int argc, char **argv);

extern void print_addrs();

#endif /* __UTIL_H */
