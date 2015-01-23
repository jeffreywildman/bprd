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
 * \defgroup pidfile Pidfile
 * This module provides an API for creating and destroying a PID file associated with the running process.
 * \{
 */

#include "pidfile.h"

#include <fcntl.h>      /* for open() */
#include <limits.h>     /* for PATH_MAX */
#include <stdint.h>     /* for intmax_t */
#include <stdio.h>      /* for snprintf() */
#include <sys/stat.h>   /* for open() */
#include <sys/types.h>  /* for pid_t, getpid() */
#include <unistd.h>     /* for unlink(), getpid(), write(), close() */


static char pidfile_pathname[PATH_MAX];  /**< Location of the pidfile. */

/**
 * Stores the max length of a PID represented in ASCII form, plus one for newline.
 * \f[\log_{10}(2^{(8*\textrm{sizeof}(pid\_t))}) < 8*\textrm{sizeof}(pid\_t)*\log_{10}(2) < 3*\textrm{sizeof}(pid\_t)\f]
 */
static const int maxpidstrlen = 3*sizeof(pid_t)+1;


/**
 * Create pidfile with PID inside.
 *
 * \param pathname File to create, must not already exist.
 *
 * \retval 0 On success.
 * \retval -1 On error.
 */
int pidfile_create(const char *pathname) {

    int buflen, pidfd;
    char buf[maxpidstrlen];

    if ((pidfd = open(pathname, O_EXCL | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        return -1;
    }
    
    buflen = snprintf(buf, maxpidstrlen, "%jd\n", (intmax_t)getpid());
    if (write(pidfd, buf, buflen) < 0) {
        return -1;
    }
    if (close(pidfd) < 0) {
        return -1;
    }

    buflen = snprintf(pidfile_pathname, PATH_MAX, "%s", pathname);
    if (buflen == PATH_MAX) {
        return -1;
    }

    return 0;
}


/**
 * Destroy pidfile.
 *
 * \retval 0 On success.
 * \retval -1 On error.
 */
int pidfile_destroy() {

    if (unlink(pidfile_pathname) < 0) {
        return -1;
    }

    return 0;
}

/** \} */
