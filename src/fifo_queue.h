/*
 * simple_fifo.h
 * (C) 2012 by Bradford Boyle <bradford@minerva.ece.drexel.edu
 * 
 * Implements a simple FIFO queue for libnetfilter_queue by keeoubg
 * track of the id the most recently seen packet and the id of the
 * most recently accepted/dropped packet
 */
#ifndef __FIFO_QUEUE_H_
#define __FIFO_QUEUE_H_

#include<stdint.h>
#include<libnetfilter_queue/libnetfilter_queue.h>

/* Typedefs to shorten netfilter_queue types*/
typedef struct nfq_q_handle nfq_qh_t;
typedef struct nfgenmsg nfgenmsg_t;
typedef struct nfq_data nfq_data_t;

/* Simple FIFO queue for keep tracking packets currently being
 * held in the kernel. Each enqueued packet is given an id number
 * that is sequentially increasing. We keep track of the id of the
 * most recently enqueued packet as the ``tail'' and the id of the
 * most recently released packet as the head.
 */
typedef struct dubp_simple_fifo {
	uint32_t head;
	uint32_t tail;

	nfq_qh_t *qh;
} fifo_t;


void fifo_init(fifo_t *queue);
int fifo_add_packet(nfq_qh_t *qh, nfgenmsg_t *nfmsg, nfq_data_t *nfa, void *data);
void fifo_send_packet(fifo_t *queue);
void fifo_drop_packet(fifo_t *queue);
inline uint32_t fifo_length(fifo_t *queue);
void fifo_delete(fifo_t *queue);
void fifo_print(fifo_t *queue);
#endif
