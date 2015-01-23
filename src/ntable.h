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
