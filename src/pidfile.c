#include "pidfile.h"

#include <fcntl.h>      /* for open(), */
#include <stdint.h>     /* for intmax_t */
#include <stdio.h>      /* for sprintf() */
#include <sys/stat.h>   /* for open() */
#include <sys/types.h>  /* for pid_t, getpid() */
#include <unistd.h>     /* for unlink(), getpid(), write(), close() */


/**
 * Stores the max length of a PID represented in ASCII form, plus one for newline.
 * \note log10(2^(8*sizeof(pid_t))) < 8*sizeof(pid_t)*log10(2) < 3*sizeof(pid_t)
 */
static const int maxpidstrlen = 3*sizeof(pid_t)+1;


/**
 * Create pidfile with PID inside.
 *
 * \param pathname File to create, must not already exist.
 *
 * \return Returns 0 on success, otherwise -1 on error.
 */
int pidfile_create(const char *pathname) {

    int buflen, pidfd;
    char buf[maxpidstrlen];

    if ((pidfd = open(pathname, O_EXCL | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        return -1;
    }
    
    buflen = sprintf(buf, "%jd\n", (intmax_t)getpid());
    if (write(pidfd, buf, buflen) < 0) {
        return -1;
    }
    if (close(pidfd) < 0) {
        return -1;   
    }

    return 0;
}


/**
 * Destroy pidfile.
 *
 * \todo Fully consider how dangerous this function could be in the wrong hands.
 *
 * \param pathname File to destroy.
 *
 * \return Returns 0 on success, otherwise -1 on error.
 */
int pidfile_destroy(const char *pathname) {

    if (unlink(pathname) < 0) {
        return -1;
    } 
    
    return 0;
}
