#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pidfile.h"
#include "logger.h"


/* find max length of pid in ASCII form, plus one for newline */
/* log10(2^(8*sizeof(pid_t))) < 8*sizeof(pid_t)*log10(2) < 3*sizeof(pid_t) */
static const int maxpidstrlen = 3*sizeof(pid_t)+1;

void pidfile_create(const char *pathname) {

    int buflen, pidfd;
    char buf[maxpidstrlen];

    if ((pidfd = open(pathname, O_EXCL | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        DUBP_LOG_ERR("Unable to create pidfile, it may already exist");
    }
    
    buflen = sprintf(buf, "%jd\n", (intmax_t)getpid());
    if (write(pidfd, buf, buflen) < 0) {
        DUBP_LOG_ERR("Error writing to pidfile");
    }
    close(pidfd);
}

void pidfile_destroy(const char *pathname) {

    if (unlink(pathname) < 0) {
        DUBP_LOG_ERR("Unable to remove pidfile");
    }
}
