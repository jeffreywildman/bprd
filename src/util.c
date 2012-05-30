#include "util.h"

#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "dubp.h"

#define ETH_ALEN 6


void mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
    int i, l;

    l = 0;
    for (i = 0; i < ETH_ALEN ; i++) {
        if (i == 0) {
            sprintf(mac_addr+l, "%02x", arg[i]);
            l += 2;
        } else {
            sprintf(mac_addr+l, ":%02x", arg[i]);
            l += 3;
        }
    }
}


int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
    int i;

    for (i = 0; i < ETH_ALEN ; i++) {
        int temp;
        char *cp = strchr(arg, ':');
        if (cp) {
            *cp = 0;
            cp++;
        }
        if (sscanf(arg, "%x", &temp) != 1)
            return -1;
        if (temp < 0 || temp > 255)
            return -1;

        mac_addr[i] = temp;
        if (!cp)
            break;
        arg = cp;
    }
    if (i < ETH_ALEN - 1)
        return -1;

    return 0;
}


int addr2str(const sockaddr_t *saddr, char *host, size_t hostlen) {

    if (!host) {return -EAI_FAIL;}
        
    if (saddr->sa_family == AF_INET) {
        return getnameinfo(saddr, sizeof(sockaddr_in_t), host, hostlen, NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
    } else if (saddr->sa_family == AF_INET6) {
        return getnameinfo(saddr, sizeof(sockaddr_in6_t), host, hostlen, NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
    } else {
        strcpy(host, "\0");
        return 0;
    }
}


void print_addrs () {
    /* get my current address on the above hardware interface */
    /* TODO: completely clean up the following code */
    struct ifaddrs *iflist, *ifhead;
    if (getifaddrs(&iflist) < 0) {
        printf("Unable to get interface addresses\n");
    }

    ifhead = iflist;

    printf("interfaces: \n");
    while(iflist) {
        if (iflist->ifa_name && strcmp(iflist->ifa_name,dubpd.if_name) == 0) {
            printf("%s: ", iflist->ifa_name);
        } else {
            goto skip;
        }
        printf("<");
        if (iflist->ifa_flags & IFF_UP) {printf("UP ");}
        if (iflist->ifa_flags & IFF_BROADCAST) {printf("BCAST ");}
        if (iflist->ifa_flags & IFF_DEBUG) {printf("DBG ");}
        if (iflist->ifa_flags & IFF_LOOPBACK) {printf("LOOP ");}
        if (iflist->ifa_flags & IFF_POINTOPOINT) {printf("PTP ");}
        if (iflist->ifa_flags & IFF_MULTICAST) {printf("MCAST ");}
        printf(">\n");

        char addrstr[NI_MAXHOST];
        size_t addrstrlen = NI_MAXHOST;
        unsigned int family = iflist->ifa_addr->sa_family;

        if (iflist->ifa_addr) {
            printf("\tfamily: %u", family);
            if (family == AF_PACKET) {
                printf(" (AF_PACKET)\n");
            } else if (family == AF_INET) {
                printf(" (AF_INET)\n");
            } else if (family == AF_INET6) {
                printf(" (AF_INET6)\n");
            } else {
                printf(" (other)\n");
            }

            if (family == AF_INET || family == AF_INET6) {
                addr2str(iflist->ifa_addr, addrstr, addrstrlen);
                printf("\taddress: %s\n", addrstr);
                /* store this address for later */
                /* TODO: verify assumption that dubpd.if_name will have only one IPv4 address */
                /* TODO: otherwise, need to know which IPv4 address to use! */
                /* TODO: generalize to v4/v6 - need to know which IPv6 address to use! */
                if (family == AF_INET) {
                    dubpd.saddr = (sockaddr_t *)malloc(sizeof(sockaddr_in_t));
                    memcpy(dubpd.saddr,(const sockaddr_in_t *)iflist->ifa_addr,sizeof(sockaddr_in_t));
                    dubpd.saddrlen = sizeof(sockaddr_in_t);
                }
            }
        }

        if (iflist->ifa_netmask) {
            addr2str(iflist->ifa_netmask, addrstr, addrstrlen);
            printf("\tnetmask: %s\n", addrstr);
        }
        if ((iflist->ifa_flags & IFF_BROADCAST) && iflist->ifa_ifu.ifu_broadaddr) {
            addr2str(iflist->ifa_ifu.ifu_broadaddr, addrstr, addrstrlen);
            printf("\tbroadcast: %s\n", addrstr);
        } else if ((iflist->ifa_flags & IFF_POINTOPOINT) && iflist->ifa_ifu.ifu_dstaddr) {
            addr2str(iflist->ifa_ifu.ifu_dstaddr, addrstr, addrstrlen);
            printf("\tptp dest: %s\n", addrstr);
        }
        if (iflist->ifa_data) {
            printf("\t...has ida_data!\n");
        }
skip:
        iflist = iflist->ifa_next;
    }

    freeifaddrs(ifhead);
}


void print_args(int argc, char **argv) {
    int i = 0;
    printf("Args:\n");
    while (i < argc) {
        printf("\t%s\n",argv[i++]);
    }
}

