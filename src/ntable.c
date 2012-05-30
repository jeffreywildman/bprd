#include "ntable.h"

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#include "dubp.h"
#include "logger.h"


/* free memory associated with a commodity element 
   e - element containing a commodity
 */
static void del_data_c(void *data) {

    assert(data);

    commodity_t *c = (commodity_t *)data;
    free(c);
}


/* commodity type-specific free list */
void clist_free(list_t *l) {

    list_free(l, del_data_c);
}


/* free memory associated with a neighbor element
   e - element containing a neighbor entry
 */
static void del_data_n(void *data) {

    assert(data);

    neighbor_t *n = (neighbor_t *)data;
    list_free(&n->clist, del_data_c);
    free(n);
}


/* neighbor type-specific free list */
void nlist_free(list_t *l) {
    
    list_free(l, del_data_n);
}


/* compare commodity elements
   e1 - element 1
   e2 - element 2
 */
static int cmp_data_c(void *data1, void *data2) {

    assert(data1 && data2);

    commodity_t *c1 = (commodity_t *)data1;
    commodity_t *c2 = (commodity_t *)data2;

    return netaddr_cmp(&c1->cdata.addr, &c2->cdata.addr); 
}


/* commodity type-specific find */
commodity_t *clist_find(list_t *l, commodity_t *c) {

    elm_t *e = list_find(l, (void *)c, cmp_data_c);

    return e ? (commodity_t *)e->data : NULL;
}


/* compare neighbor elements
   e1 - element 1
   e2 - element 2
 */
static int cmp_data_n(void *data1, void *data2) {

    assert(data1 && data2);

    neighbor_t *n1 = (neighbor_t *)data1;
    neighbor_t *n2 = (neighbor_t *)data2;

    return netaddr_cmp(&n1->addr, &n2->addr); 
}


/* neighbor type-specific find*/
neighbor_t *nlist_find(list_t *l, neighbor_t *n) {

    elm_t *e = list_find(l, (void *)n, cmp_data_n);

    return e ? (neighbor_t *)e->data : NULL;
}


/* condition that neighbor has gone stale */
static int cond_n_expired(void *data) {

    assert(data);

    neighbor_t *n = (neighbor_t *)data;
    return (time(NULL) - n->update_time > dubpd.neighbor_timeout);
}


/* remove any entries with update_time too stale
TODO: allow timeout to be fractions of a second
 */
void ntable_refresh(neighbortable_t *ntable) {

    assert(ntable);

    list_remove_cond(&ntable->nlist, cond_n_expired, del_data_n);
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


#include <stdio.h>
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
            printf("\t\tDest: %s \t Backlog: %u\n", netaddr_to_string(&naddr_str, &c->cdata.addr), c->cdata.backlog);
        }
        printf("\n");
    }
}
