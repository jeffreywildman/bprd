/**
 * \defgroup neighbor Neighbor
 * \{
 */

#include "neighbor.h"

#include <assert.h>             /* for assert() */
#include <stdlib.h>             /* for free() */

#include "common/netaddr.h"     /* for netaddr_cmp() */

#include "list.h"
#include "commodity.h"


/**
 * \struct neighbor
 * \var neighbor::addr
 * Address of the neighbor.
 * \var neighbor::bidir
 * Boolean integer indicating a bidirectional link to the neighbor.
 * \var neighbor::update_time
 * Time neighbor information was last updated.
 * \var neighbor::clist
 * The neighbor's commodity list.
 */


/**
 * Free memory associated with a neighbor.
 *
 * \param data Neighbor to be freed.
 */
static void del_data_n(void *data) {

    assert(data);

    neighbor_t *n = (neighbor_t *)data;
    clist_free(&n->clist);
    free(n);
}


/**
 * Neighbor type-specific free list.
 * \see list_free
 *
 * \param l List whose elements are to be freed. 
 */
void nlist_free(list_t *l) {
    
    list_free(l, del_data_n);
}


/**
 * Compare neighbors.
 *
 * Comparison is done based on address assigned to each neighbor.
 *
 * \param data1 Neighbor 1.
 * \param data2 Neighbor 2.
 *
 * \retval >0 If \a data1>data2.
 * \retval <0 If \a data1<data2.
 * \retval 0 Otherwise.
 */
static int cmp_data_n(void *data1, void *data2) {

    assert(data1 && data2);

    neighbor_t *n1 = (neighbor_t *)data1;
    neighbor_t *n2 = (neighbor_t *)data2;

    return netaddr_cmp(&n1->addr, &n2->addr); 
}


/**
 * Neighbor type-specific find.
 * \see list_find
 *
 * \param l List whose elements are to be searched. 
 * \param n Neighbor to compare against.
 *
 * \returns A reference to a matching commodity if found.
 * \retval NULL If no matching commodity found.
 */
neighbor_t *nlist_find(list_t *l, neighbor_t *n) {

    elm_t *e = list_find(l, (void *)n, cmp_data_n);

    return e ? (neighbor_t *)e->data : NULL;
}


/**
 * Neighbor type-specific conditional remove.
 *
 * \param l List whose element are to be tested.
 * \param cond_data Pointer to a function that accepts (void *data) within an element of \a l and returns a 
 * Boolean integer indicating whether or not the data meets a condition.
 */
void nlist_remove_cond(list_t *l, int (*cond_data)(void *)) {

    list_remove_cond(l, cond_data, del_data_n);
}

/** \} */
