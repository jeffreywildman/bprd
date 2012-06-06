/**
 * \defgroup ntable Neighbor Table
 * \{
 */

#include "ntable.h"

#include <assert.h>         /* for assert() */
#include <pthread.h>        /* for pthread_mutex_*() */ 
#include <time.h>           /* for time() */
#include <sys/types.h>      /* for time_t */

#include "dubp.h"
#include "logger.h"
#include "commodity.h"
#include "neighbor.h"


/**
 * \struct neighbortable
 * \var neighbortable::nlist
 * List of neighbors.
 * \var neighbortable::mutex
 * Mutex lock for neighbor table.
 */


/**
 * Condition that neighbor has gone stale
 *
 * \param data Neighbor to evaluate.
 */
static int cond_n_expired(void *data) {

    assert(data);

    neighbor_t *n = (neighbor_t *)data;
    return (time(NULL) - n->update_time > dubpd.neighbor_timeout);
}


/**
 * Remove any entries with update_time too stale.
 *
 * \todo Allow timeout to be fractions of a second.
 *
 * \param ntable Neighbor table to be refreshed.
 */
void ntable_refresh(neighbortable_t *ntable) {

    assert(ntable);

    nlist_remove_cond(&ntable->nlist, cond_n_expired);
}


/**
 * Initialize the mutex on a neighbor table.
 *
 * \param ntable Neighbor table to initialize.
 */
void ntable_mutex_init(neighbortable_t *ntable) {

    assert(ntable);

    if (pthread_mutex_init(&ntable->mutex, NULL) < 0) {
        DUBP_LOG_ERR("Unable to intialize ntable mutex");   
    }
}

/**
 * Lock the mutex on a neighbor table.
 *
 * This function blocks until lock is obtained.
 *
 * \param ntable Neighbor table to lock.
 */
void ntable_mutex_lock(neighbortable_t *ntable) {

    assert(ntable);

    if (pthread_mutex_lock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");   
    }
}


/**
 * Unlock the mutex on a neighbor table.
 *
 * \param ntable Neighbor table to unlock.
 */
void ntable_mutex_unlock(neighbortable_t *ntable) {

    assert(ntable);

    if (pthread_mutex_unlock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");   
    }
}


#include <stdio.h>
#include <sys/queue.h>
/**
 * Print out a neighbor table.
 *
 * \param ntable Neighbor table to print.
 */
void ntable_print(neighbortable_t *ntable) {

    elm_t *e,*f;
    time_t t;
    neighbor_t *n;
    commodity_t *c;
    netaddr_str_t naddr_str;

    assert(ntable);
   
    t = time(NULL);
    printf("Neighbor Table, Current Time: %s\n", asctime(localtime(&t)));
    /* iterate through list looking for matching element */
    LIST_EMPTY(&ntable->nlist) ? printf("\tNONE\n") : 0;
    for (e = LIST_FIRST(&ntable->nlist); e != NULL; e = LIST_NEXT(e, elms)) {
        assert(e->data);
        n = (neighbor_t *)e->data;
        printf("\tAddress: %s\n", netaddr_to_string(&naddr_str, &n->addr));
        printf("\tBidir: %u\n", n->bidir);
        printf("\tUpdate Time: %s", asctime(localtime(&n->update_time)));
        printf("\tCommodities:");
        LIST_EMPTY(&n->clist) ? printf(" NONE\n") : printf("\n");
        for (f = LIST_FIRST(&n->clist); f != NULL; f = LIST_NEXT(f, elms)) {
            assert(f->data);
            c = (commodity_t *)f->data;
            printf("\t\tDest: %s \t Backlog: %u \t Differential: %u\n", netaddr_to_string(&naddr_str, &c->cdata.addr), c->cdata.backlog, c->backdiff);
        }
        printf("\n");
    }
}

/** \} */
