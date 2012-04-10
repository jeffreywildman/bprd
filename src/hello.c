#include "hello.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "dubp.h"
#include "logger.h"


/* loop endlessly and send hello messages */
static void *hello_thread(void *arg __attribute__((unused)) ) {

    hellomsg_t hello;
    uint32_t i = 0;
    ssize_t n;
    
    /* construct hello message */
    hello.type = 0xFF;
    hello.metric = htonl(0x12345678);

    while (1) {
        
        /* TODO: lock ntable.mutex before reading from ntable */
        hello.seqnum = htonl(i);
        /* TODO: unlock ntable.mutex after reading from ntable */

        if ((n = sendto(dubpd.sockfd, &hello, sizeof(hello), 0, (const struct sockaddr *)dubpd.maddr, dubpd.maddrlen)) < 0) {
            DUBP_LOG_ERR("Unable to send hello!");
        } else {
            DUBP_LOG_DBG("Sent hello message to " MANET_LINKLOCAL_ROUTERS_V4);
        }

        /* TODO: generalize hello message interval */
        sleep(dubpd.hello_interval);

        i++;
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
