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

/*
 * fifo_queue.h
 *
 * A simple FIFO queue for libnetfilter_queue is maintained by keeping
 * track of the id the most recently seen packet and the id of the
 * most recently accepted/dropped packet.
 */

#ifndef __FIFO_QUEUE_H
#define __FIFO_QUEUE_H

#include <stdint.h>
#include <libnetfilter_queue/libnetfilter_queue.h>


typedef struct nfq_q_handle nfq_qh_t;
typedef struct nfgenmsg nfgenmsg_t;
typedef struct nfq_data nfq_data_t;

typedef struct bprd_simple_fifo {
	uint32_t head;
	uint32_t tail;

	nfq_qh_t *qh;
} fifo_t;


extern void fifo_init(fifo_t *queue);
extern int fifo_add_packet(nfq_qh_t *qh, nfgenmsg_t *nfmsg, nfq_data_t *nfa, void *data);
extern void fifo_send_packet(fifo_t *queue);
extern void fifo_drop_packet(fifo_t *queue);
extern inline uint32_t fifo_length(fifo_t *queue);
extern void fifo_delete(fifo_t *queue);
extern void fifo_print(fifo_t *queue);

#endif /* __FIFO_QUEUE_H */
