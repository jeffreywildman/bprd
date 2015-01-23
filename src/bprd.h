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

#ifndef __BPRD_H
#define __BPRD_H

#include <stdint.h>
#include <sys/socket.h>

#include <netlink/addr.h>

#include "ntable.h"

/* RFC 5498 - IANA Allocations for Mobile Ad Hoc Network (MANET) Protocols */
#define MANET_LINKLOCAL_ROUTERS_V4 "224.0.0.109"
#define MANET_LINKLOCAL_ROUTERS_V6 "FF02::6D"
#define IPPROTO_MANET 138  /* if running over SOCK_RAW */
#define IPPORT_MANET 269   /* if running over SOCK_DGRAM */

#define BPRD_DEFAULT_INTERFACE "eth0"

#define USEC_PER_MSEC 1000                  /* useconds/msecond */
#define BPRD_DEFAULT_HELLO_INTERVAL 100     /* mseconds */
#define BPRD_DEFAULT_RELEASE_INTERVAL 100   /* mseconds */
#define BPRD_DEFAULT_UPDATE_INTERVAL 100    /* mseconds */
#define BPRD_DEFAULT_NEIGHBOR_TIMEOUT 5     /* # of missed hello messages */

/**< \todo Move this into a config.h. */
#define BPRD_DEFAULT_PIDLEN 25
#define BPRD_DEFAULT_PIDSTR "/var/run/bprd.pid"
#define BPRD_DEFAULT_CONLEN 25
/**< \todo Move this into a config.h. */
#define BPRD_DEFAULT_CONSTR "/etc/bprd.conf"

#define BPRD_MSG_TYPE_HELLO 1

#define BPRD_MSGTLV_TYPE_COM 1
#define BPRD_MSGTLV_TYPE_COMKEY 2
#define BPRD_MSGTLV_TYPE_BACKLOG 3


/** 
 * \struct bprd
 * Data structure defining a BPRD process.
 */
typedef struct bprd {
    char    *program;           /**< Program name. */
    int     dmode;              /**< Boolean integer indicating if process is a daemon. */
    int     ipver;              /**< IP version. */
    char    *confile;           /**< Config file location. */
    char    *pidfile;           /**< Pidfile location. */

    /** \todo Allow bprd to run over multiple interfaces. */
    unsigned int if_index;      /**< Index of the hardware interface running BPRD. */
    char *if_name;              /**< Name of the hardware interface running BPRD. */
    
    int sockfd;                 /**< Socket descriptor running BPRD. */
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
    uint32_t hello_interval;    /**< Time period between hello messages (useconds). */
    uint32_t release_interval;  /**< Time period between releasing packets (useconds). */
    uint32_t update_interval;   /**< Time period between updating next hop routes (useconds). */
    uint32_t neighbor_timeout;   /**< Time period (useconds). */
   
    /* commodity table */
    list_t clist;               /**< Commodity list. */
    /** \todo Determine if a mutex is needed for the commodity list. */
    pthread_t backlogger_tid;   /**< ID of the backlogger thread. */
    pthread_t router_tid;       /**< ID of the router thread. */

    /* neighbor table */
    neighbortable_t ntable;     /**< Neighbor table. */

} bprd_t;

extern bprd_t bprd;


#endif /* __BPRD_H */
