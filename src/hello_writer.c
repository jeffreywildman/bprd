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

#include "hello.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <packetbb/pbb_writer.h>
#include <common/netaddr.h>

#include "bprd.h"
#include "logger.h"
#include "commodity.h"
#include "neighbor.h"

static struct pbb_writer pbb_w;
static struct pbb_writer_interface pbb_iface;
static struct pbb_writer_message *pbb_hello_msgwriter;
static struct pbb_writer_content_provider pbb_cpr;


static void hello_send(struct pbb_writer *w, struct pbb_writer_interface *iface, void *buffer, size_t buflen) {
    
    ssize_t n;
    
    if ((n = sendto(bprd.sockfd, buffer, buflen, 0, (const struct sockaddr *)bprd.maddr, bprd.maddrlen)) < 0) {
        BPRD_LOG_ERR("Unable to send hello!");
    } else {
        //BPRD_LOG_DBG("Sent hello message");
    }
}


static void hello_add_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg) {
    struct netaddr naddr;
    union netaddr_socket snaddr;
    
    pbb_writer_set_msg_header(w, msg, 1, 0, 0, 1);
    
    if (bprd.ipver == AF_INET) {memcpy(&snaddr.v4,bprd.saddr,bprd.saddrlen);}
    else if (bprd.ipver == AF_INET6) {memcpy(&snaddr.v6,bprd.saddr,bprd.saddrlen);} 
    else {BPRD_LOG_ERR("Unrecognized IP version");}
    
    netaddr_from_socket(&naddr,&snaddr);
    pbb_writer_set_msg_originator(w, msg, naddr.addr);
}


static void hello_fin_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg, 
                                 struct pbb_writer_address *first_addr, 
                                 struct pbb_writer_address *last_addr, 
                                 bool not_fragmented) {
    pbb_writer_set_msg_seqno(w, msg, bprd.hello_seqno);
    /* TODO: proper seqno rollover detection and handling */
    bprd.hello_seqno++;
}


static void hello_add_msgtlvs(struct pbb_writer *w, struct pbb_writer_content_provider *provider) {

    uint8_t addr_len;

    if (bprd.ipver == AF_INET) {addr_len = 4;}
    else if (bprd.ipver == AF_INET6) {addr_len = 16;}
    else {BPRD_LOG_ERR("Unrecognized IP version");}

    /* add my commodities to message */
    /* TODO: mutex lock commodity list? */
    elm_t *e;
    commodity_t *c;
    commodity_s_t cdata;
    for (e = LIST_FIRST(&bprd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = (commodity_t *)e->data;
        cdata = c->cdata;
        /* TODO: hton byteorder worries here?! */
        pbb_writer_add_messagetlv(w, BPRD_MSGTLV_TYPE_COM, 0, &cdata, sizeof(commodity_s_t));
        //pbb_writer_add_messagetlv(w, BPRD_MSGTLV_TYPE_COMKEY, 0, &c->addr.addr, addr_len);
        //pbb_writer_add_messagetlv(w, BPRD_MSGTLV_TYPE_BACKLOG, 0, &c->backlog, sizeof(c->backlog));
    }   

}

static void hello_add_addresses(struct pbb_writer *w, struct pbb_writer_content_provider *provider) {

    struct pbb_writer_address *addr;
    
    ntable_mutex_lock(&bprd.ntable);
    /* refresh neighbor list */
    ntable_refresh(&bprd.ntable); 
    /* add my neighbors to message */
    elm_t *e;
    neighbor_t *n;
    for (e = LIST_FIRST(&bprd.ntable.nlist); e != NULL; e = LIST_NEXT(e, elms)) {
        n = (neighbor_t *)e->data;    
        /* TODO: set prefix length correctly */
        /* for now, use whole address */
        addr = pbb_writer_add_address(w, provider->creator, n->addr.addr, n->addr.prefix_len);
    }
    ntable_mutex_unlock(&bprd.ntable);
}


static bool useAllIf(struct pbb_writer *w, struct pbb_writer_interface *iface, void *param) {
    return true;
}


void hello_writer_init() {

    /* initialize packetbb writer */
    size_t mtu = 512;
    uint8_t addr_len;

    if (bprd.ipver == AF_INET) {addr_len = 4;}
    else if (bprd.ipver == AF_INET6) {addr_len = 16;}
    else {BPRD_LOG_ERR("Unrecognized IP version");}

    if (pbb_writer_init(&pbb_w, mtu, 3*mtu) < 0) {
        BPRD_LOG_ERR("Unable to initialize packetbb writer");
    }

    if (pbb_writer_register_interface(&pbb_w, &pbb_iface, mtu) < 0) {
        BPRD_LOG_ERR("Unable to register packetbb interface");
    }
    /* set callbacks for this interface */
    pbb_iface.addPacketHeader = NULL;
    pbb_iface.finishPacketHeader = NULL;
    pbb_iface.sendPacket = hello_send;
    /* TODO: submit this hack to OLSR mailing list - uninitialized bin_msgs_size causes segfault when creating empty messages */
    /* We manually set to zero here */
    pbb_iface.bin_msgs_size = 0;

    /* register a message type with the writer that is not interface specific with addr_len */
    if ((pbb_hello_msgwriter = pbb_writer_register_message(&pbb_w, BPRD_MSG_TYPE_HELLO, false, addr_len)) == NULL) {
        BPRD_LOG_ERR("Unable to register hello message type");
    }
    /* set callbacks for message writing */
    pbb_hello_msgwriter->addMessageHeader = hello_add_msg_header;
    pbb_hello_msgwriter->finishMessageHeader = hello_fin_msg_header;

    pbb_writer_register_msgcontentprovider(&pbb_w, &pbb_cpr, BPRD_MSG_TYPE_HELLO, 1);

    pbb_cpr.addMessageTLVs = hello_add_msgtlvs;
    pbb_cpr.finishMessageTLVs = NULL;
    pbb_cpr.addAddresses = hello_add_addresses;

}


void hello_writer_destroy() {

    /* TODO: implement! */

}


/* loop endlessly and send hello messages */
static void *hello_writer_thread(void *arg __attribute__((unused)) ) {

    hello_writer_init();

    while (1) {

        pbb_writer_create_message(&pbb_w, BPRD_MSG_TYPE_HELLO, useAllIf, NULL);
        pbb_writer_flush(&pbb_w, &pbb_iface, false);

        /** \todo change to nanosleep() */
        usleep(bprd.hello_interval);
    }

    return NULL;
}


void hello_writer_thread_create() {

    /* TODO: check out pthread_attr options, currently set to NULL */
    if (pthread_create(&(bprd.hello_writer_tid), NULL, hello_writer_thread, NULL) < 0) {
        BPRD_LOG_ERR("Unable to create hello thread");
    }

    /* TODO: wait here until process stops? pthread_join(htdata.tid)? */
    /* TODO: handle the case where hello writer thread stops  - encapsulate in while(1) - restart hello thread? */

}
