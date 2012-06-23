#ifndef __DUBP_H
#define __DUBP_H

#include <stdint.h>
#include <sys/socket.h>

#include <netlink/addr.h>

#include "ntable.h"

/* RFC 5498 - IANA Allocations for Mobile Ad Hoc Network (MANET) Protocols */
#define MANET_LINKLOCAL_ROUTERS_V4 "224.0.0.109"
#define MANET_LINKLOCAL_ROUTERS_V6 "FF02::6D"
#define IPPROTO_MANET 138  /* if running over SOCK_RAW */
#define IPPORT_MANET 269   /* if running over SOCK_DGRAM */

#define DUBP_DEFAULT_INTERFACE "eth0"
#define DUBP_DEFAULT_HELLO_INTERVAL 1   /* seconds */
#define DUBP_DEFAULT_NEIGHBOR_TIMEOUT 5 /* # of missed hello messages */

#define DUBP_DEFAULT_RELEASE_RATE 1000  /* mseconds */
#define DUBP_DEFAULT_UPDATE_RATE 1000   /* mseconds */

/**< \todo Move this into a config.h. */
#define DUBP_DEFAULT_PIDLEN 25
#define DUBP_DEFAULT_PIDSTR "/var/run/dubpd.pid"
#define DUBP_DEFAULT_CONLEN 25
/**< \todo Move this into a config.h. */
#define DUBP_DEFAULT_CONSTR "/etc/dubpd.conf"

#define DUBP_MSG_TYPE_HELLO 1

#define DUBP_MSGTLV_TYPE_COM 1
#define DUBP_MSGTLV_TYPE_COMKEY 2
#define DUBP_MSGTLV_TYPE_BACKLOG 3


/** 
 * \struct dubp
 * Data structure defining a DUBP process.
 */
typedef struct dubp {
    char    *program;           /**< Program name. */
    int     dmode;              /**< Boolean integer indicating if process is a daemon. */
    int     ipver;              /**< IP version. */
    char    *confile;           /**< Config file location. */
    char    *pidfile;           /**< Pidfile location. */

    /** \todo Allow dubpd to run over multiple interfaces. */
    unsigned int if_index;      /**< Index of the hardware interface running DUBP. */
    char *if_name;              /**< Name of the hardware interface running DUBP. */
    
    int sockfd;                 /**< Socket descriptor running DUBP. */
    struct nl_addr *saddr_nl;
    struct sockaddr *saddr;     /**< Primary address assigned to interface \a if_name. */
    uint8_t saddrlen;           /**< Size of address \a saddr (bytes). */

    struct nl_addr *maddr_nl;
    struct sockaddr *maddr;     /**< Multicast address used for hello messaging. */
    uint8_t maddrlen;           /**< Size of address \a maddr (bytes). */

    /* hello thread data */   
    pthread_t hello_writer_tid; /**< ID of the hello message writing thread. */
    pthread_t hello_reader_tid; /**< ID of the hello message reader thread. */
    uint16_t hello_seqno;       /**< Last sequence used in a transmitted hello message. */

    /* timers */
    uint8_t hello_interval;     /**< Time period between hello messages (seconds). */
    uint8_t neighbor_timeout;   /**< Time period (seconds). */

    uint32_t release_rate;      /**< Time period between releasing packets (mseconds). */
    uint32_t update_rate;       /**< Time period between updating next hop routes (mseconds). */
   
    /* commodity table */
    list_t clist;               /**< Commodity list. */
    /** \todo Determine if a mutex is needed for the commodity list. */
    pthread_t backlogger_tid;   /**< ID of the backlogger thread. */
    pthread_t router_tid;       /**< ID of the router thread. */

    /* neighbor table */
    neighbortable_t ntable;     /**< Neighbor table. */

} dubp_t;

extern dubp_t dubpd;


#endif /* __DUBP_H */
