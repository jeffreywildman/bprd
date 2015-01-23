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

/**
 * \defgroup daemonizer Daemonizer
 * This module handles the conversion of a BPRD process into a daemon.  
 * \{
 */

#include "daemonizer.h"

#include <signal.h>         /* for signal(), sigemptyset(), sigaction() */
#include <stdlib.h>         /* for exit() */
#include <sys/stat.h>       /* for umask() */
#include <unistd.h>         /* for fork(), setsid(), chdir(), close() */

#include "bprd.h"
#include "logger.h"
#include "pidfile.h"



/**
 * Handle when SIGTERM is sent to the daemonized process.
 *
 * \param signum Unused. 
 */
static void daemon_handler_sigterm(int signum __attribute__ ((unused))) {

    if (pidfile_destroy(bprd.pidfile) < 0) {
        BPRD_LOG_ERR("Unable to destroy pidfile");        
    }
    exit(1);
}


/**
 * Create a daemon out of the current process.
 *
 * \retval 0 On success.
 * \retval -1 On error.
 */
int daemon_create() {

    pid_t pid, sid;

    if ((pid = fork()) < 0) {
        BPRD_LOG_ERR("Unable to create interim daemon process");
    } else if (pid > 0) {
        /* this is the parent process */
        exit(0);
    } 
    /* this is the interim daemon child process */

    /* make child process a group leader */
    if ((sid = setsid()) < 0) {
        BPRD_LOG_ERR("Unable to make interim daemon a process group leader");
    }

    /* ignore signal hang up */
    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) < 0) {
        BPRD_LOG_ERR("Unable to create daemon process");
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
    if (pidfile_create(bprd.pidfile) < 0) {
        BPRD_LOG_ERR("Unable to create pidfile");
    }

    /* install our SIGTERM handler */
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        BPRD_LOG_ERR("Unable to set up SIGTERM handler");
        if (pidfile_destroy(bprd.pidfile) < 0) {
            BPRD_LOG_ERR("Unable to destroy pidfile");   
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

/** \} */
