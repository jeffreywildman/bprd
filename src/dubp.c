/*
 * Drexel University Backpressure Daemon
 */

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dubp.h"
#include "pidfile.h"
#include "logger.h"

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

    int err;
    int sockfd;
    struct sockaddr_in saddr;

    /* create socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        DUBP_LOG_ERR("Unable to create socket");
    }

    /* construct binding address */ 
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* clients may reach server through any valid address */
    saddr.sin_port = htons(IPPORT_MANET);

    /* bind address to socket */
    if ((err = bind(sockfd, (const struct sockaddr *)&saddr, sizeof(saddr))) < 0) {
        DUBP_LOG_ERR("Unable to bind socket");
    }

    return sockfd;
}



int main(int argc, char **argv) {

    int sockfd;

    program = argv[0];

    usage();

    /* initialize logging */
    log_init();
    DUBP_LOG_DBG("testing message!");
    
    /* switch over to daemon process */
    daemon_init();

    /* start up socket */
    sockfd = socket_init();

    /* close socket and get out of here */
    close(sockfd);

    DUBP_LOG_DBG("testing message only /var/log/syslog!");
    
    log_cleanup();

    return 0;
}
