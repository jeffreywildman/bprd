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

#ifndef PBB_WRITER_H_
#define PBB_WRITER_H_

struct pbb_writer;
struct pbb_writer_message;

#include "common/avl.h"
#include "common/common_types.h"
#include "common/list.h"
#include "packetbb/pbb_context.h"
#include "packetbb/pbb_tlv_writer.h"

/*
 * Macros to iterate over existing addresses in a message(fragment)
 * during message generation (finishMessageHeader/finishMessageTLVs
 * callbacks)
 */
#define for_each_fragment_address(first, last, address, loop) list_for_element_range(first, last, address, addr_node, loop)
#define for_each_message_address(message, address, loop) list_for_each_element(&message->addr_head, address, addr_node, loop)

/**
 * state machine values for the writer.
 * If compiled with WRITE_STATE_MACHINE, this can check if the functions
 * of the writer are called from the right context.
 */
#if WRITER_STATE_MACHINE == 1
enum pbb_internal_state {
  PBB_WRITER_NONE,
  PBB_WRITER_ADD_PKTHEADER,
  PBB_WRITER_ADD_PKTTLV,
  PBB_WRITER_ADD_HEADER,
  PBB_WRITER_ADD_MSGTLV,
  PBB_WRITER_ADD_ADDRESSES,
  PBB_WRITER_FINISH_MSGTLV,
  PBB_WRITER_FINISH_HEADER,
  PBB_WRITER_FINISH_PKTTLV,
  PBB_WRITER_FINISH_PKTHEADER
};
#endif

/**
 * This struct represents a single address tlv of an address
 * during pbb message creation.
 */
struct pbb_writer_addrtlv {
  /* tree node of tlvs of a certain type/exttype */
  struct avl_node tlv_node;

  /* backpointer to tlvtype */
  struct pbb_writer_tlvtype *tlvtype;

  /* tree node of tlvs used by a single address */
  struct avl_node addrtlv_node;

  /* backpointer to address */
  struct pbb_writer_address *address;

  /* tlv type and extension is stored in writer_tlvtype */

  /* tlv value length */
  uint16_t length;

  /*
   * if multiple tlvs with the same type/ext have the same
   * value for a continous block of addresses, they should
   * use the same storage for the value (the pointer should
   * be the same)
   */
  void *value;

  /*
   * true if the TLV has the same length/value for the
   * address before this one too
   */
  bool same_length;
  bool same_value;
};

/**
 * This struct represents a single address during the pbb
 * message creation.
 */
struct pbb_writer_address {
  /* node of address list in writer_message */
  struct list_entity addr_node;

  /* node for quick access ( O(log n)) to addresses */
  struct avl_node addr_tree_node;

  /* tree to connect all TLVs of this address */
  struct avl_tree addrtlv_tree;

  /* index of the address */
  int index;

  /* address/prefix */
  uint8_t addr[PBB_MAX_ADDRLEN];
  uint8_t prefixlen;

  /* address block with same prefix/prefixlen until certain address */
  struct pbb_writer_address *block_end;
  uint8_t block_headlen;
  bool block_multiple_prefixlen;
};

/**
 * This struct is preallocated for each tlvtype that can be added
 * to an address of a certain message type.
 */
struct pbb_writer_tlvtype {
  /* node of tlvtype list in pbb_writer_message */
  struct list_entity tlvtype_node;

  /* back pointer to message creator */
  struct pbb_writer_message *creator;

  /* number of users of this tlvtype */
  int usage_counter;

  /* tlv type and extension is stored in writer_tlvtype */
  uint8_t type;

  /* tlv extension type */
  uint8_t exttype;

  /* head of writer_addrtlv list */
  struct avl_tree tlv_tree;

  /* tlv type*256 + tlv_exttype */
  int int_type;

  /* internal data for address compression */
  int int_tlvblock_count[PBB_MAX_ADDRLEN];
  bool int_tlvblock_multi[PBB_MAX_ADDRLEN];
};

/**
 * This struct represents a single content provider of
 * tlvs for a message context.
 */
struct pbb_writer_content_provider {
  /* node for tree of content providers for a message creator */
  struct avl_node provider_node;

  /* back pointer to message creator */
  struct pbb_writer_message *creator;

  /* priority of content provider */
  int priority;

  /* callbacks for adding tlvs and addresses to a message */
  void (*addMessageTLVs)(struct pbb_writer *,
      struct pbb_writer_content_provider *);
  void (*addAddresses)(struct pbb_writer *,
      struct pbb_writer_content_provider *);
  void (*finishMessageTLVs)(struct pbb_writer *,
      struct pbb_writer_content_provider *, struct pbb_writer_address *,
      struct pbb_writer_address *, bool);

  /* custom user data */
  void *user;
};

/**
 * This struct is allocated for each message type that can
 * be generated by the writer.
 */
struct pbb_writer_message {
  /* node for tree of message creators */
  struct avl_node msgcreator_node;

  /* tree of message content providers */
  struct avl_tree provider_tree;

  /*
   * true if the creator has already registered
   * false if the creator was registered because of a tlvtype or content
   * provider registration
   */
  bool registered;

  /* true if a different message must be generated for each interface */
  bool if_specific;

  /*
   * back pointer for interface this message is generated,
   * only used for interface specific message types
   */
  struct pbb_writer_interface *specific_if;

  /* message type */
  uint8_t type;

  /* message address length */
  uint8_t addr_len;

  /* message hopcount */
  bool has_hopcount;
  uint8_t hopcount;

  /* message hoplimit */
  bool has_hoplimit;
  uint8_t hoplimit;

  /* message originator */
  bool has_origaddr;
  uint8_t orig_addr[PBB_MAX_ADDRLEN];

  /* message sequence number */
  uint16_t seqno;
  bool has_seqno;

  /* binary data of message tlvblock */
  uint16_t tlvblock_length;
  void *tlvblock_value;

  /* allocated space for message tlvblock */
  uint16_t tlvblock_allocated;

  /* head of writer_address list/tree */
  struct list_entity addr_head;
  struct avl_tree addr_tree;

  /* head of writer_tlvtype list */
  struct list_entity tlvtype_head;

  /* callbacks for controling the message header fields */
  void (*addMessageHeader)(struct pbb_writer *, struct pbb_writer_message *);
  void (*finishMessageHeader)(struct pbb_writer *, struct pbb_writer_message *,
      struct pbb_writer_address *, struct pbb_writer_address *, bool);

  /* number of bytes neccessary for addressblocks including tlvs */
  size_t bin_addr_size;

  /* custom user data */
  void *user;
};

/**
 * This struct represents a single outgoing interface for
 * the pbb writer
 */
struct pbb_writer_interface {
  /* node for list of all interfaces */
  struct list_entity node;

  /* maximum number of bytes per packets allowed for interface */
  size_t mtu;

  /* packet buffer is currently flushed */
  bool is_flushed;

  /* buffer for constructing the current packet */
  struct pbb_tlv_writer_data pkt;

  /* number of bytes used by messages */
  size_t bin_msgs_size;

  /* packet seqno handling */
  bool has_seqno;
  uint16_t seqno;

  /* callback for interface specific packet handling */
  void (*addPacketHeader)(struct pbb_writer *, struct pbb_writer_interface *);
  void (*finishPacketHeader)(struct pbb_writer *, struct pbb_writer_interface *);
  void (*sendPacket)(struct pbb_writer *, struct pbb_writer_interface *, void *, size_t);

  /* custom user data */
  void *user;
};

/**
 * This struct represents a content provider for adding
 * tlvs to a packet header.
 */
struct pbb_writer_pkthandler {
  /* node for list of packet handlers */
  struct list_entity node;

  /* callbacks for packet handler */
  void (*addPacketTLVs)(struct pbb_writer *, struct pbb_writer_interface *);
  void (*finishPacketTLVs)(struct pbb_writer *, struct pbb_writer_interface *);

  /* custom user data */
  void *user;
};

/**
 * This struct represents the internal state of a
 * packetbb writer.
 */
struct pbb_writer {
  /* tree of all message handlers */
  struct avl_tree msgcreators;

  /* list of all packet handlers */
  struct list_entity pkthandlers;

  /* list of all interfaces */
  struct list_entity interfaces;

  /* maximum number of bytes in a message */
  size_t msg_mtu;

  /* buffer for constructing the current message */
  struct pbb_tlv_writer_data msg;

  /* temporary buffer for addrtlv values of a message */
  uint8_t *addrtlv_buffer;
  size_t addrtlv_size, addrtlv_used;

  /* callbacks for memory management */
  struct pbb_writer_address* (*malloc_address_entry)(void);
  struct pbb_writer_addrtlv* (*malloc_addrtlv_entry)(void);

  void (*free_address_entry)(void *);
  void (*free_addrtlv_entry)(void *);

  /* custom user data */
  void *user;
#ifdef WRITER_STATE_MACHINE
  enum pbb_internal_state int_state;
#endif
};

/* functions that can be called from addAddress callback */
EXPORT struct pbb_writer_address *pbb_writer_add_address(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint8_t *addr, uint8_t prefix);
EXPORT enum pbb_result pbb_writer_add_addrtlv(struct pbb_writer *writer,
    struct pbb_writer_address *addr, struct pbb_writer_tlvtype *tlvtype,
    void *value, size_t length, bool allow_dup);

/* functions that can be called from add/finishMessageTLVs callback */
EXPORT enum pbb_result pbb_writer_add_messagetlv(struct pbb_writer *writer,
    uint8_t type, uint8_t exttype, void *value, size_t length);
EXPORT enum pbb_result pbb_writer_allocate_messagetlv(struct pbb_writer *writer,
    bool has_exttype, size_t length);
EXPORT enum pbb_result pbb_writer_set_messagetlv(struct pbb_writer *writer,
    uint8_t type, uint8_t exttype, void *value, size_t length);

/* functions that can be called from add/finishMessageHeader callback */
EXPORT void pbb_writer_set_msg_addrlen(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint8_t addrlen);
EXPORT void pbb_writer_set_msg_header(struct pbb_writer *writer,
    struct pbb_writer_message *msg, bool has_originator,
    bool has_hopcount, bool has_hoplimit, bool has_seqno);
EXPORT void pbb_writer_set_msg_originator(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint8_t *originator);
EXPORT void pbb_writer_set_msg_hopcount(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint8_t hopcount);
EXPORT void pbb_writer_set_msg_hoplimit(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint8_t hoplimit);
EXPORT void pbb_writer_set_msg_seqno(struct pbb_writer *writer,
    struct pbb_writer_message *msg, uint16_t seqno);

/* functions that can be called from add/finishPacketTLVs callback */
EXPORT enum pbb_result pbb_writer_add_packettlv(
    struct pbb_writer *writer, struct pbb_writer_interface *interf,
    uint8_t type, uint8_t exttype, void *value, size_t length);
EXPORT enum pbb_result pbb_writer_allocate_packettlv(
    struct pbb_writer *writer, struct pbb_writer_interface *interf,
    bool has_exttype, size_t length);
EXPORT enum pbb_result pbb_writer_set_packettlv(
    struct pbb_writer *writer, struct pbb_writer_interface *interf,
    uint8_t type, uint8_t exttype, void *value, size_t length);

/* functions that can be called from add/finishPacketHeader */
EXPORT void pbb_writer_set_pkt_header(
    struct pbb_writer *writer, struct pbb_writer_interface *interf, bool has_seqno);
EXPORT void pbb_writer_set_pkt_seqno(
    struct pbb_writer *writer, struct pbb_writer_interface *interf, uint16_t seqno);

/* functions that can be called outside the callbacks */
EXPORT struct pbb_writer_tlvtype *pbb_writer_register_addrtlvtype(
    struct pbb_writer *writer, uint8_t msgtype, uint8_t tlv, uint8_t tlvext);
EXPORT void pbb_writer_unregister_addrtlvtype(struct pbb_writer *writer,
    struct pbb_writer_tlvtype *tlvtype);

EXPORT int pbb_writer_register_msgcontentprovider(struct pbb_writer *writer,
    struct pbb_writer_content_provider *cpr, uint8_t msgid, int priority);
EXPORT void pbb_writer_unregister_content_provider(struct pbb_writer *writer,
    struct pbb_writer_content_provider *cpr);

EXPORT struct pbb_writer_message *pbb_writer_register_message(
    struct pbb_writer *writer, uint8_t msgid, bool if_specific, uint8_t addr_len);
EXPORT void pbb_writer_unregister_message(struct pbb_writer *writer,
    struct pbb_writer_message *msg);

EXPORT void pbb_writer_register_pkthandler(struct pbb_writer *writer,
    struct pbb_writer_pkthandler *pkt);
EXPORT void pbb_writer_unregister_pkthandler(struct pbb_writer *writer,
    struct pbb_writer_pkthandler *pkt);

EXPORT int pbb_writer_register_interface(struct pbb_writer *writer,
    struct pbb_writer_interface *interf, size_t mtu);
EXPORT void pbb_writer_unregister_interface(
    struct pbb_writer *writer, struct pbb_writer_interface *interf);

/* functions for message creation */
typedef bool (*pbb_writer_ifselector)(struct pbb_writer *, struct pbb_writer_interface *, void *);

EXPORT bool pbb_writer_singleif_selector(struct pbb_writer *, struct pbb_writer_interface *, void *);
EXPORT bool pbb_writer_allif_selector(struct pbb_writer *, struct pbb_writer_interface *, void *);

EXPORT enum pbb_result pbb_writer_create_message(
    struct pbb_writer *writer, uint8_t msgid,
    pbb_writer_ifselector useIf, void *param);

/**
 * creates a message of a certain ID for a single interface
 * @param writer pointer to writer context
 * @param msgid type of message
 * @param interf pointer to outgoing interface
 * @return PBB_OKAY if message was created and added to packet buffer,
 *   PBB_... otherwise
 */
static INLINE enum pbb_result pbb_writer_create_message_singleif(
    struct pbb_writer *writer, uint8_t msgid, struct pbb_writer_interface *interf) {
  return pbb_writer_create_message(writer, msgid, pbb_writer_singleif_selector, interf);
}

/**
 * creates a message of a certain ID for all interface
 * @param writer pointer to writer context
 * @param msgid type of message
 * @return PBB_OKAY if message was created and added to packet buffer,
 *   PBB_... otherwise
 */
static INLINE enum pbb_result pbb_writer_create_message_allif(
    struct pbb_writer *writer, uint8_t msgid) {
  return pbb_writer_create_message(writer, msgid, pbb_writer_allif_selector, NULL);
}

EXPORT enum pbb_result pbb_writer_forward_msg(struct pbb_writer *writer,
    uint8_t *msg, size_t len, pbb_writer_ifselector useIf, void *param);
EXPORT void pbb_writer_flush(struct pbb_writer *writer,
    struct pbb_writer_interface *interface, bool force_empty_packet);

EXPORT int pbb_writer_init(struct pbb_writer *, size_t mtu, size_t addrtlv_data);
EXPORT void pbb_writer_cleanup(struct pbb_writer *writer);

/* internal functions that are not exported to the user */
void _pbb_writer_free_addresses(struct pbb_writer *writer, struct pbb_writer_message *msg);
void _pbb_writer_begin_packet(struct pbb_writer *writer, struct pbb_writer_interface *interf);


#endif /* PBB_WRITER_H_ */
