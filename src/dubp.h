#ifndef __DUBP_H
#define __DUBP_H

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


/* dubpd instance */
struct dubp {
    char *program;      /* program name */
    char *pidfile;      /* pidfile location */

    /* TODO: allow dubpd to run over mutiple interfaces */
    char *ifrn_name;     /* HW interface running dubpd */ 

    /* timers */
    unsigned int hello_interval; 


    /* TODO: do i need to keep a reference to my address? */
    //commodityhead_t chead;  /* my commodity list */

    /* neighbor table */
    //neighbor_table_t ntable;

} dubpd;



#endif /* __DUBP_H */
