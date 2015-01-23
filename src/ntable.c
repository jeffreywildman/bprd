/**
 * The BackPressure Routing Daemon (bprd).
 *
 * Copyright (c) 2012 Jeffrey Wildman <jeffrey.wildman@gmail.com>
 * Copyright (c) 2012 Bradford Boyle <bradford.d.boyle@gmail.com>
 *
 * bprd is released under the MIT License.  You should have received
 * a copy of the MIT License with this program.  If not, see
 * <http://opensource.org/licenses/MIT>.
 */

/**
 * \defgroup ntable Neighbor Table
 * \{
 */

#include "ntable.h"

#include <assert.h>         /* for assert() */
#include <pthread.h>        /* for pthread_mutex_*() */ 
#include <sys/time.h>       /* for timeval, gettimeofday() */

#include "bprd.h"
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

    struct timeval now;
    uint32_t elapsed;

    assert(data);

    neighbor_t *n = (neighbor_t *)data;

    /** \todo error handling */
    gettimeofday(&now, NULL);

    elapsed = (now.tv_sec - n->update_time.tv_sec) * 1000000;
    elapsed += (now.tv_usec - n->update_time.tv_usec);
    return (elapsed > bprd.neighbor_timeout);
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
        BPRD_LOG_ERR("Unable to intialize ntable mutex");   
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
        BPRD_LOG_ERR("Unable to lock ntable mutex");   
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
        BPRD_LOG_ERR("Unable to lock ntable mutex");   
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
        printf("\tUpdate Time: %s", asctime(localtime(&n->update_time.tv_sec)));
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
