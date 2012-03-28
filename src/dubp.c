/*
 * Drexel University Backpressure Daemon
 */

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

#include "dubp.h"
#include "pidfile.h"
#include "logger.h"
#include "hello.h"

int dubp_debug = 1;
char *program;

/* TODO: move this into a config.h */
static const char *pidfile = "/var/run/dubpd.pid";


static void usage() {
    printf("Usage:\t%s\n",program);
}


static void daemon_handler_sigterm(int signum __attribute__ ((unused))) {

    /* TODO: abstract actual pidfile location */
    pidfile_destroy(pidfile);
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

    /* TODO: move the pid file location into config.h */
    /* TODO: attempt to create lock on /var/run/pid file before creating daemon */
    pidfile_create(pidfile);
   
    /* install our SIGTERM handler */
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        DUBP_LOG_ERR("Unable to set up SIGTERM handler");
        /* TODO: abstract actual pidfile location */
        pidfile_destroy(pidfile);
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
static int socket_init() {

    int sockfd;
    struct sockaddr_in saddr;
    struct ip_mreqn maddr;

    /* create socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        DUBP_LOG_ERR("Unable to create socket");
    }

    /* construct binding address */
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(IPPORT_MANET);

    /* bind address to socket */
    if (bind(sockfd, (const struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        DUBP_LOG_ERR("Unable to bind socket");
    }

    /* specify default interface for outgoing multicast messages */
    memset(&maddr.imr_multiaddr, 0, sizeof(maddr.imr_multiaddr));
    memset(&maddr.imr_address, 0, sizeof(maddr.imr_address));
    if (inet_pton(AF_INET, MANET_LINKLOCAL_ROUTERS_V4, &maddr.imr_multiaddr) <= 0) {
        DUBP_LOG_ERR("Unable to convert MANET link local address");
    }
    if ((maddr.imr_ifindex = if_nametoindex(DUBP_INTERFACE)) <= 0) {
        DUBP_LOG_ERR("Unable to convert device name to index");
    }
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &maddr, sizeof(maddr)) < 0) {
        DUBP_LOG_ERR("Unable to set outgoing multicast interface");
    }

    /* TODO: determine if DONTROUTE is needed here if we have already specified outgoing interface for outgoing multicast */
    /* specify outgoing messages to skip routing */
    setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, (int)0, sizeof(int));

    return sockfd;
}


int main(int argc, char **argv) {

    int sockfd;
    struct sockaddr_in maddr; 
    hellomsg_t hello;
    uint32_t i = 0;
    ssize_t n;

    program = argv[0];

    usage();

    /* initialize logging */
    log_init();
    DUBP_LOG_DBG("testing message!");
    
    /* switch over to daemon process */
    daemon_init();

    /* start up socket */
    sockfd = socket_init();

    /* construct MANET address to send to */
    memset(&maddr, 0, sizeof(maddr));
    maddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, MANET_LINKLOCAL_ROUTERS_V4, &maddr.sin_addr.s_addr) <= 0) {
        DUBP_LOG_ERR("Unable to convert MANET link local address");
    }
    maddr.sin_port = htons(IPPORT_MANET);

    /* construct hello message */
    hello.type = 0xFF;
    hello.metric = htonl(0x12345678);

    while (1) {
        
        hello.seqnum = htonl(i);

        if ((n = sendto(sockfd, &hello, sizeof(hellomsg_t), 0, (const struct sockaddr *)&maddr, sizeof(maddr))) < 0) {
            DUBP_LOG_ERR("Unable to send hello!");
        } else {
            DUBP_LOG_DBG("Sent hello message to " MANET_LINKLOCAL_ROUTERS_V4);
        }

        sleep(1);

        i++;
    }


    /* close socket and get out of here */
    close(sockfd);

    DUBP_LOG_DBG("testing message only /var/log/syslog!");
    
    log_cleanup();

    return 0;
}
