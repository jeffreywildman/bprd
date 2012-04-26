#include <pthread.h>
#include <unistd.h>
#include <sys/queue.h>
#include <stdio.h>

#include "dubp.h"
#include "logger.h"
#include "ntable.h"


static void backlogger_init() {

    /* PRECONDITION: all commodities have been initialized and exist in dubpd.clist */
    /* PRECONDITION: each commodity_t element in dubpd.clist has 'uint32_t nfq_id' set and 'fifo_t *queue == NULL' */

    /* initialization of backlogger thread */
    /* open connection to libnetfilter */
    /* set up necessary queues to track commodities */
   
    /* sample iteration through dubpd.clist */
    elm_t *e;
    commodity_t *c;

    /* iterate through list looking for matching element */
    for (e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = e->data;
        printf("This is the NFQUEUE id: %u\n", c->nfq_id);
        //c->queue will be fifo_t * set to NULL;
    }

    /* POSTCONDITION: each commodity_t element in dubpd.clist has a valid fifo_t *queue that can be used in function 
       calls to your library */
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
