/*
 * fifo_queue.c
 * (C) 2012 by Bradford Boyle <bradford@minerva.ece.drexel.edu>
 *
 */
#include "fifo_queue.h"
#include <stdio.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT/NF_DROP */
#include<libnetfilter_queue/libnetfilter_queue.h>

/*
 * Initialize the internal representation FIFO queue
 * queue - the queue
 */
void fifo_init(fifo_t *queue)
{
	if (queue)
	{
		(queue)->head = 0;
		(queue)->tail = 0;
		(queue)->qh = NULL;
	}
}

/*
 * Callback function for adding packets to userspace queue
 * function prototype specified by libnetfilter_queue
 */
int fifo_add_packet(nfq_qh_t *qh, nfgenmsg_t *nfmsg, nfq_data_t *nfa, void *data)
{
	fifo_t *queue = (fifo_t *) data;
	if (queue)
	{
		(queue)->tail += 1;
	}

	// Callback should return < 0 to stop processing
	return 0;
}

/*
 * send head of queue (i.e. the oldest packet in the queue)
 * queue - the queue from which to send a packet
 * uses nfq_set_verdict() with a verdic of NF_ACCEPT
 */
void fifo_send_packet(fifo_t *queue)
{
	if ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_ACCEPT, 0, NULL);
	}
}

/*
 * drop head of queue (i.e. the oldest packet in the queue)
 * queue - the queue from which to drop a packet
 * uses nfq_set_verdict() with a verdic of NF_DROP
 */
void fifo_drop_packet(fifo_t *queue)
{
	if ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_DROP, 0, NULL);
	}
}

/*
 * returns the number of packets currently enqueued
 * queue - the queue that you want the length of
 */
inline uint32_t fifo_length(fifo_t *queue)
{
	uint32_t length = 0;
	if ((queue)->head < (queue)->tail)
	{
		length = (queue)->tail - (queue)->head;
	}
	
	return length;
}

/*
 * Drops all currently enqueued packets in preparation for freeing memory
 * queue - the queue to drop all packets from
 */
void fifo_delete(fifo_t *queue)
{
	while ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_DROP, 0, NULL);
	}
}

/*
 * Prints the id for all packets currently in the queue
 * queue - the queue that you want to print
 */
void fifo_print(fifo_t *queue)
{
	uint32_t i = 0;
	if (queue)
	{
		for (i = (queue)->head+1; i <= (queue)->tail; i++)
		{
			printf("pkt: %u\n", i);
		}
	}
}
