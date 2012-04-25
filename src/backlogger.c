#include <pthread.h>
#include <unistd.h>


#include "dubp.h"
#include "logger.h"


static void backlogger_init() {
    
    /* initialization of backlogger thread */

    /* open connection to libnetfilter */
    /* set up necessary queues to track commodities */
    /* make sure references to queue are associated with property commodity_t element in dubpd.clist */

}


/* loop endlessly and send hello messages */
static void *backlogger_thread(void *arg __attribute__((unused)) ) {

    backlogger_init();

    while (1) {

        /* main backlogger loop */

        /* TODO: insert blocking recv statement to catch packets added to NFQUEUEs that we are tracking */
        DUBP_LOG_DBG("Backlogger is waiting to be implemented...");
        /* for now, just sleep */
        sleep(dubpd.hello_interval);
    }

    return NULL;
}


void backlogger_thread_create() {

    /* TODO: check out pthread_attr options, currently set to NULL */
    if (pthread_create(&(dubpd.backlogger_tid), NULL, backlogger_thread, NULL) < 0) {
        DUBP_LOG_ERR("Unable to create backlogger thread");
    }

    /* TODO: wait here until process stops? pthread_join(htdata.tid)? */
    /* TODO: handle the case where hello writer thread stops  - encapsulate in while(1) - restart hello thread? */

}
