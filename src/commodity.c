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
 * \defgroup commodity Commodity
 * \{
 */

#include "commodity.h"

#include <assert.h>             /* for assert() */
#include <stdlib.h>             /* for free() */

#include "common/netaddr.h"     /* for netaddr_cmp() */

#include "list.h"


/**
 * \struct commodity_short
 * Data structure containing commodity fields essential for sharing.
 * \var commodity_short::addr
 * Destination address of the commodity.
 * \var commodity_short::backlog
 * Backlog associated with the commodity.
 */


/**
 * \struct commodity
 * Data structure containing full definition of a commodity.
 * \var commodity::cdata
 * Essential commodity fields encapsulated in a data structure.
 * \var commodity::backdiff
 * Backlog differential of commodity.
 * \var commodity::nfq_id
 * NFQUEUE ID associated with this commodity (\see backlogger)
 * \var commodity::queue
 * Queue holding packets of this commodity (\see fifo_queue)
 */


/**
 * Free memory associated with a commodity.
 * 
 * \param data Commodity to be freed.
 */
static void del_data_c(void *data) {

    assert(data);

    commodity_t *c = (commodity_t *)data;
    free(c);
}


/**
 * Commodity type-specific free list.
 * \see list_free
 *
 * \param l List whose elements are to be freed. 
 */
void clist_free(list_t *l) {

    list_free(l, del_data_c);
}


/**
 * Compare commodities.
 *
 * Comparison is done based on address assigned to each commodity.
 *
 * \param data1 Commodity 1.
 * \param data2 Commodity 2.
 *
 * \retval >0 If \a data1>data2.
 * \retval <0 If \a data1<data2.
 * \retval 0 Otherwise.
 */
static int cmp_data_c(void *data1, void *data2) {

    assert(data1 && data2);

    commodity_t *c1 = (commodity_t *)data1;
    commodity_t *c2 = (commodity_t *)data2;

    return netaddr_cmp(&c1->cdata.addr, &c2->cdata.addr); 
}


/**
 * Commodity type-specific find.
 * \see list_find
 *
 * \param l List whose elements are to be searched. 
 * \param c Commodity to compare against.
 *
 * \returns A reference to a matching commodity if found.
 * \retval NULL If no matching commodity found.
 */
commodity_t *clist_find(list_t *l, commodity_t *c) {

    elm_t *e = list_find(l, (void *)c, cmp_data_c);

    return e ? (commodity_t *)e->data : NULL;
}

/** \} */
