#include "hello.h"

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include <packetbb/pbb_reader.h>
#include <common/netaddr.h>

#include "dubp.h"
#include "logger.h"
#include "ntable.h"


static struct pbb_reader pbb_r;
static struct pbb_reader_tlvblock_consumer pbb_pkt_cons, pbb_msg_cons, pbb_addr_cons;


void hello_recv(uint8_t *buf, size_t buflen) {

    /* lock neighbor table while processing message */
    ntable_mutex_lock(&dubpd.ntable);
    pbb_reader_handle_packet(&pbb_r, buf, buflen);
    ntable_mutex_unlock(&dubpd.ntable);
}


static neighbor_t *n = NULL;

static enum pbb_result hello_cons_msg_start (struct pbb_reader_tlvblock_consumer *c __attribute__ ((unused)), 
                                          struct pbb_reader_tlvblock_context *context) {
    assert (context->type == PBB_CONTEXT_MESSAGE);

    /* TODO: softer error handling */
    assert (context->msg_type == DUBP_MSG_TYPE_HELLO);
    assert (context->has_origaddr);

    /* grab source address and access neighbor table */
    neighbor_t ntemp;
    netaddr_from_binary(&ntemp.addr, context->orig_addr, context->addr_len, dubpd.ipver);

    /* TODO: ignore my own hello messages! */
    
    /* find existing neighbor with matching address or create new one */
    n = nlist_find(&dubpd.ntable.nlist, &ntemp);
    if (n == NULL) {
        n = (neighbor_t *)malloc(sizeof(neighbor_t));
        netaddr_from_binary(&n->addr, context->orig_addr, context->addr_len, dubpd.ipver);
        n->bidir = 0;
        list_init(&n->clist);
        list_insert(&dubpd.ntable.nlist, n);
    }
    n->update_time = time(NULL);

    return PBB_OKAY;
}


#include <stdio.h>
static void print_hexline(void *buffer, size_t length) {
    size_t i;
    uint8_t *buf = buffer;

    for (i=0; i<length; i++) {
        printf("%s%02x", ((i&3) == 0) ? " " : "", (int)(buf[i]));
    }
}
static size_t min_sizet(size_t a, size_t b) {
    return a < b ? a : b;
}
static void printhex(const char *prefix, void *buffer, size_t length) {
    size_t j;
    uint8_t *buf = buffer;

    for (j=0; j<length; j+=32) {
        printf("%s%04zx:", prefix, j);

        print_hexline(&buf[j], min_sizet(length-j, 32));
        printf("\n");
    }
}


static enum pbb_result hello_cons_msg_tlv(struct pbb_reader_tlvblock_consumer *c __attribute__ ((unused)),
                                          struct pbb_reader_tlvblock_entry *tlv,
                                          struct pbb_reader_tlvblock_context *context) {
    assert (context->type == PBB_CONTEXT_MESSAGE);

    commodity_t *com, *comtemp;

    /* read in commodities for the neighbor */
    if (tlv->type == DUBP_MSGTLV_TYPE_COM && tlv->length == sizeof(commodity_t)) {
        comtemp = (commodity_t *)tlv->single_value;
        /* try to find commodity in neighbor commodity list or create new one */
        com = clist_find(&n->clist, comtemp);
        if (com == NULL) {
            com = (commodity_t *)malloc(sizeof(commodity_t));
            com->addr = comtemp->addr;
            list_insert(&n->clist, com);
        }
        com->backlog = comtemp->backlog;
    } else {
        DUBP_LOG_ERR("Unrecognized TLV parameters");
    }

    return PBB_OKAY;
}



void hello_reader_init() {

    pbb_reader_init(&pbb_r);

    /* we don't care about the packet */
    pbb_reader_add_packet_consumer(&pbb_r, &pbb_pkt_cons, NULL, 0, 0);
    pbb_pkt_cons.start_callback = NULL;
    pbb_pkt_cons.tlv_callback = NULL;

    /* hello message consumer */
    pbb_reader_add_message_consumer(&pbb_r, &pbb_msg_cons, NULL, 0, DUBP_MSG_TYPE_HELLO, 0);
    pbb_msg_cons.start_callback = hello_cons_msg_start;
    pbb_msg_cons.tlv_callback = hello_cons_msg_tlv;

    /* hello message address consumer */
    pbb_reader_add_address_consumer(&pbb_r, &pbb_addr_cons, NULL, 0, DUBP_MSG_TYPE_HELLO, 0);
    pbb_addr_cons.start_callback = NULL;
    pbb_addr_cons.tlv_callback = NULL;

}


/* loop endlessly and recv hello messages */
static void *hello_reader_thread(void *arg __attribute__((unused)) ) {

    static uint8_t buf[512]; /* TODO: link to MTU size */
    ssize_t buflen;
    struct sockaddr saddr; 
    socklen_t saddr_len;

    hello_reader_init();

    while (1) {

        /* block while reading from socket */
        if ((buflen = recvfrom(dubpd.sockfd, (void *)&buf, sizeof(buf), 0, &saddr, &saddr_len)) < 0) {
            DUBP_LOG_ERR("Unable to receive hello!");
        } else {
            DUBP_LOG_DBG("Received hello message");
            /* TODO: confirm that packet came from correct MCAST protocol, addr, and port */
            hello_recv(buf, buflen);
        }
    }

    return NULL;
}


void hello_reader_thread_create() {

    /* TODO: check out pthread_attr options, currently set to NULL */
    if (pthread_create(&(dubpd.hello_reader_tid), NULL, hello_reader_thread, NULL) < 0) {
        DUBP_LOG_ERR("Unable to create hello thread");
    }

    /* TODO: wait here until process stops? pthread_join(htdata.tid)? */
    /* TODO: handle the case where hello reader thread stops  - encapsulate in while(1) - restart hello thread? */

}
