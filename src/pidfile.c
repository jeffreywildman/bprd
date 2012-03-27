#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pidfile.h"
#include "util.h"


/* find max length of pid in ASCII form, plus one for newline */
/* log10(2^(8*sizeof(pid_t))) < 8*sizeof(pid_t)*log10(2) < 3*sizeof(pid_t) */
static const int maxpidstrlen = 3*sizeof(pid_t)+1;

void pidfile_create(const char *pathname) {

    int buflen, pidfd;
    char buf[maxpidstrlen];

    if ((pidfd = open(pathname, O_EXCL | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        print_error2(__func__, "Unable to create pidfile, it may already exist", errno);
        exit(1);
    }
    
    buflen = sprintf(buf, "%jd\n", (intmax_t)getpid());
    if (write(pidfd, buf, buflen) < 0) {
        print_error2(__func__, "Error writing to pidfile", errno);
    }
    close(pidfd);
}

void pidfile_destroy(const char *pathname) {

    if (unlink(pathname) < 0) {
        print_error2(__func__, "Unable to remove pidfile", errno); 
    }
}
