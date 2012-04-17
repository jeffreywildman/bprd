#ifndef __DUBP_H
#define __DUBP_H

#include <stdint.h>
#include <sys/socket.h>

#include "ntable.h"

#define ETH_ALEN 6

/* RFC 5498 - IANA Allocations for Mobile Ad Hoc Network (MANET) Protocols */
#define MANET_LINKLOCAL_ROUTERS_V4 "224.0.0.109"
#define MANET_LINKLOCAL_ROUTERS_V6 "FF02::6D"
#define IPPROTO_MANET 138  /* if running over SOCK_RAW */
#define IPPORT_MANET 269   /* if running over SOCK_DGRAM */

#define DUBP_DEFAULT_INTERFACE "eth0"
#define DUBP_DEFAULT_HELLO_INTERVAL 1  /* seconds */

/* TODO: move this into a config.h */
#define DUBP_DEFAULT_PIDLEN 25
#define DUBP_DEFAULT_PIDSTR "/var/run/dubpd.pid"

#define DUBP_MSG_TYPE_HELLO 1


/* dubpd instance */
struct dubp {
    char    *program;       /* program name */
    int     dmode;    
    int     ipver;         
    char    *confile;       /* config file location */
    char    *pidfile;       /* pidfile location */

    int sockfd;         /* socket running dubpd */

    /* TODO: allow dubpd to run over mutiple interfaces */
    char *if_name;              /* HW interface running dubpd */
    struct sockaddr *saddr;     /* generic structure address running dubpd */
    uint8_t saddrlen;           /* size of socket address structure */

    struct sockaddr *maddr;     /* generic structure for multicast address */
    uint8_t maddrlen;           /* size of multicast address structure */

    /* hello thread data */   
    pthread_t hello_tid;
    uint16_t hello_seqno;

    /* timers */
    unsigned int hello_interval; 

    /* TODO: do i need to keep a reference to my address? */
    commodityhead_t chead;  /* my commodity list */
    uint8_t csize;          /* size of my commodity list */
    /* TODO: do i need a mutex for my commodity list? */

    /* neighbor table */
    neighbor_table_t ntable;

} dubpd;


#endif /* __DUBP_H */
