#include "ntable.h"

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <stdint.h>

#include "logger.h"


void ntable_init() {

    neighbor_table_t *ntable = &dubpd.ntable;

    if (pthread_mutex_init(&ntable->mutex, NULL) < 0) {
        DUBP_LOG_ERR("Unable to intialize ntable mutex");   
    }

    if (pthread_mutex_lock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");
    }

    LIST_INIT(&ntable->nhead);
    ntable->size = 0;

    if (pthread_mutex_unlock(&ntable->mutex) < 0) {
        DUBP_LOG_ERR("Unable to unlock ntable mutex");
    }
}


void ntable_destroy() {

    neighbor_table_t *ntable = &dubpd.ntable;

    /* TODO: destroy/free rest of table */
    
    if (pthread_mutex_destroy(&ntable.mutex) < 0) {
        DUBP_LOG_ERR("Unable to destroy ntable mutex");
    }  

}
