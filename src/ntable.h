#ifndef __NTABLE_H
#define __NTABLE_H

#include <stdint.h>             /* for uint*_t */
#include <sys/types.h>          /* for pthread_mutex_t */

#include "list.h"

typedef struct netaddr netaddr_t;
typedef union netaddr_socket netaddr_socket_t;
typedef struct netaddr_str netaddr_str_t;

typedef struct neighbortable {
    list_t nlist;
    pthread_mutex_t mutex;
} neighbortable_t;

extern void ntable_refresh(neighbortable_t *ntable);
extern void ntable_mutex_init(neighbortable_t *ntable);
extern void ntable_mutex_lock(neighbortable_t *ntable); 
extern void ntable_mutex_unlock(neighbortable_t *ntable);
extern void ntable_print(neighbortable_t *ntable);

#endif /* __NTABLE_H */
