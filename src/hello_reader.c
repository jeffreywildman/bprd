#include "hello.h"

#include <stdint.h>
#include <pthread.h>

#include <packetbb/pbb_reader.h>
#include <common/netaddr.h>

#include "dubp.h"
#include "logger.h"


static struct pbb_reader pbb_r;
static struct pbb_reader_tlvblock_consumer pbb_pkt_cons, pbb_msg_cons, pbb_addr_cons;


void hello_recv(uint8_t *buf, size_t buflen) {


    pbb_reader_handle_packet(&pbb_r, buf, buflen);

}


void hello_reader_init() {

    pbb_reader_init(&pbb_r);

    /* we don't care about the packet */
    pbb_reader_add_packet_consumer(&pbb_r, &pbb_pkt_cons, NULL, 0, 0);
    pbb_pkt_cons.start_callback = NULL;
    pbb_pkt_cons.tlv_callback = NULL;

    /* hello message consumer */
    pbb_reader_add_message_consumer(&pbb_r, &pbb_msg_cons, NULL, 0, DUBP_MSG_TYPE_HELLO, 0);
    pbb_msg_cons.start_callback = NULL;
    pbb_msg_cons.tlv_callback = NULL;

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
            /* confirm that packet came from correct MCAST protocol, addr, and port */
            /* call hello_recv(buf, buflen) here */
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
