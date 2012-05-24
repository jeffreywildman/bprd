/*
 * fifo_queue.c
 * (C) 2012 by Bradford Boyle <bradford@minerva.ece.drexel.edu>
 *
 */

/**
 * \defgroup FIFO_Queue
 * \{
 */

#include "fifo_queue.h"

#include <netinet/in.h>                 /* must come before linux/netfilter.h so in_addr and in6_addr are defined */
                       /* http://fixunix.com/debian/494850-bug-487103-linux-libc-dev-netfilter-h-needs-h-include.html */

#include <stdio.h>                                  /* for printf() */
#include <linux/netfilter.h>                        /* for NF_ACCEPT/NF_DROP */
#include <libnetfilter_queue/libnetfilter_queue.h>  /* for nfq_set_verdict() */


/**
 * Initialize the internal representation FIFO queue
 * 
 * \param queue The queue.
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


/**
 * Callback function for adding packets to userspace queue
 * 
 * Function prototype specified by libnetfilter_queue
 *
 * \param qh
 * \param nfmsg
 * \param nfa
 * \param data
 */
int fifo_add_packet(nfq_qh_t *qh __attribute__ ((unused)), 
                    nfgenmsg_t *nfmsg __attribute__ ((unused)), 
                    nfq_data_t *nfa __attribute__ ((unused)), 
                    void *data)
{
	fifo_t *queue = (fifo_t *) data;
	if (queue)
	{
		(queue)->tail += 1;
	}

	// Callback should return < 0 to stop processing
	return 0;
}


/**
 * Send head of queue.
 *
 * The head of queue is the oldest packet in the queue.  Uses nfq_set_verdict() with a verdic of NF_ACCEPT.
 * 
 * \param queue The queue from which to send a packet.
 */
void fifo_send_packet(fifo_t *queue)
{
	if ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_ACCEPT, 0, NULL);
	}
}


/**
 * Drop head of queue.  
 *
 * The head of queue is the oldest packet in the queue.  Uses nfq_set_verdict() with a verdic of NF_DROP.
 *
 * \param queue The queue from which to drop a packet.
 */
void fifo_drop_packet(fifo_t *queue)
{
	if ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_DROP, 0, NULL);
	}
}


/**
 * Returns the number of packets currently enqueued.
 *
 * \param queue The queue that you want the length of.
 *
 * \return Number of packets.
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


/**
 * Drops all currently enqueued packets in preparation for freeing memory.
 * 
 * \param queue The queue to drop all packets from.
 */
void fifo_delete(fifo_t *queue)
{
	while ((queue) && ((queue)->head < (queue)->tail))
	{
		(queue)->head += 1;
		nfq_set_verdict((queue)->qh, (queue)->head, NF_DROP, 0, NULL);
	}
}


/**
 * Prints the id for all packets currently in the queue.
 *
 * \param queue The queue that you want to print.
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

/** \} */
