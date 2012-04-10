#ifndef __HELLO_H
#define __HELLO_H

#include <stdint.h>

/* Consider referencing packetbb - RFC 5444 */
typedef struct hellomsg {
    uint8_t     type;
    /* TODO: don't waste 3 bytes here - look at sizeof(hellomsg_t)! */
    uint32_t    seqnum;
    uint32_t    metric;
} hellomsg_t;

/* TODO: serialize function for hellomsg */
/* TODO: reverse serialize function for hellomsg */

void hello_thread_create(int sockfd);


#endif /* __HELLO_H */
