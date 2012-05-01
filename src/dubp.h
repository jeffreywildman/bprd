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
#define DUBP_DEFAULT_HELLO_INTERVAL 1   /* seconds */
#define DUBP_DEFAULT_NEIGHBOR_TIMEOUT 5 /* # of missed hello messages */

/* TODO: move this into a config.h */
#define DUBP_DEFAULT_PIDLEN 25
#define DUBP_DEFAULT_PIDSTR "/var/run/dubpd.pid"
#define DUBP_DEFAULT_CONLEN 25
/* TODO: move this into a config.h!!! */
#define DUBP_DEFAULT_CONSTR "/etc/dubpd.conf"
/* the following assumes dubpd is run from project's top level directory */
//#define DUBP_DEFAULT_CONSTR "./scripts/dubp.conf"

#define DUBP_MSG_TYPE_HELLO 1

#define DUBP_MSGTLV_TYPE_COM 1
#define DUBP_MSGTLV_TYPE_COMKEY 2
#define DUBP_MSGTLV_TYPE_BACKLOG 3


/* dubpd instance */
typedef struct dubp {
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
    pthread_t hello_writer_tid;
    pthread_t hello_reader_tid;
    uint16_t hello_seqno;

    /* timers */
    uint8_t hello_interval;     /* seconds */
    uint8_t neighbor_timeout;   /* seconds */
   
    /* commodity table */
    list_t clist;               /* my commodity list */
    /* TODO: do i need a mutex for my commodity list? */
    pthread_t backlogger_tid;

    /* neighbor table */
    neighbortable_t ntable;

} dubp_t;

extern dubp_t dubpd;


#endif /* __DUBP_H */
