#include "ntable.h"

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <stdint.h>

#include "dubp.h"
#include "logger.h"


void ntable_init() {

    pthread_mutex_t *mutex = &dubpd.ntable.mutex;
    neighborhead_t *nhead = &dubpd.ntable.nhead;
    uint8_t *nsize = &dubpd.ntable.nsize;

    if (pthread_mutex_init(mutex, NULL) < 0) {
        DUBP_LOG_ERR("Unable to intialize ntable mutex");   
    }

    if (pthread_mutex_lock(mutex) < 0) {
        DUBP_LOG_ERR("Unable to lock ntable mutex");
    }

    LIST_INIT(nhead);
    *nsize = 0;

    if (pthread_mutex_unlock(mutex) < 0) {
        DUBP_LOG_ERR("Unable to unlock ntable mutex");
    }
}


void ntable_destroy() {

    pthread_mutex_t *mutex = &dubpd.ntable.mutex;

    /* TODO: destroy/free rest of table */
    
    if (pthread_mutex_destroy(mutex) < 0) {
        DUBP_LOG_ERR("Unable to destroy ntable mutex");
    }  

}
