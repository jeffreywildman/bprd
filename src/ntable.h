#ifndef __NTABLE_H
#define __NTABLE_H

#include <stdint.h>
#include <sys/queue.h>
#include <sys/types.h>


/* commodity list definitions */
typedef LIST_HEAD(commodityhead, commodity) commodityhead_t;
typedef struct commodity {
    uint8_t address;    /* address of the commodity */
    uint8_t backlog;    /* backlog associated with the commodity */
    LIST_ENTRY(commodity) commodities;
} commodity_t;

/* neighbor list definitions */
typedef LIST_HEAD(neighborhead, neighbor) neighborhead_t;
typedef struct neighbor {
    uint8_t address;        /* address of the neighbor */
    uint8_t bidir;          /* boolean integer indicating a bidirectional link to neighbor */
    commodityhead_t chead;  /* commodity list of neighbor */
    uint8_t size;           /* number of commodities at neighbor */
    LIST_ENTRY(neighbor) neighbors;
} neighbor_t;

/* neighbor table definitions */
typedef struct neighbor_table {
    neighborhead_t nhead;   /* neighbor list */
    uint8_t size;           /* number of neighbors */
    pthread_mutex_t mutex;
} neighbor_table_t;


extern void ntable_init();
extern void ntable_destroy();


#endif /* __NTABLE_H */
