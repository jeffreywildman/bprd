#include "hello.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "dubp.h"
#include "logger.h"

#include <packetbb/pbb_writer.h>


static void hello_send(struct pbb_writer *w, struct pbb_writer_interface *iface, void *buffer, size_t buflen) {
    
    ssize_t n;
    
    if ((n = sendto(dubpd.sockfd, buffer, buflen, 0, (const struct sockaddr *)dubpd.maddr, dubpd.maddrlen)) < 0) {
        DUBP_LOG_ERR("Unable to send hello!");
    } else {
        DUBP_LOG_DBG("Sent hello message");
    }
}

static void hello_add_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg) {
    /* TODO: include originator address once dubpd.saddr is properly set */
    pbb_writer_set_msg_header(w, msg, 0, 0, 0, 1);
    //pbb_writer_set_msg_header(w, msg, 1, 0, 0, 1);
    //pbb_writer_set_msg_originator(w, msg, ???);
}

static void hello_fin_msg_header(struct pbb_writer *w, struct pbb_writer_message *msg, 
                                 struct pbb_writer_address *first_addr, 
                                 struct pbb_writer_address *last_addr, 
                                 bool not_fragmented) {
    pbb_writer_set_msg_seqno(w, msg, dubpd.hello_seqno);
    /* TODO: proper seqno rollover detection and handling */
    dubpd.hello_seqno++;
}

static bool useAllIf(struct pbb_writer *w, struct pbb_writer_interface *iface, void *param) {
    return true;
}

/* loop endlessly and send hello messages */
static void *hello_thread(void *arg __attribute__((unused)) ) {

    /* initialize packetbb writer */
    struct pbb_writer pbb_w;
    struct pbb_writer_interface pbb_iface;
    struct pbb_writer_message *pbb_hello_msgwriter;

    size_t mtu = 128;
    uint8_t addr_len = 4;  /* TODO: remove assumption of IPv4 */

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

    while (1) {

        pbb_writer_create_message(&pbb_w, DUBP_MSG_TYPE_HELLO, useAllIf, NULL);
        pbb_writer_flush(&pbb_w, &pbb_iface, false);

        /* TODO: generalize hello message interval */
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
