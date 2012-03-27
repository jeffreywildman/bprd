/*
 * Drexel University Backpressure Routing Daemon
 */

#include <stdio.h>

#include "dubp.h"

int monsta_debug = 1;
char *program;

static void usage() {
    printf("Usage:\t%s\n",program);
}

int main(int argc, char **argv) {

    program = argv[0];

    usage();

    return 0;
}
