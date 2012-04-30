#include <pthread.h>
#include <unistd.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

#include "dubp.h"
#include "fifo_queue.h"
#include "logger.h"
#include "ntable.h"

static struct nfq_handle *h;

static void backlogger_init() {

    /* PRECONDITION: all commodities have been initialized and exist in dubpd.clist */
    /* PRECONDITION: each commodity_t element in dubpd.clist has 'uint32_t nfq_id' set and 'fifo_t *queue == NULL' */

    /* initialization of backlogger thread */
    /* open connection to libnetfilter */
    /* set up necessary queues to track commodities */
    elm_t *e;
    commodity_t *c;

    //TODO: Assume setpriorty() has been called already

    /* Opening netfilter_queue library handle */
    h = nfq_open();
    if (!h) {
        DUBP_LOG_ERR("error during nfq_open()");
    }

    /* Unbind existing nf_queue handler for AF_INET (if any) */
    /* TODO: extend to IPv6 handling */
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        DUBP_LOG_ERR("error during nfq_unbind_pf()");
    }

    /* Bind nfnetlink_queue as nf_queue handler for AF_INET */
    /* TODO: extend to IPv6 handling */
    if (nfq_bind_pf(h, AF_INET) < 0) {
        DUBP_LOG_ERR("error during nfq_bind_pf()");
    }

    /* iterate through list looking for matching element */
    for (e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = e->data;
        c->queue = (fifo_t *)malloc(sizeof(fifo_t));
        fifo_init(c->queue);

        /* Bind this socket to queue c->id */
        c->queue->qh = nfq_create_queue(h, c->nfq_id, &fifo_add_packet, c->queue);
        if (!c->queue->qh) {
            DUBP_LOG_ERR("error during nfq_create_queue()");
        }

        /* Set packet copy mode to NFQNL_COPY_META */
        if (nfq_set_mode(c->queue->qh, NFQNL_COPY_META, 0xffff) < 0) {
            DUBP_LOG_ERR("can't set packet_copy mode");
        }
    } // for

    /* POSTCONDITION: each commodity_t element in dubpd.clist has a valid fifo_t *queue that can be used in function 
       calls to your library */
}


/* loop endlessly and handle commodity packets */
static void *backlogger_thread(void *arg __attribute__((unused)) ) {

    backlogger_init();
    int fd, rv;
    /* TODO: correct way to allocate buffer? */
    char buf[4096] __attribute__ ((aligned));

    fd = nfq_fd(h);

    while ((rv = recv(fd, buf, sizeof(buf),0)) && rv >=0) {
        /* main backlogger loop */
        nfq_handle_packet(h, buf, rv);
    }
    //TODO: Clean up?

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
