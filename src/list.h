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

#ifndef __LIST_H
#define __LIST_H

#include <sys/queue.h>      /* for LIST_HEAD(), LIST_ENTRY() */

typedef LIST_HEAD(list, elm) list_t;
typedef struct elm {
    void *data;
    LIST_ENTRY(elm) elms;
} elm_t;

extern void list_init(list_t *l);
extern void list_insert(list_t *l, void *data);
extern void list_free(list_t *l, void (del_data)(void *));
extern elm_t *list_find(list_t *l, void *data, int (*cmp_data)(void *, void *));
extern void list_remove_cond(list_t *l, int (*cond_data)(void *), void (*del_data)(void *));

#endif /* __LIST_H */
