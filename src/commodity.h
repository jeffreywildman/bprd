#ifndef __COMMODITY_H
#define __COMMODITY_H

#include <stdint.h>             /* for uint*_t */

#include <common/netaddr.h>     /* for struct netaddr */

#include "fifo_queue.h"
#include "list.h"

typedef struct commodity_short {
        struct netaddr addr;
        uint32_t backlog;
} commodity_s_t;

typedef struct commodity {
    commodity_s_t cdata;
    uint32_t backdiff;
    uint16_t nfq_id;
    fifo_t *queue;
} commodity_t;

extern void clist_free(list_t *l);
extern commodity_t *clist_find(list_t *l, commodity_t *c);
extern void clist_print(list_t *l);

#endif /* __COMMODITY_H */
