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
 * \defgroup list List
 * \{
 */

#include "list.h"

#include <assert.h>             /* for assert() */
#include <sys/queue.h>          /* for LIST_*() */
#include <stdlib.h>             /* for free() */
#include <string.h>             /* for memset() */


/**
 * \struct elm 
 * A generic element of a list.
 * \var elm::data 
 * Generic (void *) to arbitrary data.
 */


/** 
 * Initialize a list.
 * 
 * \param l The head of the list to be initialized.
 */
void list_init(list_t *l) {

    assert(l);
    LIST_INIT(l);
}


/**
 * Insert an element at the head of a list.
 *
 * \param l The head of a list.
 * \param data The data to be inserted into \a l.
 */
void list_insert(list_t *l, void *data) {

    assert(l && data);
    elm_t *e = (elm_t *)malloc(sizeof(elm_t));
    memset(e, 0, sizeof(elm_t)); /* initialize all fields of elm_t to zero, otherwise BAD things... */
    e->data = data;
    LIST_INSERT_HEAD(l, e, elms);
}


/** 
 * Free the contents of a list.
 *
 * \param l The head of a list.
 * \param del_data Pointer to a function that accepts (void *data) within an element of \a l and frees it.
 */
void list_free(list_t *l, void (*del_data)(void *)) {

    assert(l && del_data);
    while(!LIST_EMPTY(l)) {
        elm_t *e = LIST_FIRST(l);
        LIST_REMOVE(e, elms);
        del_data(e->data);
        free(e);
    }
}


/**
 * Search a list and return the first matching element.
 *
 * \param l The head of a list.
 * \param data The (void *) data to be compared against.
 * \param cmp_data Pointer to a function that compares \a data and the (void *) data within elements of \a l.
 *
 * \returns A reference to a matching element if found.
 * \retval NULL If no matching element found.
 */
elm_t *list_find(list_t *l, void *data, int (*cmp_data)(void *, void *)) {

    assert(l && data && cmp_data);
   
    elm_t *f;

    /* iterate through list looking for matching element */
    for (f = LIST_FIRST(l); f != NULL; f = LIST_NEXT(f, elms)) {
        if (cmp_data(data, f->data) == 0) {
            return f;
        }
    }

    /* element has not been found */
    return NULL;
}


/**
 * Remove elements that satisfy a condition from a list.
 *
 * \param l The head of a list.
 * \param cond_data Pointer to a function that accepts (void *data) within an element of \a l and returns a 
 * Boolean integer indicating whether or not the data meets a condition.
 * \param del_data Pointer to a function that accepts (void *data) within an element of \a l and frees it.
 */
void list_remove_cond(list_t *l, int (*cond_data)(void *), void (*del_data)(void *)) {
     
    assert(l && cond_data && del_data);

    elm_t *eprev = NULL;
    elm_t *ecur  = LIST_FIRST(l);

    while(ecur) {
        if (cond_data(ecur->data)) {
            /* element meets condition, remove from list and free */
            LIST_REMOVE(ecur, elms);
            del_data(ecur->data);
            free(ecur);
            if (!eprev) {
                ecur = LIST_FIRST(l);
            } else {
                ecur = LIST_NEXT(eprev, elms);
            }
        } else {
            /* element does not meet condition, advance iterators */
            eprev = ecur;
            ecur = LIST_NEXT(ecur, elms);
        }
    }
}

/** \} */
