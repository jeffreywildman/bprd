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
 * \defgroup backlogger Backlogger
 * This module manages tracking commodity levels for the BPRD process.
 * \{
 */

#include <pthread.h>        /* for pthread_create() */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>      /* for LIST_*() */
#include <unistd.h>

#include <libnetfilter_queue/libnetfilter_queue.h>  /* for nfq_*() */

#include "commodity.h"
#include "bprd.h"
#include "fifo_queue.h"
#include "logger.h"


static struct nfq_handle *h;    /**< Handle to netfilter queue library. */


/**
 * Release packets back to kernel.
 *
 * Find the commodity with the largest backlog differential and send \a count packets from it.
 *
 * \param count Number of packets to release.
 */ 
void backlogger_packet_release(unsigned int count) {

    elm_t *e;
    commodity_t *c = NULL;
    commodity_t *ctemp;
    uint32_t diffopt = 0;

    /* search for largest max differential */
    for (e = LIST_FIRST(&bprd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        ctemp = (commodity_t *)e->data;

        if (ctemp->backdiff > diffopt) {
            c = ctemp;
            diffopt = ctemp->backdiff;
        }
    }

    if (c) {
        /* only send up to min(count,(diffopt+1)/2) packets! otherwise gradient will grow in the reverse direction */
        count = (diffopt+1)/2 > count ? count : (diffopt+1)/2;
        /* release up to count packets of this commodity */
        while (count--) {fifo_send_packet(c->queue);}
    }
}


/**
 * Initialize the backlogger thread.
 *
 * Open connection to libnetfilter, link each commodity with a netfilter queue, and add iptables rules to filter 
 * commodities into their respective netfilter queues.
 *
 * \pre All commodities have been initialized and exist in bprd.clist.  Each commodity_t element in bprd.clist has 
 * 'uint32_t nfq_id' set and 'fifo_t *queue == NULL'
 *
 * \post Each commodity_t element in bprd.clist has a valid fifo_t *queue that can be used in calls to functions in
 * fifo_queue.h
 */
static void backlogger_init() {

    elm_t *e;
    commodity_t *c;

    /** \todo determine if setpriorty() must be called to improve performance */

    /* opening netfilter_queue library handle */
    h = nfq_open();
    if (!h) {
        BPRD_LOG_ERR("error during nfq_open()");
    }

    /* unbind existing nf_queue handler for AF_INET (if any) */
    /** \todo extend to IPv6 handling */
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        BPRD_LOG_ERR("Error during nfq_unbind_pf()");
    }

    /* bind nfnetlink_queue as nf_queue handler for AF_INET */
    /** \todo extend to IPv6 handling */
    if (nfq_bind_pf(h, AF_INET) < 0) {
        BPRD_LOG_ERR("Error during nfq_bind_pf()");
    }

    /* iterate through list looking for matching element */
    for (e = LIST_FIRST(&bprd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = (commodity_t *)e->data;
        c->queue = (fifo_t *)malloc(sizeof(fifo_t));
        fifo_init(c->queue);

        /* bind this socket to queue c->nfq_id */
        c->queue->qh = nfq_create_queue(h, c->nfq_id, &fifo_add_packet, c->queue);
        if (!c->queue->qh) {
            BPRD_LOG_ERR("Error during nfq_create_queue()");
        }

        /* set packet copy mode to NFQNL_COPY_META */
        if (nfq_set_mode(c->queue->qh, NFQNL_COPY_META, 0xffff) < 0) {
            BPRD_LOG_ERR("Can't set packet_copy mode");
        }
    }

    /* flush iptables */
    if (system("/sbin/iptables -t raw -F") < 0) {
        BPRD_LOG_ERR("Unable to flush iptables");   
    }

    int n;

    char cmd[256];
    netaddr_str_t naddr_str;
    struct netaddr naddr;
    union netaddr_socket nsaddr;
   
    /* convert my address into a netaddr for easy comparison */
    nsaddr.std = *bprd.saddr; 
    netaddr_from_socket(&naddr, &nsaddr);

    /* set iptables for each commodity that is not destined to me */
    for (e = LIST_FIRST(&bprd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = (commodity_t *)e->data;

        /* skip commodity destined to me! */
        if (netaddr_cmp(&naddr, &(c->cdata.addr)) == 0) {
            continue;
        }
   
        n = snprintf(cmd, 256, "/sbin/iptables -t raw -A OUTPUT -d %s -j NFQUEUE --queue-num %d", netaddr_to_string(&naddr_str, &c->cdata.addr), c->nfq_id);
        if (n < 0 || n == 256) {
            BPRD_LOG_ERR("Unable to construct system string");
        }
        system(cmd);

        n = snprintf(cmd, 256, "/sbin/iptables -t raw -A PREROUTING -d %s -j NFQUEUE --queue-num %d", netaddr_to_string(&naddr_str, &c->cdata.addr), c->nfq_id);
        if (n < 0 || n == 256) {
            BPRD_LOG_ERR("Unable to construct system string");
        }
        system(cmd);
    }
}


/**
 * Loop endlessly and handle commodity packets.
 *
 * \param arg Unused.
 */
static void *backlogger_thread_main(void *arg __attribute__((unused)) ) {

    backlogger_init();

    int fd, rv;
    /** \todo determine if this is the correct way to allocate buffer */
    char buf[4096] __attribute__ ((aligned));

    fd = nfq_fd(h);

    while ((rv = recv(fd, buf, sizeof(buf),0)) && rv >=0) {
        /* main backlogger loop */
        nfq_handle_packet(h, buf, rv);
        //BPRD_LOG_DBG("Handling Packet!");
    }
    /** \todo clean up if while loop breaks? */

    return NULL;
}


/**
 * Create a new thread to handle continuous backlogger duties.
 */
void backlogger_thread_create() {

    /** \todo Check out pthread_attr options, currently set to NULL */
    if (pthread_create(&(bprd.backlogger_tid), NULL, backlogger_thread_main, NULL) < 0) {
        BPRD_LOG_ERR("Unable to create backlogger thread");
    }

    /** \todo wait here until process stops? pthread_join(htdata.tid)? */
    /** \todo handle the case where hello writer thread stops  - encapsulate in while(1) - restart hello thread? */

}

/** \} */
