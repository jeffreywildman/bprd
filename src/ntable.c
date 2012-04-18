#include "ntable.h"

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#include "dubp.h"
#include "logger.h"


/* initialize commodity list
 */
void clist_init(commodityhead_t *chead) {
    
    assert(chead);
    LIST_INIT(chead);
}



/* free the commodity list
 */
void clist_free(commodityhead_t *chead) {

    while(!LIST_EMPTY(chead)) {
        commodity_t *c = LIST_FIRST(chead);
        LIST_REMOVE(c, commodities);
        free(c);
    }
}


/* search through the commodity list for a commodity entry with address caddr */
/* returns a reference to the neighbor entry if found */
/* returns NULL if neighbor entry not found */
commodity_t *clist_find(commodityhead_t *chead, netaddr_t *caddr) {

    assert(chead && caddr);
    
    commodity_t *c = NULL;
   
    /* iterate through list looking for commodity with netaddr */
    for (c = LIST_FIRST(chead); c != NULL; c = LIST_NEXT(c, commodities)) {
        if (netaddr_cmp(&c->addr, caddr) == 0) {
            return c;
        }
    }

    /* neighbor has not been found */
    return NULL;
}


/* insert the commodity entry into the commodity list */
void clist_insert(commodityhead_t *chead, commodity_t *c) {
    
    assert(chead && c);
    LIST_INSERT_HEAD(chead, c, commodities);
}


/* initialize neighbor list
 */
void nlist_init(neighborhead_t *nhead) {
    
    assert(nhead);
    LIST_INIT(nhead);
}


/* free the neighbor list 
 */
void nlist_free(neighborhead_t *nhead) {

    assert(nhead);

    while(!LIST_EMPTY(nhead)) {
        neighbor_t *n = LIST_FIRST(nhead);
        LIST_REMOVE(n, neighbors);
        clist_free(&n->chead);
        free(n);
    }
}


/* search through the neighbor list for a neighbor entry with address naddr */
/* returns a reference to the neighbor entry if found */
/* returns NULL if neighbor entry not found */
neighbor_t *nlist_find(neighborhead_t *nhead, netaddr_t *naddr) {

    assert(nhead && naddr);
    
    neighbor_t *n = NULL;
   
    /* iterate through list looking for neighbor with netaddr */
    for (n = LIST_FIRST(nhead); n != NULL; n = LIST_NEXT(n, neighbors)) {
        if (netaddr_cmp(&n->addr, naddr) == 0) {
            return n;
        }
    }

    /* neighbor has not been found */
    return NULL;
}


/* insert the neighbor entry into the neighbor list */
void nlist_insert(neighborhead_t *nhead, neighbor_t *n) {
    
    assert(nhead && n);
    LIST_INSERT_HEAD(nhead, n, neighbors);
}


/* remove any entries with update_time too stale
TODO: allow timeout to be fractions of a second
 */
void nlist_refresh(neighborhead_t *nhead) {

    assert(nhead);

    neighbor_t *nprev    = NULL;
    neighbor_t *ncur     = LIST_FIRST(nhead);

    while(ncur) {
        if (time(NULL) - ncur->update_time > dubpd.neighbor_timeout) {
            /* neighbor has gone stale, remove from list and free */
            LIST_REMOVE(ncur, neighbors);
            clist_free(&ncur->chead);
            free(ncur);
            if (!nprev) {
                ncur = LIST_FIRST(nhead);
            } else {
                ncur = LIST_NEXT(nprev, neighbors);
            }
        } else {
            /* neighbor is ok, advance iterators */
            nprev = ncur;
            ncur = LIST_NEXT(ncur, neighbors);
        }
    }
}

void ntable_mutex_init(neighbortable_t *ntable) {

    assert(ntable);

    if (pthread_mutex_init(&ntable->mutex, NULL) < 0) {
        DUBP_LOG_ERR("Unable to intialize ntable mutex");   
    }
}


void ntable_mutex_lock(neighbortable_t *ntable) {
    
    assert(ntable);

    if (pthread_mutex_lock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");   
    }
}


void ntable_mutex_unlock(neighbortable_t *ntable) {

    assert(ntable);

    if (pthread_mutex_unlock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");   
    }
}
