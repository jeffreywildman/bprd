#ifndef __UTIL_H
#define __UTIL_H

#define BIT(x) (1ULL<<(x))

extern int mac_addr_a2n(unsigned char *mac_addr, char *arg);
extern void mac_addr_n2a(char *mac_addr, unsigned char *arg);

#endif /* __UTIL_H */
