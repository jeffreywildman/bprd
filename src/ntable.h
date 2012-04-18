#ifndef __NTABLE_H
#define __NTABLE_H

#include <stdint.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <common/netaddr.h>
typedef struct netaddr netaddr_t;

/* commodity list definitions */
typedef LIST_HEAD(commodityhead, commodity) commodityhead_t;
typedef struct commodity {
    netaddr_t addr;         /* destination address of the commodity */
    uint8_t backlog;        /* backlog associated with the commodity */
    LIST_ENTRY(commodity) commodities;
} commodity_t;

/* neighbor list definitions */
typedef LIST_HEAD(neighborhead, neighbor) neighborhead_t;
typedef struct neighbor {
    netaddr_t addr;         /* address of the neighbor */
    uint8_t bidir;          /* boolean integer indicating a bidirectional link to neighbor */
    time_t update_time;     /* time last updated */ 
    commodityhead_t chead;  /* commodity list of neighbor */
    LIST_ENTRY(neighbor) neighbors;
} neighbor_t;

/* neighbor table definitions */
typedef struct neighbortable {
    neighborhead_t nhead;   /* neighbor list */
    pthread_mutex_t mutex;
} neighbortable_t;

extern void clist_init(commodityhead_t *chead);
extern void clist_free(commodityhead_t *chead);
extern commodity_t *clist_find(commodityhead_t *chead, netaddr_t *caddr);
extern void clist_insert(commodityhead_t *chead, commodity_t *c);

extern void nlist_init(neighborhead_t *nhead);
extern void nlist_free(neighborhead_t *nhead);
extern neighbor_t *nlist_find(neighborhead_t *nhead, netaddr_t *naddr);
extern void nlist_insert(neighborhead_t *nhead, neighbor_t *n);
extern void nlist_refresh(neighborhead_t *nhead);

extern void ntable_mutex_init(neighbortable_t *ntable);
extern void ntable_mutex_lock(neighbortable_t *ntable); 
extern void ntable_mutex_unlock(neighbortable_t *ntable);


#endif /* __NTABLE_H */
