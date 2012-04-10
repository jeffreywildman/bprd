/*
 * PacketBB handler library (see RFC 5444)
 * Copyright (c) 2010 Henning Rogge <hrogge@googlemail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org/git for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 */

#ifndef PBB_PARSER_H_
#define PBB_PARSER_H_

#include "common/avl.h"
#include "common/common_types.h"
#include "packetbb/pbb_context.h"

#if DISALLOW_CONSUMER_CONTEXT_DROP == 1
#define PBB_CONSUMER_DROP_ONLY(value, def) (def)
#else
#define PBB_CONSUMER_DROP_ONLY(value, def) (value)
#endif

/* Bitarray with 256 elements for skipping addresses/tlvs */
struct pbb_reader_bitarray256 {
  uint32_t a[256/32];
};

/* type of context for a pbb_reader_tlvblock_context */
enum pbb_reader_tlvblock_context_type {
  PBB_CONTEXT_PACKET,
  PBB_CONTEXT_MESSAGE,
  PBB_CONTEXT_ADDRESS
};

/**
 * This structs temporary holds the content of a decoded TLV.
 */
struct pbb_reader_tlvblock_entry {
  /* single linked list of entries */
  struct avl_node node;

  /* tlv type */
  uint8_t type;

  /* tlv flags */
  uint8_t flags;

  /* tlv type extension */
  uint8_t type_ext;

  /* tlv value length */
  uint16_t length;

  /*
   * pointer to tlv value, NULL if length == 0
   * this pointer is NOT aligned
   */
  uint8_t *single_value;

  /* index range of tlv (for address blocks) */
  uint8_t index1, index2;

  /* internal sorting order for types: tlvtype * 256 + exttype */
  uint16_t int_order;

  /*
   * pointer to start of value array, can be different from
   * "value" because of multivalue tlvs
   */
  uint8_t *int_value;

  /* true if this is a multivalue tlv */
  bool int_multivalue_tlv;

#if DISALLOW_CONSUMER_CONTEXT_DROP == 0
  /* internal bitarray to mark tlvs that shall be skipped by the next handler */
  struct pbb_reader_bitarray256 int_drop_tlv;
#endif
};

/* common context for packet, message and address TLV block */
struct pbb_reader_tlvblock_context {
  /* applicable for all TLV blocks */
  enum pbb_reader_tlvblock_context_type type;

  /* packet context */
  uint8_t pkt_version;
  uint8_t pkt_flags;

  bool has_pktseqno;
  uint16_t pkt_seqno;

  /*
   * message context
   * only for message and address TLV blocks
   */
  uint8_t msg_type;
  uint8_t msg_flags;
  uint8_t addr_len;

  bool has_hopcount;
  uint8_t hopcount;

  bool has_hoplimit;
  uint8_t hoplimit;

  bool has_origaddr;
  uint8_t orig_addr[PBB_MAX_ADDRLEN];

  uint16_t seqno;
  bool has_seqno;

  /*
   * address context
   * only for address TLV blocks
   */
  uint8_t *addr;
  uint8_t prefixlen;
};

/* internal representation of a parsed address block */
struct pbb_reader_addrblock_entry {
  /* single linked list of address blocks */
  struct list_entity list_node;

  /* corresponding tlv block */
  struct avl_tree tlvblock;

  /* number of addresses */
  uint8_t num_addr;

  /* start index/length of middle part of address */
  uint8_t mid_start, mid_len;

  /*
   * pointer to list of prefixes, NULL if same prefix length
   * for all addresses
   */
  uint8_t *prefixes;

  /* pointer to array of middle address parts */
  uint8_t *mid_src;

  /* storage for head/tail of address */
  uint8_t addr[PBB_MAX_ADDRLEN];

  /* storage for fixed prefix length */
  uint8_t prefixlen;

#if DISALLOW_CONSUMER_CONTEXT_DROP == 0
  /* bitarray to mark addresses that shall be skipped by the next handler */
  struct pbb_reader_bitarray256 dropAddr;
#endif
};

/**
 * representation of a consumer for a tlv block and context
 */
struct pbb_reader_tlvblock_consumer_entry {
  /* set by initialization to create a sorted list */
  struct avl_node node;

  /*
   * set by initialization, internal sorting order for types:
   * tlvtype * 256 + exttype
   */
  int int_order;

  /* set by the consumer if the entry is mandatory */
  bool mandatory;

  /* set by the consumer to define the type of the tlv */
  uint8_t type;

  /* set by the consumer to require a certain type extension */
  bool match_type_ext;

  /* set by the consumer to define the required type extension */
  uint8_t type_ext;

  /* set by the consumer to make the parser copy the TLV value into a private buffer */
  void *copy_value;
  uint16_t copy_value_maxlen;

  /* set by the parser to announce that the TLV was present multiple times */
  bool duplicate_tlv;

  /*
   * set by parser as a pointer to the TLVs data
   * This pointer will only be valid during the runtime of the
   * corresponding callback. Do not copy the pointer into a global
   * variable
   */
  struct pbb_reader_tlvblock_entry *tlv;

  /* set by the consumer callback together with a PBB_DROP_TLV to drop this TLV */
  bool drop;
};

/* representation of a tlv block consumer */
struct pbb_reader_tlvblock_consumer {
  /* sorted tree of consumers for a packet, message or address tlv block */
  struct avl_node node;

  /* order of this consumer */
  int order;

  /* if true the consumer will be called for all messages */
  bool default_msg_consumer;

  /*
   * message id of message and address consumers, ignored if
   * default_msg_consumer is true
   */
  uint8_t msg_id;

  /* true if an address block consumer, false if message/packet consumer */
  bool addrblock_consumer;

  /* Tree of sorted consumer entries */
  struct avl_tree consumer_entries;

  /* consumer for TLVblock context start and end*/
  enum pbb_result (*start_callback)(struct pbb_reader_tlvblock_consumer *,
      struct pbb_reader_tlvblock_context *context);
  enum pbb_result (*end_callback)(struct pbb_reader_tlvblock_consumer *,
      struct pbb_reader_tlvblock_context *context, bool dropped);

  /* consumer for single TLV */
  enum pbb_result (*tlv_callback)(struct pbb_reader_tlvblock_consumer *,
      struct pbb_reader_tlvblock_entry *,
      struct pbb_reader_tlvblock_context *context);

  /* consumer for tlv block and context */
  enum pbb_result (*block_callback)(struct pbb_reader_tlvblock_consumer *,
      struct pbb_reader_tlvblock_context *context, bool mandatory_missing);

  /* private data pointer for API user */
  void *private;
};

/* representation of the internal state of a packetbb parser */
struct pbb_reader {
  /* sorted tree of packet consumers */
  struct avl_tree packet_consumer;

  /* sorted tree of message/addr consumers */
  struct avl_tree message_consumer;

  /* callback for message forwarding */
  void (*forward_message)(struct pbb_reader_tlvblock_context *context, uint8_t *buffer, size_t length, void *);
  void *forward_message_data;

  /* callbacks for memory management */
  struct pbb_reader_tlvblock_entry* (*malloc_tlvblock_entry)(void);
  struct pbb_reader_addrblock_entry* (*malloc_addrblock_entry)(void);

  void (*free_tlvblock_entry)(void *);
  void (*free_addrblock_entry)(void *);
};

EXPORT void pbb_reader_init(struct pbb_reader *);
EXPORT void pbb_reader_cleanup(struct pbb_reader *);
EXPORT void pbb_reader_add_packet_consumer(struct pbb_reader *parser,
    struct pbb_reader_tlvblock_consumer *consumer,
    struct pbb_reader_tlvblock_consumer_entry *entries, size_t entrycount,
    int order);
EXPORT void pbb_reader_add_message_consumer(struct pbb_reader *,
    struct pbb_reader_tlvblock_consumer *,
    struct pbb_reader_tlvblock_consumer_entry *,
    size_t entrycount, uint8_t msg_id, int order);
EXPORT void pbb_reader_add_defaultmsg_consumer(struct pbb_reader *parser,
    struct pbb_reader_tlvblock_consumer *,
    struct pbb_reader_tlvblock_consumer_entry *entries,
    size_t entrycount, int order);
EXPORT void pbb_reader_add_address_consumer(struct pbb_reader *,
    struct pbb_reader_tlvblock_consumer *,
    struct pbb_reader_tlvblock_consumer_entry *,
    size_t entrycount, uint8_t msg_id, int order);
EXPORT void pbb_reader_add_defaultaddress_consumer(struct pbb_reader *parser,
    struct pbb_reader_tlvblock_consumer *,
    struct pbb_reader_tlvblock_consumer_entry *entries,
    size_t entrycount, int order);

EXPORT void pbb_reader_remove_packet_consumer(
    struct pbb_reader *, struct pbb_reader_tlvblock_consumer *);
EXPORT void pbb_reader_remove_message_consumer(
    struct pbb_reader *, struct pbb_reader_tlvblock_consumer *);

/**
 * Inline function to remove an address consumer. Both message and
 * address consumers are internally mapped to the same data structure,
 * so you can just use the pbb_reader_remove_message_consumer()
 * function.
 *
 * @param reader pointer to reader context
 * @param consumer pointer to tlvblock consumer object
 */
static INLINE void pbb_reader_remove_address_consumer(
    struct pbb_reader *reader, struct pbb_reader_tlvblock_consumer *consumer) {
  pbb_reader_remove_message_consumer(reader, consumer);
}

EXPORT int pbb_reader_handle_packet(
    struct pbb_reader *parser, uint8_t *buffer, size_t length);

#endif /* PBB_PARSER_H_ */
