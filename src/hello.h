#ifndef __HELLO_H
#define __HELLO_H

#include <sys/types.h>

/* Consider referencing packetbb - RFC 5444 */
typedef struct hellomsg {
    uint8_t     type;
    /* TODO: don't waste 3 bytes here - look at sizeof(hellomsg_t)! */
    uint32_t    seqnum;
    uint32_t    metric;
} hellomsg_t;


#endif /* __HELLO_H */
