/*
 * Drexel University Backpressure Daemon
 */

/**
 * \defgroup dubp DUBP
 * \{
 */

#include "dubp.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>             /* for exit() */
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>             /* for fork() */

#include <netlink/addr.h>
#include <netlink/socket.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#include <common/netaddr.h>

#include "pidfile.h"
#include "logger.h"

#include "hello.h"
#include "backlogger.h"
#include "daemonizer.h"
#include "ntable.h"
#include "util.h"
#include "commodity.h"
#include "router.h"


/* pre-initialization of runtime variables */
dubp_t dubpd = {
    .program = NULL,
    .dmode = 0,
    .ipver = AF_INET,
    .confile = NULL,
    .pidfile = NULL,
    .sockfd = -1,
    .if_name = NULL,
    .saddr_nl = NULL,
    .saddr = NULL,
    .saddrlen = 0,
    .maddr_nl = NULL,
    .maddr = NULL,
    .maddrlen = 0
};

/* options acted upon immediately before others */
static struct option pre_options[] = {
    {"config", required_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {0,0,0,0}
};
static struct option long_options[] = {
    {"v4", no_argument, NULL, '4'},
    {"v6", no_argument, NULL, '6'},
    {"commodity", required_argument, NULL, 'r'},
    {"config", required_argument, NULL, 'c'},
    {"daemon", no_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {"interface", required_argument, NULL, 'i'},
    {"pidfile", required_argument, NULL, 'p'},
    {0,0,0,0}
};

static void usage() {
    printf("Usage:\t%s [OPTION]...\n",dubpd.program);
    printf("Start the backpressure routing protocol with OPTIONs.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("  -4, --v4              \trun the protocol using IPv4 (default)\n");
    printf("  -6, --v6              \trun the protocol using IPv6\n");
    printf("  -r, --commodity=\"ADDR,ID\"     \tdefine a commodity via command-line\n");
    printf("  -c, --config=FILE     \tread configuration parameters from FILE\n");
    printf("  -d, --daemon          \trun the program as a daemon\n");
    printf("  -h, --help            \tprint this help message\n");
    printf("  -i, --interface=IFACE \trun the protocol over interface IFACE (default is eth0)\n");
    printf("  -p, --pidfile=FILE    \tset pid file to FILE (default is /var/run/dubpd.pid)\n");
}


/* initialize socket for hello messages */
/** \todo Make protocol v4/v6 independent. */
static void socket_init() {

    struct sockaddr_in saddr;
    struct ip_mreqn mreq;

    /* create socket */
    if ((dubpd.sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        DUBP_LOG_ERR("Unable to create socket");
    }

    /* construct binding address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(IPPORT_MANET);

    /* bind address to socket */
    if (bind(dubpd.sockfd, (const struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        DUBP_LOG_ERR("Unable to bind socket");
    }

    /* construct multicast request data structure */ 
    memset(&mreq.imr_multiaddr, 0, sizeof(mreq.imr_multiaddr));
    memset(&mreq.imr_address, 0, sizeof(mreq.imr_address));
    if (inet_pton(AF_INET, MANET_LINKLOCAL_ROUTERS_V4, &mreq.imr_multiaddr) <= 0) {
        DUBP_LOG_ERR("Unable to convert MANET link local address");
    }
    if ((mreq.imr_ifindex = if_nametoindex(dubpd.if_name)) <= 0) {
        DUBP_LOG_ERR("Unable to convert device name to index");
    }

    /* join multicast group on the desired interface */
    /* now we should be able to receive multicast messages on this interface as well */
    if (setsockopt(dubpd.sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        DUBP_LOG_ERR("Unable to join multicast group");
    }

    /* set default interface for outgoing multicast messages */
    if (setsockopt(dubpd.sockfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
        DUBP_LOG_ERR("Unable to set default outgoing multicast interface");
    }

}


/* create a commodity and add to list */
/* char *buf should be of the form "ADDRESS,ID" */
/* PRECONDITION: list is already initialized */
void create_commodity(char *buf) {

    /** \todo Appropriately size addrstr */
    char addrstr[256];
    uint32_t nfq_id;
    commodity_t *c;

    /* extract fields from string */
    if (sscanf(buf, "%[^,],%u\n", addrstr, &nfq_id) != 2) {  /* we want exactly two args processed */
        DUBP_LOG_ERR("Error parsing commodity string");   
    }

    /* create new commodity */
    c = (commodity_t *)malloc(sizeof(commodity_t));
    memset(c, 0, sizeof(*c));
    if (netaddr_from_string(&c->cdata.addr, addrstr) < 0) {
        DUBP_LOG_ERR("Unable to convert string to address");
    }
    /* check for duplicate */
    if (clist_find(&dubpd.clist, c) != NULL) {
        DUBP_LOG_ERR("Duplicate commodity detected");
    }
    c->cdata.backlog = 0;
    c->nfq_id = nfq_id;
    c->queue = NULL;
    list_insert(&dubpd.clist, c);
}


/* configuration file reading, for now supports commodity definitions only! */
/* TODO: support full range of options! */
/* TODO: look at i) libconfig or ii) glibc's key-value file parser */
int confile_read() {

    FILE *confd;
    char buf[256];

    if ((confd = fopen(dubpd.confile, "r")) == NULL) {
        return -1;
    } 

    if (fileno(confd) < 0) {
        DUBP_LOG_ERR("Config file not valid");
    }

    /* parse config file */
    while (fgets(buf, 255, confd)) {
        if (strlen(buf) == 255) {
            DUBP_LOG_ERR("Line in config file too long!");
        }
        if (buf[0] == '#') {
            /* skip comment */
        } else {
            create_commodity(buf);
        }
    };

    if (ferror(confd)) {
        DUBP_LOG_ERR("Error while reading from config file");
    } 

    fclose(confd);

    return 0;
}


/* get the primary address on the desired interface */
void create_primary() {

    struct ifaddrs *iflist, *ifhead;

    if (dubpd.ipver == AF_INET6) {dubpd.saddrlen = sizeof(sockaddr_in6_t);} 
    else {dubpd.saddrlen = sizeof(sockaddr_in_t);}

    if (getifaddrs(&iflist) < 0) {
        DUBP_LOG_ERR("Unable to get interface addresses");
    }

    ifhead = iflist;

    while(iflist) {
        if (iflist->ifa_name && strcmp(iflist->ifa_name,dubpd.if_name) == 0) {
            unsigned int family = iflist->ifa_addr->sa_family;
            if (iflist->ifa_addr && family == (uint8_t)dubpd.ipver) {
                /** \todo validate or remove assumption that first IPv4/v6 address on the desired interface is the correct one! */
                if ((dubpd.saddr = (sockaddr_t *)malloc(dubpd.saddrlen)) == NULL) {
                    DUBP_LOG_ERR("Unable to allocate memory");
                }
                memcpy(dubpd.saddr,(const sockaddr_in_t *)iflist->ifa_addr,dubpd.saddrlen);
                break;
            }
        }
        iflist = iflist->ifa_next;
    }
    freeifaddrs(ifhead);
}


/* set multicast address for sending hello messages */
void create_multicast() {

    void *addr;
    int err;

    if (dubpd.ipver == AF_INET6) {dubpd.maddrlen = sizeof(sockaddr_in6_t);} 
    else {dubpd.maddrlen = sizeof(sockaddr_in_t);}

    if ((dubpd.maddr = (struct sockaddr_t *)malloc(dubpd.maddrlen)) == NULL) {
        DUBP_LOG_ERR("Unable to allocate memory");
    }
    memset(dubpd.maddr, 0, sizeof(dubpd.maddrlen));
    dubpd.maddr->sa_family = dubpd.ipver;

    if (dubpd.ipver == AF_INET6) {
        addr = &(((sockaddr_in6_t *)dubpd.maddr)->sin6_addr);
        err = inet_pton(dubpd.ipver, MANET_LINKLOCAL_ROUTERS_V6, addr);
        ((sockaddr_in6_t *)dubpd.maddr)->sin6_port = htons(IPPORT_MANET);
    } else {
        addr = &(((sockaddr_in_t *)dubpd.maddr)->sin_addr);
        err = inet_pton(dubpd.ipver, MANET_LINKLOCAL_ROUTERS_V4, addr);
        ((sockaddr_in_t *)dubpd.maddr)->sin_port = htons(IPPORT_MANET);
    } 

    if (err <= 0) {
        DUBP_LOG_ERR("Unable to convert MANET link local address");
    }
}


/* initialize dubp instance */
/* precedence of options, i) command-line, ii) config file, iii) in-code defaults */
/* bash_completion of command-line args */
void dubp_init(int argc, char **argv) {

    int c;

    dubpd.program = argv[0];
    /* must be initialized prior to config file read in! */
    /* initialize my commodity list */
    list_init(&dubpd.clist);  
    /* initialize my neighbor table */
    ntable_mutex_init(&dubpd.ntable);
    ntable_mutex_lock(&dubpd.ntable);
    list_init(&dubpd.ntable.nlist);
    ntable_mutex_unlock(&dubpd.ntable);

    int lo_index;
    opterr = 0;

    /* first check for pre-options within arguments */
    while ((c = getopt_long_only(argc, argv, "-c:h", pre_options, &lo_index)) != -1) {
        switch (c) {
        case 'c':
            printf("config file option: %s\n", optarg);
            dubpd.confile = optarg;
            break;
        case 'h':
            usage();
            /* TODO: exit more gracefully */
            exit(0);
            break;
        default:
            break;
        }
    }

    /* set confile location */
    if (!dubpd.confile) {
        if (!(dubpd.confile = (char *)malloc(DUBP_DEFAULT_CONLEN*sizeof(char)))) {
            DUBP_LOG_ERR("Unable to allocate memory");
        }
        if (sprintf(dubpd.confile, "%s", DUBP_DEFAULT_CONSTR) < 0) {
            DUBP_LOG_ERR("Unable to set default confile string");
        }
    }

    /* read from configuration file before parsing remaining command-line args */
    if (confile_read() < 0) {
        DUBP_LOG_DBG("Unable to open configuration file: %s", dubpd.confile);
    }

    /* reset optind to reparse command-line args */
    optind = 0;
    opterr = 1;

    /* iterate through arguments again, optind is now first argument not recognized by original pass */
    while ((c = getopt_long_only(argc, argv, "46r:c:dhi:p:", long_options, &lo_index)) != -1) {
        switch (c) {
        case 0:
            if (long_options[lo_index].flag != 0) {
                printf("%s: %d\n", long_options[lo_index].name, long_options[lo_index].val);
            }
            break;
        case '4':
            printf("v4 option\n");
            dubpd.ipver = AF_INET;
            break;
        case '6':
            printf("v6 option\n");
            dubpd.ipver = AF_INET6;
            break;
        case 'r':
            printf("commodity option: %s\n", optarg);
            create_commodity(optarg);
            break;
        case 'c':
            /* ignore the config file this time around! */
            break;
        case 'd':
            printf("daemon option");
            dubpd.dmode = 1;
            break;
        case 'h':
            /* ignore help this time around! */
            break;
        case 'i':
            printf("interface: %s\n", optarg);
            dubpd.if_name = optarg;
            break;
        case 'p':
            printf("pidfile option: %s\n", optarg);
            dubpd.pidfile = optarg;
            break;
        case '?':
            DUBP_LOG_ERR("Unable to parse input arguments");
            break;
        default:
            DUBP_LOG_ERR("Unable to parse input arguments");
            break;
        }
    }

    /* handle unrecognized options */
    if (optind < argc) {
        printf("Unrecognized options:\n");
        while (optind < argc) {
            printf("Unrecognized option: %s\n", argv[optind++]); 
        }
        usage();
        DUBP_LOG_DBG("Unrecognized options on command-line input");
    }

    /* set remaining parameters to defaults */ 

    /* set pidfile location */
    if (!dubpd.pidfile) {
        if (!(dubpd.pidfile = (char *)malloc(DUBP_DEFAULT_PIDLEN*sizeof(char)))) {
            DUBP_LOG_ERR("Unable to allocate memory");
        }
        if (sprintf(dubpd.pidfile, "%s", DUBP_DEFAULT_PIDSTR) < 0) {
            DUBP_LOG_ERR("Unable to set default pidfile string");
        }
    }

    /* get hardware interface name */
    if (!dubpd.if_name) {
        if (!(dubpd.if_name = (char *)malloc(IF_NAMESIZE*sizeof(char)))) {
            DUBP_LOG_ERR("Unable to allocate memory");
        }
        if (snprintf(dubpd.if_name, IF_NAMESIZE*sizeof(char), "%s", DUBP_DEFAULT_INTERFACE) < 0) {
            DUBP_LOG_ERR("Unable to set default interface string");
        }
    }
    /* get hardware interface index */
    if ((dubpd.if_index = if_nametoindex(dubpd.if_name)) == 0) {
        DUBP_LOG_ERR("Unable to get index of hardware interface: %s", dubpd.if_name);
    }

    /* check ipver */
    if (dubpd.ipver != AF_INET && dubpd.ipver != AF_INET6) {
        DUBP_LOG_ERR("Unknown IP version");
    }

    /* get current address on the hardware interface running DUBP */
    if (!dubpd.saddr) {
        create_primary();
    }
    if (!dubpd.saddr) {
        DUBP_LOG_ERR("Unable to find pre-existing IP address of the desired version on the desired interface: %s", dubpd.if_name);
    }

    /* get multicast address for hello messages */
    if (!dubpd.maddr) {
        create_multicast();
    }
    if (!dubpd.maddr) {
        DUBP_LOG_ERR("Unable to create multicast address");
    }

    /* timers */
    dubpd.hello_interval = DUBP_DEFAULT_HELLO_INTERVAL;
    dubpd.neighbor_timeout = dubpd.hello_interval * DUBP_DEFAULT_NEIGHBOR_TIMEOUT;

    dubpd.hello_seqno = 0;

    /* verify existing commodity list up to this point is of the correct type */
    elm_t *e;
    commodity_t *com;
    for(e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        com = (commodity_t *)e->data;
        if (com->cdata.addr.type != dubpd.ipver) {
            DUBP_LOG_ERR("Commodity destination IP address version does not match program's IP version");      
        }
        /** \todo Verify uniqueness of nfq_id on each commodity. */
    }
}


int main(int argc, char **argv) {

    /* initialize logging */
    logger_init();

    /* set instance parameters */
    dubp_init(argc, argv);

    /* switch over to daemon process */
    if (dubpd.dmode) {daemon_create();}

    /* start up socket */
    socket_init();

    /* start up backlog thread */
    backlogger_thread_create();

    /** \todo Wait until signal from the backlog thread instead of sleeping here! */
    /* if we don't wait and call backlogger_update too quickly, then data structures aren't properly initialized and bad
     * things happen... */
    sleep(1);

    /* start the hello threads */
    hello_reader_thread_create();
    hello_writer_thread_create();

    /* initialize the interface with the routing table */
    if (router_init(dubpd.if_index, dubpd.ipver) < 0) {
        DUBP_LOG_ERR("Unable to initialize router");
    }

    /* just hang out here for a while */
    /** \todo This 'thread' will periodically poll commodity data, 
     * update backlogs, set routes, release data packets to kernel 
     */
    while(1) {

        /** \todo Determine which backlog update scheme is best:
         * i) better if we only update periodically here, or
         * ii) better if we update each time we need the backlog 
         */
        router_update();
        //router_release();

        sleep(1);
    }

    /* close socket and get out of here */
    close(dubpd.sockfd);

    /* cleanup logging */
    logger_cleanup();

    return 0;
}

/** \} */
