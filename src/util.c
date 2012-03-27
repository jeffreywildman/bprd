#include "util.h"

#include <stdio.h>
#include <string.h>

#include <netlink/errno.h>

#include "dubp.h"


#define MAX_ERROR_MSG_SIZE 256
extern char *program;


/* thread-safe customized error printing */
void print_error(const char *func, char *msg) {

    fprintf(stderr, "[%s] %s(): Error", program, func);
    if (msg != NULL) {
        fprintf(stderr, ": %s", msg);
    }
    fprintf(stderr, "\n");
}


/* thread-safe customized error printing */
void print_error2(const char *func, char *msg, int err) {

    char errbuf[MAX_ERROR_MSG_SIZE];
    
    fprintf(stderr, "[%s] %s(): Error", program, func);
    if (msg != NULL) {
        fprintf(stderr, ": %s", msg);
    }
    if (strerror_r(err, errbuf, MAX_ERROR_MSG_SIZE) == 0) {
        fprintf(stderr, ": %s", errbuf);
    }
    fprintf(stderr, "\n");
}


void mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
    int i, l;

    l = 0;
    for (i = 0; i < ETH_ALEN ; i++) {
        if (i == 0) {
            sprintf(mac_addr+l, "%02x", arg[i]);
            l += 2;
        } else {
            sprintf(mac_addr+l, ":%02x", arg[i]);
            l += 3;
        }
    }
}


int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
    int i;

    for (i = 0; i < ETH_ALEN ; i++) {
        int temp;
        char *cp = strchr(arg, ':');
        if (cp) {
            *cp = 0;
            cp++;
        }
        if (sscanf(arg, "%x", &temp) != 1)
            return -1;
        if (temp < 0 || temp > 255)
            return -1;

        mac_addr[i] = temp;
        if (!cp)
            break;
        arg = cp;
    }
    if (i < ETH_ALEN - 1)
        return -1;

    return 0;
}
