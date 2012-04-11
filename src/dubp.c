/*
 * Drexel University Backpressure Daemon
 */

#include "dubp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "pidfile.h"
#include "logger.h"
#include "hello.h"
#include "ntable.h"


static void usage() {
    printf("Usage:\t%s\n",dubpd.program);
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
    if ((mreq.imr_ifindex = if_nametoindex(dubpd.ifrn_name)) <= 0) {
        DUBP_LOG_ERR("Unable to convert device name to index");
    }

    /* join multicast group on the desired interface */
    /* now we should be able to receive multicast messages on this interface as well */
    if (setsockopt(dubpd.sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        DUBP_LOG_ERR("Unable to join multicast group");
    }
 
}


/* initialize dubp instance */
void dubp_init(int argc, char **argv) {

    dubpd.program = argv[0];

    /* for now just set default parameters */ 
  
    /* set pidfile location */
    if (!(dubpd.pidfile = (char *)malloc(DUBP_DEFAULT_PIDLEN*sizeof(char)))) {
        DUBP_LOG_ERR("Unable to allocate memory");
    }
    if (sprintf(dubpd.pidfile, "%s", DUBP_DEFAULT_PIDSTR) < 0) {
        DUBP_LOG_ERR("Unable to set default pidfile string");
    }

    /* set hardware interface name */
    if (!(dubpd.ifrn_name = (char *)malloc(IF_NAMESIZE*sizeof(char)))) {
        DUBP_LOG_ERR("Unable to allocate memory");
    }
    if (snprintf(dubpd.ifrn_name, IF_NAMESIZE*sizeof(char), "%s", DUBP_DEFAULT_INTERFACE) < 0) {
        DUBP_LOG_ERR("Unable to set default interface string");
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

    dubpd.hello_seqno = 0;

    /* TODO: initialize my commodity list */
    LIST_INIT(&dubpd.chead);
    dubpd.csize = 0;

    /* TODO: initialize my neighbor table - remember mutex lock! */
    ntable_init(&dubpd.ntable);

}


int main(int argc, char **argv) {

    struct sockaddr saddr; 
    socklen_t saddr_len;
    hellomsg_t hello;

    
    /* initialize logging */
    log_init();
    
    /* set instance parameters */
    dubp_init(argc, argv);

    //usage();
    
    /* switch over to daemon process */
    daemon_init();

    /* start up socket */
    socket_init();

    /* start the hello thread */
    hello_thread_create();

    /* just hang out here for a while */
    while(1) {

        /* block while reading from socket */
        /* TODO: use packetbb to read packets! */
        //if (recvfrom(dubpd.sockfd, &hello, sizeof(hello), 0, &saddr, &saddr_len) < 0) {
        //    DUBP_LOG_ERR("Unable to receive hello!");
        //} else {
        //    DUBP_LOG_DBG("Received hello message");
        //}
        
        sleep(1);
    }

    /* close socket and get out of here */
    close(dubpd.sockfd);

    /* cleanup logging */
    log_cleanup();

    return 0;
}
