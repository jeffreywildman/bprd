#ifndef __NEIGHBOR_H
#define __NEIGHBOR_H

#include <stdint.h>             /* for uint*_t */
#include <sys/time.h>           /* for timeval */

#include <common/netaddr.h>     /* for netaddr */

#include "list.h"

typedef struct neighbor {
    struct netaddr addr;        /* address of the neighbor */
    uint8_t bidir;              /* boolean integer indicating a bidirectional link to neighbor */
    struct timeval update_time; /* time last updated */ 
    list_t clist;               /* commodity list of neighbor */
} neighbor_t;

extern void nlist_free(list_t *l);
extern neighbor_t *nlist_find(list_t *l, neighbor_t *n);
extern void nlist_remove_cond(list_t *l, int (*cond_data)(void *));

#endif /* __NEIGHBOR_H */
