#ifndef __LIST_H
#define __LIST_H

#include <sys/queue.h>      /* for LIST_HEAD(), LIST_ENTRY() */

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

#endif /* __LIST_H */
