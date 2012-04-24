/*
 * Drexel University Backpressure Daemon
 */

#include "dubp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <common/netaddr.h>

#include "pidfile.h"
#include "logger.h"
#include "hello.h"
#include "ntable.h"
#include "util.h"

/* pre-initialization of runtime variables */
dubp_t dubpd = {
.program = NULL,
.dmode = 0,
.ipver = AF_INET,
.confile = NULL,
.pidfile = NULL,
.sockfd = -1,
.if_name = NULL,
.saddr = NULL,
.saddrlen = 0,
.maddr = NULL,
.maddrlen = 0
};


static void usage() {
    printf("\nUsage:\t%s [OPTION]...\n",dubpd.program);
    printf("Start the backpressure routing protocol with OPTIONs.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("  -c, --config=FILE     \tread configuration parameters from FILE\n");
    printf("  -d                    \trun the program as a daemon\n");
    printf(" -v4, --v4              \trun the protocol using IPv4 (default)\n");
    printf(" -v6, --v6              \trun the protocol using IPv6\n");
    printf("  -i, --interface=IFACE \trun the protocol over interface IFACE (default is eth0)\n");
    printf("  -p, --pidfile=FILE    \tset pid file to FILE (default is /var/run/dubpd.pid)\n");
}


static void daemon_handler_sigterm(int signum __attribute__ ((unused))) {

    if (pidfile_destroy(dubpd.pidfile) < 0) {
        DUBP_LOG_ERR("Unable to destroy pidfile");        
    }
    exit(1);
}


static int daemon_init() {
    
    pid_t pid, sid;

    if ((pid = fork()) < 0) {
        DUBP_LOG_ERR("Unable to create interim daemon process");
    } else if (pid > 0) {
        /* this is the parent process */
        exit(0);
    } 
    /* this is the interim daemon child process */

    /* make child process a group leader */
    if ((sid = setsid()) < 0) {
        DUBP_LOG_ERR("Unable to make interim daemon a process group leader");
    }

    /* ignore signal hang up */
    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) < 0) {
        DUBP_LOG_ERR("Unable to create daemon process");
    } else if (pid > 0) {
        /* this is the interim daemon process */
        exit(0);
    }
    /* this is the daemon process */

    /* set file mode mask */
    umask(0);

    /* get our SIGTERM handler ready */
    struct sigaction sa;
    sa.sa_handler = daemon_handler_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /* TODO: attempt to create lock on /var/run/pid file before creating daemon */
    if (pidfile_create(dubpd.pidfile) < 0) {
        DUBP_LOG_ERR("Unable to create pidfile");
    }
   
    /* install our SIGTERM handler */
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        DUBP_LOG_ERR("Unable to set up SIGTERM handler");
        if (pidfile_destroy(dubpd.pidfile) < 0) {
            DUBP_LOG_ERR("Unable to destroy pidfile");   
        }
    }

    /* change current directory to root */
    chdir("/");

    /* close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}


/* initialize socket for hello messages */
/* TODO: protocol v4/v6 independent */
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
 
}



/* initialize dubp instance */
/* precedence of options, i) command-line, ii) config file, iii) in-code defaults */
void dubp_init(int argc, char **argv) {

    int c;

    dubpd.program = argv[0];

    int lo_index;
    static struct option config_option[] = {
        {"config", required_argument, NULL, 'c'},
        {0,0,0,0}
    };
    static struct option long_options[] = {
        /* these options set a flag */
        {"d", no_argument, &dubpd.dmode, 1},
        {"v4", no_argument, &dubpd.ipver, AF_INET},
        {"v6", no_argument, &dubpd.ipver, AF_INET6},
        /* these options do not set a flag */
        {"config", required_argument, NULL, 'c'},
        {"interface", required_argument, NULL, 'i'},
        {"pidfile", required_argument, NULL, 'p'},
        {0,0,0,0}
    };

    opterr = 0;
    
    /* first check for config file within arguments */
    while ((c = getopt_long_only(argc, argv, "-", config_option, &lo_index)) != -1) {
        if (c == 'c') {
            printf("config: %s\n", optarg);
            dubpd.confile = optarg;
            /* TODO: process configuration file here, before all other options */
            /* confile_read(); */
        }
    }
    
    /* reset optind to reparse command-line args */
    optind = 0;
    opterr = 1;

    /* iterate through arguments again, optind is now first argument not recognized by original pass */
    while ((c = getopt_long_only(argc, argv, "", long_options, &lo_index)) != -1) {
        switch (c) {
        case 0:
            if (long_options[lo_index].flag != 0) {
                printf("%s: %d\n", long_options[lo_index].name, long_options[lo_index].val);
            }
            break;
        case 'c':
            /* ignore the config file this time around! */
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

    /* set hardware interface name */
    if (!dubpd.if_name) {
        if (!(dubpd.if_name = (char *)malloc(IF_NAMESIZE*sizeof(char)))) {
            DUBP_LOG_ERR("Unable to allocate memory");
        }
        if (snprintf(dubpd.if_name, IF_NAMESIZE*sizeof(char), "%s", DUBP_DEFAULT_INTERFACE) < 0) {
            DUBP_LOG_ERR("Unable to set default interface string");
        }
    }

    /* set interface address */
    if (!dubpd.saddr) {

        /* get my current address on the above hardware interface */
        struct ifaddrs *iflist, *ifhead;
        if (getifaddrs(&iflist) < 0) {
            DUBP_LOG_ERR("Unable to get interface addresses");
        }

        ifhead = iflist;

        while(iflist) {
            if (iflist->ifa_name && strcmp(iflist->ifa_name,dubpd.if_name) == 0) {
                unsigned int family = iflist->ifa_addr->sa_family;
                if (iflist->ifa_addr && family == (uint8_t)dubpd.ipver) {
                    /* TODO: validate or remove assumption that first IPv4/v6 address on the desired interface is the correct one! */
                    if ((uint8_t)dubpd.ipver == AF_INET) {
                        dubpd.saddr = (sockaddr_t *)malloc(sizeof(sockaddr_in_t));
                        dubpd.saddrlen = sizeof(sockaddr_in_t);
                    } else if ((uint8_t)dubpd.ipver == AF_INET6) {
                        dubpd.saddr = (sockaddr_t *)malloc(sizeof(sockaddr_in6_t));
                        dubpd.saddrlen = sizeof(sockaddr_in6_t);
                    } else {DUBP_LOG_ERR("Unknown IP version");}
                    memcpy(dubpd.saddr,(const sockaddr_in_t *)iflist->ifa_addr,dubpd.saddrlen);
                    break;
                }
            }
            iflist = iflist->ifa_next;
        }
        freeifaddrs(ifhead);
    }
    if (!dubpd.saddr) {
        DUBP_LOG_ERR("Unable to find pre-existing IP address of the desired version on the desired interface");
    }

    /* set multicast address for hello messages */
    /* TODO: remove assumption of IPv4 address */
    struct sockaddr_in *maddr;
    if(!(maddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in)))) {
        DUBP_LOG_ERR("Unable to allocate memory");
    }
    memset(maddr, 0, sizeof(struct sockaddr_in));
    maddr->sin_family = AF_INET;
    if (inet_pton(AF_INET, MANET_LINKLOCAL_ROUTERS_V4, &maddr->sin_addr.s_addr) <= 0) {
        DUBP_LOG_ERR("Unable to convert MANET link local address");
    }
    maddr->sin_port = htons(IPPORT_MANET);

    dubpd.maddr = (struct sockaddr *)maddr;
    dubpd.maddrlen = sizeof(*maddr);

    /* timers */
    dubpd.hello_interval = DUBP_DEFAULT_HELLO_INTERVAL;
    dubpd.neighbor_timeout = dubpd.hello_interval * DUBP_DEFAULT_NEIGHBOR_TIMEOUT;

    dubpd.hello_seqno = 0;

    list_init(&dubpd.clist);
    /* TODO: initialize my commodity list */
    /* TODO: link in with Bradford's code here to initialize */
    /* TODO: remove these test commodities */
    commodity_t *com = (commodity_t *)malloc(sizeof(commodity_t));
    if (netaddr_from_string(&com->addr, "192.168.1.200/24") < 0) {
        DUBP_LOG_ERR("Unable to convert string to address");
    }
    com->backlog = 0x77;
    list_insert(&dubpd.clist, com);
    com = (commodity_t *)malloc(sizeof(commodity_t));
    if (netaddr_from_string(&com->addr, "192.168.1.201/24") < 0) {
        DUBP_LOG_ERR("Unable to convert string to address");
    }
    com->backlog = 0x78;
    list_insert(&dubpd.clist, com);

    /* initialize my neighbor table */
    ntable_mutex_init(&dubpd.ntable);
    ntable_mutex_lock(&dubpd.ntable);
    list_init(&dubpd.ntable.nlist);
    
    /* TODO: remove these test neighbors */
    /* neighbor 1, no commodity */
    neighbor_t *n = (neighbor_t *)malloc(sizeof(neighbor_t));
    if (netaddr_from_string(&n->addr, "192.168.1.220/24") < 0) {
        DUBP_LOG_ERR("Unable to convert string address");
    }
    n->bidir = 0;
    n->update_time = time(NULL);
    list_init(&n->clist);
    list_insert(&dubpd.ntable.nlist, n);

    /* neighbor 2, one commodity */
    n = (neighbor_t *)malloc(sizeof(neighbor_t));
    if (netaddr_from_string(&n->addr, "192.168.1.221/24") < 0) {
        DUBP_LOG_ERR("Unable to convert string address");
    }
    n->bidir = 0;
    n->update_time = time(NULL);
    list_init(&n->clist);
    com = (commodity_t *)malloc(sizeof(commodity_t));
    if (netaddr_from_string(&com->addr, "192.168.1.200/24") < 0) {
        DUBP_LOG_ERR("Unable to convert string to address");
    }
    com->backlog = 0xAA;
    list_insert(&n->clist, com);
    list_insert(&dubpd.ntable.nlist, n);

    ntable_mutex_unlock(&dubpd.ntable);
}


int main(int argc, char **argv) {

    /* initialize logging */
    log_init();

    /* set instance parameters */
    dubp_init(argc, argv);

    /* switch over to daemon process */
    if (dubpd.dmode) {daemon_init();}

    /* start up socket */
    socket_init();

    /* start the hello threads */
    hello_reader_thread_create();
    hello_writer_thread_create();

    /* just hang out here for a while */
    /* TODO: this 'thread' will periodically poll commodity data, 
       update backlogs, set routes, release data packets to kernel */
    while(1) {

        sleep(1);
    }

    /* close socket and get out of here */
    close(dubpd.sockfd);

    /* cleanup logging */
    log_cleanup();

    return 0;
}
