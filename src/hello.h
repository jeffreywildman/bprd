#ifndef __HELLO_H
#define __HELLO_H

#include <sys/types.h>
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


/* data structure to hold information about one hop neighbors */
typedef struct hop1node {
    uint8_t     address;  /* TODO: upgrade this to an IPv4 address */
    uint8_t     metric;
    uint8_t     bidirectional;  /* boolean integer indicating if this is a bidirectional link */
} hop1node_t;

/* data structure to hold information about two hop neighbors */
typedef struct hop2node {
    uint8_t     address;  /* TODO: upgrade this to an IPv4 address */
} hop2node_t;

/* data structure to hold linked list of hop2nodes */
typedef struct hop2item {
    hop2node_t *h2node;
    struct hop2item *next;
} hop2item_t;

/* data structure to hold linked list of hop1nodes and a linked list of their twohopneighbors */
typedef struct hop1item {
    hop1node_t *h1node;
    hop2item_t *h2list;
    uint32_t size;  /* size of twohoplist */
    struct hop1item *next;
} hop1item_t;

/* data structure to hold neighbor table */
typedef struct neighbor_table {
    hop1item_t *h1list;
    uint32_t size;  /* size of onehoplist */
    pthread_mutex_t mutex;
} neighbor_table_t;

/* global neighbor table */
/* TODO: will need to implement mutex on read/write to this data structure */
neighbor_table_t ntable;

void hello_thread_create(int sockfd);


#endif /* __HELLO_H */
