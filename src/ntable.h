#ifndef __NTABLE_H
#define __NTABLE_H

#include <stdint.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <common/netaddr.h>

#include "fifo_queue.h"

typedef struct netaddr netaddr_t;
typedef union netaddr_socket netaddr_socket_t;
typedef struct netaddr_str netaddr_str_t;


/* TODO: move list functions into separate file */
/* list definitions */
typedef LIST_HEAD(list, elm) list_t;
typedef struct elm {
    void *data;
    LIST_ENTRY(elm) elms;
} elm_t;

/* generic list functions */
extern void list_init(list_t *l);
extern void list_insert(list_t *l, void *data);
extern void list_free(list_t *l, void (del_data)(void *));
extern elm_t *list_find(list_t *l, void *data, int (*cmp_data)(void *, void *));
extern void list_remove_cond(list_t *l, int (*cond_data)(void *), void (*del_data)(void *));


/* commodity definition */
typedef struct commodity_short {
        netaddr_t addr;     /* destination address of the commodity */
        uint8_t backlog;    /* backlog associated with the commodity */
} commodity_s_t;
typedef struct commodity {
    commodity_s_t cdata;
    uint16_t nfq_id;        /* NFQUEUE id associated with this commodity */
    fifo_t *queue;          /* queue holding packets of this commodity */
} commodity_t;

/* neighbor definition */
typedef struct neighbor {
    netaddr_t addr;         /* address of the neighbor */
    uint8_t bidir;          /* boolean integer indicating a bidirectional link to neighbor */
    time_t update_time;     /* time last updated */ 
    list_t clist;           /* commodity list of neighbor */
} neighbor_t;

/* neighbor table definitions */
typedef struct neighbortable {
    list_t nlist;      /* neighbor list */
    pthread_mutex_t mutex;
} neighbortable_t;


/* type specific list functions */
extern void clist_free(list_t *l);
extern void nlist_free(list_t *l);
extern commodity_t *clist_find(list_t *l, commodity_t *c);
extern neighbor_t *nlist_find(list_t *l, neighbor_t *n);

extern void ntable_refresh(neighbortable_t *ntable);
extern void ntable_mutex_init(neighbortable_t *ntable);
extern void ntable_mutex_lock(neighbortable_t *ntable); 
extern void ntable_mutex_unlock(neighbortable_t *ntable);
extern void ntable_print(neighbortable_t *ntable);

#endif /* __NTABLE_H */
