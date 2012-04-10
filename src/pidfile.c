#include "pidfile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/* find max length of pid in ASCII form, plus one for newline */
/* log10(2^(8*sizeof(pid_t))) < 8*sizeof(pid_t)*log10(2) < 3*sizeof(pid_t) */
static const int maxpidstrlen = 3*sizeof(pid_t)+1;

/* create pidfile at provided located, return -1 on error with errno set, return 0 on success */
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


/* destroy pidfile at provided located, return -1 on error and with errno set, return 0 on success */
/* TODO: really consider how dangerous this function could be */
int pidfile_destroy(const char *pathname) {

    if (unlink(pathname) < 0) {
        return -1;
    } 
    
    return 0;
}
