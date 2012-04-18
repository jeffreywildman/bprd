#include "hello.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <packetbb/pbb_writer.h>
#include <common/netaddr.h>

#include "dubp.h"
#include "logger.h"

static struct pbb_writer pbb_w;
static struct pbb_writer_interface pbb_iface;
static struct pbb_writer_message *pbb_hello_msgwriter;
static struct pbb_writer_content_provider pbb_cpr;
static struct pbb_writer_tlvtype *addrtlv_type_comkey, *addrtlv_type_backlog;


static void hello_send(struct pbb_writer *w, struct pbb_writer_interface *iface, void *buffer, size_t buflen) {
    
    ssize_t n;
    
    if ((n = sendto(dubpd.sockfd, buffer, buflen, 0, (const struct sockaddr *)dubpd.maddr, dubpd.maddrlen)) < 0) {
        DUBP_LOG_ERR("Unable to send hello!");
    } else {
        DUBP_LOG_DBG("Sent hello message");
    }
}

static void hello_add_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg) {
    struct netaddr naddr;
    union netaddr_socket snaddr;
    
    pbb_writer_set_msg_header(w, msg, 1, 0, 0, 1);
    
    if (dubpd.ipver == AF_INET) {memcpy(&snaddr.v4,dubpd.saddr,dubpd.saddrlen);}
    else if (dubpd.ipver == AF_INET6) {memcpy(&snaddr.v6,dubpd.saddr,dubpd.saddrlen);} 
    else {DUBP_LOG_ERR("Unrecognized IP version");}
    
    netaddr_from_socket(&naddr,&snaddr);
    pbb_writer_set_msg_originator(w, msg, naddr.addr);
}

static void hello_fin_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg, 
                                 struct pbb_writer_address *first_addr, 
                                 struct pbb_writer_address *last_addr, 
                                 bool not_fragmented) {
    pbb_writer_set_msg_seqno(w, msg, dubpd.hello_seqno);
    /* TODO: proper seqno rollover detection and handling */
    dubpd.hello_seqno++;
}

static void hello_add_addresses(struct pbb_writer *w, struct pbb_writer_content_provider *provider) {

    struct netaddr naddr;
    union netaddr_socket snaddr;
    struct pbb_writer_address *addr;
    
    /* add my address to message */
    if (dubpd.ipver == AF_INET) {memcpy(&snaddr.v4,dubpd.saddr,dubpd.saddrlen);}
    else if (dubpd.ipver == AF_INET6) {memcpy(&snaddr.v6,dubpd.saddr,dubpd.saddrlen);} 
    else {DUBP_LOG_ERR("Unrecognized IP version");} 
    netaddr_from_socket(&naddr,&snaddr);
    /* TODO: set prefix length correctly */
    /* for now, use whole address */
    addr = pbb_writer_add_address(w, provider->creator, naddr.addr, 0);

    /* add my commodities to message */
    /* TODO: mutex lock commodity list? */
    commodity_t *c;
    for (c = LIST_FIRST(&dubpd.chead); c != NULL; c = LIST_NEXT(c, commodities)) {
        pbb_writer_add_addrtlv(w, addr, addrtlv_type_comkey, &c->addr, sizeof(c->addr), true);
        pbb_writer_add_addrtlv(w, addr, addrtlv_type_backlog, &c->backlog, sizeof(c->backlog), true);
    }   

    ntable_mutex_lock(&dubpd.ntable);
    /* refresh neighbor list */
    nlist_refresh(&dubpd.ntable.nhead); 
    /* add my neighbors to message */
    neighbor_t *n;
    for (n = LIST_FIRST(&dubpd.ntable.nhead); n != NULL; n = LIST_NEXT(n, neighbors)) {
        /* TODO: set prefix length correctly */
        /* for now, use whole address */
        pbb_writer_add_address(w, provider->creator, n->addr.addr, 0);
    }
    ntable_mutex_unlock(&dubpd.ntable);
}

static bool useAllIf(struct pbb_writer *w, struct pbb_writer_interface *iface, void *param) {
    return true;
}

/* loop endlessly and send hello messages */
static void *hello_thread(void *arg __attribute__((unused)) ) {

    /* initialize packetbb writer */
    size_t mtu = 128;
    uint8_t addr_len;

    if (dubpd.ipver == AF_INET) {addr_len = 4;}
    else if (dubpd.ipver == AF_INET6) {addr_len = 16;}
    else {DUBP_LOG_ERR("Unrecognized IP version");}

    if (pbb_writer_init(&pbb_w, mtu, 3*mtu) < 0) {
        DUBP_LOG_ERR("Unable to initialize packetbb writer");
    }

    if (pbb_writer_register_interface(&pbb_w, &pbb_iface, mtu) < 0) {
        DUBP_LOG_ERR("Unable to register packetbb interface");
    }
    /* set callbacks for this interface */
    pbb_iface.addPacketHeader = NULL;
    pbb_iface.finishPacketHeader = NULL;
    pbb_iface.sendPacket = hello_send;
    /* TODO: submit this hack to OLSR mailing list - uninitialized bin_msgs_size causes segfault when creating empty messages */
    /* We manually set to zero here */
    pbb_iface.bin_msgs_size = 0;

    /* register a message type with the writer that is not interface specific with addr_len */
    if ((pbb_hello_msgwriter = pbb_writer_register_message(&pbb_w, DUBP_MSG_TYPE_HELLO, false, addr_len)) == NULL) {
        DUBP_LOG_ERR("Unable to register hello message type");
    }
    /* set callbacks for message writing */
    pbb_hello_msgwriter->addMessageHeader = hello_add_msg_header;
    pbb_hello_msgwriter->finishMessageHeader = hello_fin_msg_header;

    pbb_writer_register_msgcontentprovider(&pbb_w, &pbb_cpr, DUBP_MSG_TYPE_HELLO, 1);
    addrtlv_type_comkey = pbb_writer_register_addrtlvtype(&pbb_w, DUBP_MSG_TYPE_HELLO, DUBP_ADDRTLV_TYPE_COMKEY, 0);
    addrtlv_type_backlog = pbb_writer_register_addrtlvtype(&pbb_w, DUBP_MSG_TYPE_HELLO, DUBP_ADDRTLV_TYPE_BACKLOG, 0);

    pbb_cpr.addMessageTLVs = NULL;
    pbb_cpr.finishMessageTLVs = NULL;
    pbb_cpr.addAddresses = hello_add_addresses;

    while (1) {

        pbb_writer_create_message(&pbb_w, DUBP_MSG_TYPE_HELLO, useAllIf, NULL);
        pbb_writer_flush(&pbb_w, &pbb_iface, false);

        /* TODO: allow interval to be fractions of a second */
        sleep(dubpd.hello_interval);
    }

    return NULL;
}


void hello_thread_create() {

    /* TODO: check out pthread_attr options, currently set to NULL */
    if (pthread_create(&(dubpd.hello_tid), NULL, hello_thread, NULL) < 0) {
        DUBP_LOG_ERR("Unable to create hello thread");
    }

    /* TODO: wait here until process stops? pthread_join(htdata.tid)? */
    /* TODO: handle the case where hello thread stops  - encapsulate in while(1) - restart hello thread? */

}
