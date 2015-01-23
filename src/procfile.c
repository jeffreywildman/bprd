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
 * \defgroup procfile ProcFile
 * \{
 */

#include "procfile.h"

#include <fcntl.h>      /* for open() */
#include <sys/stat.h>   /* for open() */
#include <unistd.h>     /* for read(), lseek(), write(), close() */


/**
 * Write a value to a file in /proc.
 *
 * \param procfile The file in the /proc filesystem to write to.
 * \param oldval Stores the old value in \a procfile.
 * \param newval New value to be written to \a procfile.
 *
 * \retval 0 On success.
 * \retval -1 On error.
 */
int procfile_write(const char *procfile, char *oldval, char newval) {
 
    int fd;

    /* open procfile */
    if ((fd = open(procfile, O_RDWR)) < 0) {
        return -1;
    }

    /* save old value */
    if (oldval) {
        if (read(fd, oldval, 1) != 1) {
            return -1;
        }
        if (lseek(fd, 0, SEEK_SET) != 0) {
            return -1;
        }
    }

    /* write new value */
    if (write(fd, &newval, 1) != 1) {
        return -1;
    }

    /* close procfile */
    if (close(fd) < 0) {
        return -1;
    }

    return 0;
}

/** \} */
