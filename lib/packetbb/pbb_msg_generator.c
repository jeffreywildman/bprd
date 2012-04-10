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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common/common_types.h"
#include "packetbb/pbb_writer.h"

/* data necessary for automatic address compression */
struct _pbb_internal_addr_compress_session {
  struct pbb_writer_address *ptr;
  int total;
  int current;
  bool multiplen;
};

static void _calculate_tlv_flags(struct pbb_writer_address *addr, bool first);
static void _close_addrblock(struct _pbb_internal_addr_compress_session *acs,
    struct pbb_writer_message *msg, struct pbb_writer_address *last_addr, int);
static void _finalize_message_fragment(struct pbb_writer *writer,
    struct pbb_writer_message *msg, struct pbb_writer_address *first,
    struct pbb_writer_address *last, bool not_fragmented,
    pbb_writer_ifselector useIf, void *param);
static int _compress_address(struct _pbb_internal_addr_compress_session *acs,
    struct pbb_writer_message *msg, struct pbb_writer_address *addr,
    int same_prefixlen, bool first);
static void _write_addresses(struct pbb_writer *writer, struct pbb_writer_message *msg,
    struct pbb_writer_address *first_addr, struct pbb_writer_address *last_addr);
static void _write_msgheader(struct pbb_writer *writer, struct pbb_writer_message *msg);

/**
 * Create a message with a defined type
 * This function must NOT be called from the pbb writer callbacks.
 *
 * @param writer pointer to writer context
 * @param msgid type of message
 * @param useIf pointer to interface selector
 * @param param last parameter of interface selector
 * @return PBB_OKAY if message was created and added to packet buffer,
 *   PBB_... otherwise
 */
enum pbb_result
pbb_writer_create_message(struct pbb_writer *writer, uint8_t msgid,
    pbb_writer_ifselector useIf, void *param) {
  struct pbb_writer_message *msg;
  struct pbb_writer_content_provider *prv;
  struct list_entity *ptr1;
  struct pbb_writer_address *addr = NULL, *temp_addr = NULL, *first_addr = NULL;
  struct pbb_writer_tlvtype *tlvtype;
  struct pbb_writer_interface *interface;

  struct _pbb_internal_addr_compress_session acs[PBB_MAX_ADDRLEN];
  int best_size, best_head, same_prefixlen = 0;
  int i, idx;
  bool first;
  bool not_fragmented;
  size_t max_msg_size;
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_NONE);
#endif

  /* do nothing if no interface is defined */
  if (list_is_empty(&writer->interfaces)) {
    return PBB_OKAY;
  }

  /* find message create instance for the requested message */
  msg = avl_find_element(&writer->msgcreators, &msgid, msg, msgcreator_node);
  if (msg == NULL) {
    /* error, no msgcreator found */
    return PBB_NO_MSGCREATOR;
  }

  /*
   * test if we need interface specific messages
   * and this is not the single_if selector
   */
  if (!msg->if_specific) {
    /* not interface specific */
    msg->specific_if = NULL;
  }
  else if (useIf == pbb_writer_singleif_selector) {
    /* interface specific, but single_if selector is used */
    msg->specific_if = param;
  }
  else {
    /* interface specific, but generic selector is used */
    enum pbb_result result;

    list_for_each_element(&writer->interfaces, interface, node) {
      /* check if we should send over this interface */
      if (!useIf(writer, interface, param)) {
        continue;
      }

      /* create an unique message by recursive call */
      result = pbb_writer_create_message(writer, msgid, pbb_writer_singleif_selector, interface);
      if (result != PBB_OKAY) {
        return result;
      }
    }
    return PBB_OKAY;
  }

  /*
   * initialize packet buffers for all interfaces if necessary
   * and calculate message MTU
   */
  max_msg_size = writer->msg_mtu;
  list_for_each_element(&writer->interfaces, interface, node) {
    size_t interface_msg_mtu;

    /* check if we should send over this interface */
    if (!useIf(writer, interface, param)) {
      continue;
    }

    /* start packet if necessary */
    if (interface->is_flushed) {
      _pbb_writer_begin_packet(writer, interface);
    }

    interface_msg_mtu = interface->mtu
        - (interface->pkt.header + interface->pkt.added + interface->pkt.allocated);
    if (interface_msg_mtu < max_msg_size) {
      max_msg_size = interface_msg_mtu;
    }
  }

  /* initialize message tlvdata */
  _pbb_tlv_writer_init(&writer->msg, max_msg_size, writer->msg_mtu);

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_ADD_HEADER;
#endif
  /* let the message creator write the message header */
  pbb_writer_set_msg_header(writer, msg, false, false, false, false);
  if (msg->addMessageHeader) {
    msg->addMessageHeader(writer, msg);
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_ADD_MSGTLV;
#endif

  /* call content providers for message TLVs */
  avl_for_each_element(&msg->provider_tree, prv, provider_node) {
    if (prv->addMessageTLVs) {
      prv->addMessageTLVs(writer, prv);
    }
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_ADD_ADDRESSES;
#endif
  /* call content providers for addresses */
  avl_for_each_element(&msg->provider_tree, prv, provider_node) {
    if (prv->addAddresses) {
      prv->addAddresses(writer, prv);
    }
  }

  not_fragmented = true;
  /* no addresses ? */
  if (list_is_empty(&msg->addr_head)) {
    _finalize_message_fragment(writer, msg, NULL, NULL, true, useIf, param);
#if WRITER_STATE_MACHINE == 1
    writer->int_state = PBB_WRITER_NONE;
#endif
    _pbb_writer_free_addresses(writer, msg);
    return PBB_OKAY;
  }
  /* start address compression */
  first = true;
  addr = first_addr = list_first_element(&msg->addr_head, addr, addr_node);

  /* loop through addresses */
  idx = 0;
  ptr1 = msg->addr_head.next;
  while(ptr1 != &msg->addr_head) {
    addr = container_of(ptr1, struct pbb_writer_address, addr_node);
    if (first) {
      /* clear tlvtype information for adress compression */
      list_for_each_element(&msg->tlvtype_head, tlvtype, tlvtype_node) {
        memset(tlvtype->int_tlvblock_count, 0, sizeof(tlvtype->int_tlvblock_count));
        memset(tlvtype->int_tlvblock_multi, 0, sizeof(tlvtype->int_tlvblock_multi));
      }

      /* clear address compression session */
      memset(acs, 0, sizeof(acs));
      same_prefixlen = 1;
    }

    addr->index = idx++;

    /* calculate same_length/value for tlvs */
    _calculate_tlv_flags(addr, first);

    /* update session with address */
    same_prefixlen = _compress_address(acs, msg, addr, same_prefixlen, first);
    first = false;

    /* look for best current compression */
    best_head = -1;
    best_size = writer->msg.max + 1;
#if DO_ADDR_COMPRESSION == 1
    for (i = 0; i < msg->addr_len; i++) {
#else
    i=0;
    {
#endif
      int size = acs[i].total + acs[i].current;
      int count = addr->index - acs[i].ptr->index;

      /* a block of 255 addresses have an index difference of 254 */
      if (size < best_size && count <= 254) {
        best_head = i;
        best_size = size;
      }
    }

    /* fragmentation necessary */
    if (best_head == -1) {
      if (first_addr == addr) {
        /* even a single address does not fit into the block */
#if WRITER_STATE_MACHINE == 1
        writer->int_state = PBB_WRITER_NONE;
#endif
        _pbb_writer_free_addresses(writer, msg);
        return -1;
      }
      not_fragmented = false;

      /* get end of last fragment */
      temp_addr = list_prev_element(addr, addr_node);

      /* close all address blocks */
      _close_addrblock(acs, msg, temp_addr, 0);

      /* write message fragment */
      _finalize_message_fragment(writer, msg, first_addr, temp_addr, not_fragmented, useIf, param);

      first_addr = addr;
      first = true;

      /* continue without stepping forward */
      continue;
    } else {
      /* add cost for this address to total costs */
#if DO_ADDR_COMPRESSION == 1
      for (i = 0; i < msg->addr_len; i++) {
#else
      i=0;
      {
#endif
        acs[i].total += acs[i].current;

#if DEBUG_CLEANUP == 1
        acs[i].current = 0;
#endif
      }
    }

    ptr1 = ptr1->next;
  }

  /* get last address */
  addr = list_last_element(&msg->addr_head, addr, addr_node);

  /* close all address blocks */
  _close_addrblock(acs, msg, addr, 0);

  /* write message fragment */
  _finalize_message_fragment(writer, msg, first_addr, addr, not_fragmented, useIf, param);

  /* free storage of addresses and address-tlvs */
  _pbb_writer_free_addresses(writer, msg);

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_NONE;
#endif
  return PBB_OKAY;
}

/**
 * Single interface selector callback for message creation
 * @param writer
 * @param interf
 * @param param pointer to the specified interface
 * @return true if param equals interf, false otherwise
 */
bool
pbb_writer_singleif_selector(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf, void *param) {
  return interf == param;
}

/**
 * All interface selector callback for message creation
 * @param writer
 * @param interf
 * @param param
 * @return always true
 */
bool pbb_writer_allif_selector(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf __attribute__ ((unused)),
    void *param __attribute__ ((unused))) {
  return true;
}

/**
 * Write a binary packetbb message into the writers buffer to
 * forward it. This function handles the modification of hopcount
 * and hoplimit field. The original message will not be modified.
 * This function must NOT be called from the pbb writer callbacks.
 *
 * The function does demand the writer context pointer as void*
 * to be compatible with the readers forward_message callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message to be forwarded
 * @param len number of bytes of message
 * @param useIf function pointer to decide which interface is used
 *   for forwarding the message
 * @param param custom attribute of interface selector
 * @return PBB_OKAY if the message was put into the writer buffer,
 *   PBB_... if an error happened
 */
enum pbb_result
pbb_writer_forward_msg(struct pbb_writer *writer, uint8_t *msg, size_t len,
    pbb_writer_ifselector useIf, void *param) {
  int cnt, hopcount = -1, hoplimit = -1;
  uint16_t size;
  uint8_t flags, addr_len;
  uint8_t *ptr;
  struct pbb_writer_interface *interf;
  size_t max_msg_size;

#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_NONE);
#endif

  /* check if message is small enough to be forwarded */
  max_msg_size = writer->msg.max;
  list_for_each_element(&writer->interfaces, interf, node) {
    size_t max;

    if (!useIf(writer, interf, param)) {
      continue;
    }

    max = interf->pkt.max - (interf->pkt.header + interf->pkt.added + interf->pkt.allocated);

    if (max < max_msg_size) {
      max_msg_size = max;
    }
  }

  if (len > max_msg_size) {
    /* message too long, too much data in it */
    return PBB_FW_MESSAGE_TOO_LONG;
  }

  flags = msg[1];
  addr_len = flags & PBB_MSG_FLAG_ADDRLENMASK;

  cnt = 2;
  if ((flags & PBB_MSG_FLAG_ORIGINATOR) != 0) {
    cnt += addr_len;
  }
  if ((flags & PBB_MSG_FLAG_HOPLIMIT) != 0) {
    hoplimit = cnt++;
  }
  if ((flags & PBB_MSG_FLAG_HOPCOUNT) != 0) {
    hopcount = cnt++;
  }
  if ((flags & PBB_MSG_FLAG_SEQNO) != 0) {
    cnt += 2;
  }

  size = (msg[cnt] << 8) + msg[cnt+1];
  if (size != len) {
    /* bad message size */
    return PBB_FW_BAD_SIZE;
  }

  if (hoplimit != -1 && msg[hoplimit] <= 1) {
    /* do not forward a message with hopcount 1 or 0 */
    return PBB_OKAY;
  }

  list_for_each_element(&writer->interfaces, interf, node) {
    if (!useIf(writer, interf, param)) {
      continue;
    }


    /* check if we have to flush the message buffer */
    if (interf->pkt.header + interf->pkt.added + interf->pkt.set + interf->bin_msgs_size + len
        > interf->pkt.max) {
      /* flush the old packet */
      pbb_writer_flush(writer, interf, false);

      /* begin a new one */
      _pbb_writer_begin_packet(writer,interf);
    }

    ptr = &interf->pkt.buffer[interf->pkt.header + interf->pkt.added
                            + interf->pkt.allocated + interf->bin_msgs_size];
    memcpy(ptr, msg, len);

    /* correct hoplimit if necesssary */
    if (hoplimit != -1) {
      ptr[hoplimit]--;
    }

    /* correct hopcount if necessary */
    if (hopcount != -1) {
      ptr[hopcount]++;
    }
  }
  return PBB_OKAY;
}

/**
 * Adds a tlv to a message.
 * This function must not be called outside the message add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to packet, PBB_... otherwise
 */
enum pbb_result
pbb_writer_add_messagetlv(struct pbb_writer *writer,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_MSGTLV);
#endif
  return _pbb_tlv_writer_add(&writer->msg, type, exttype, value, length);
}

/**
 * Allocate memory for message tlv.
 * This function must not be called outside the message add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param has_exttype true if tlv has an extended type
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if memory for tlv has been allocated, PBB_... otherwise
 */
enum pbb_result
pbb_writer_allocate_messagetlv(struct pbb_writer *writer,
    bool has_exttype, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_MSGTLV);
#endif
  return _pbb_tlv_writer_allocate(&writer->msg, has_exttype, length);
}

/**
 * Sets a tlv for a message, which memory has been already allocated.
 * This function must not be called outside the message finish_tlv callback.
 *
 * @param writer pointer to writer context
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been set to packet, PBB_... otherwise
 */
enum pbb_result
pbb_writer_set_messagetlv(struct pbb_writer *writer,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_FINISH_MSGTLV);
#endif
  return _pbb_tlv_writer_set(&writer->msg, type, exttype, value, length);
}

/**
 * Sets a new address length for a message
 * This function must not be called outside the message add_header callback.
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param addrlen address length, must be less or equal than 16
 */
void
pbb_writer_set_msg_addrlen(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_message *msg, uint8_t addrlen) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER);
#endif

  assert(addrlen <= PBB_MAX_ADDRLEN);
  assert(addrlen >= 1);

  if (msg->has_origaddr && msg->addr_len != addrlen) {
    /*
     * we might need to fix the calculated header length if set_msg_header
     * was called before this function
     */
    writer->msg.header = writer->msg.header + addrlen - msg->addr_len;
  }
  msg->addr_len = addrlen;
}

/**
 * Initialize the header of a message.
 * This function must not be called outside the message add_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param has_originator true if header contains an originator address
 * @param has_hopcount true if header contains a hopcount
 * @param has_hoplimit true if header contains a hoplimit
 * @param has_seqno true if header contains a sequence number
 */
void
pbb_writer_set_msg_header(struct pbb_writer *writer, struct pbb_writer_message *msg,
    bool has_originator, bool has_hopcount, bool has_hoplimit, bool has_seqno) {

#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER);
#endif

  msg->has_origaddr = has_originator;
  msg->has_hoplimit = has_hoplimit;
  msg->has_hopcount = has_hopcount;
  msg->has_seqno = has_seqno;

  /* fixed parts: msg type, flags, length, tlvblock-length */
  writer->msg.header = 6;

  if (has_originator) {
    writer->msg.header += msg->addr_len;
  }
  if (has_hoplimit) {
    writer->msg.header++;
  }
  if (has_hopcount) {
    writer->msg.header++;
  }
  if (has_seqno) {
    writer->msg.header += 2;
  }
}

/**
 * Set originator address of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param originator pointer to originator address buffer
 */
void
pbb_writer_set_msg_originator(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_message *msg, uint8_t *originator) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER || writer->int_state == PBB_WRITER_FINISH_HEADER);
#endif

  memcpy(&msg->orig_addr[0], originator, msg->addr_len);
}

/**
 * Set hopcount of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param hopcount
 */
void
pbb_writer_set_msg_hopcount(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_message *msg, uint8_t hopcount) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER || writer->int_state == PBB_WRITER_FINISH_HEADER);
#endif
  msg->hopcount = hopcount;
}

/**
 * Set hoplimit of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param hoplimit
 */
void
pbb_writer_set_msg_hoplimit(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_message *msg, uint8_t hoplimit) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER || writer->int_state == PBB_WRITER_FINISH_HEADER);
#endif
  msg->hoplimit = hoplimit;
}

/**
 * Set sequence number of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param seqno sequence number of message header
 */
void
pbb_writer_set_msg_seqno(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_message *msg, uint16_t seqno) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_HEADER || writer->int_state == PBB_WRITER_FINISH_HEADER);
#endif
  msg->seqno = seqno;
}

/**
 * Update address compression session when a potential address block
 * is finished.
 *
 * @param acs pointer to address compression session
 * @param msg pointer to message object
 * @param last_addr pointer to last address object
 * @param common_head length of common head
 * @return common_head (might be modified common_head was 1)
 */
static void
_close_addrblock(struct _pbb_internal_addr_compress_session *acs,
    struct pbb_writer_message *msg __attribute__ ((unused)),
    struct pbb_writer_address *last_addr, int common_head) {
  int best, size;
#if DO_ADDR_COMPRESSION == 1
  int i;
  if (common_head > msg->addr_len) {
    /* nothing to do */
    return;
  }
#else
  assert(common_head == 0);
#endif
  /* check for best compression at closed blocks */
  best = common_head;
  size = acs[common_head].total;
#if DO_ADDR_COMPRESSION == 1
  for (i = common_head + 1; i < msg->addr_len; i++) {
    if (acs[i].total < size) {
      size = acs[i].total;
      best = i;
    }
  }
#endif
  /* store address block for later binary generation */
  acs[best].ptr->block_end = last_addr;
  acs[best].ptr->block_multiple_prefixlen = acs[best].multiplen;
  acs[best].ptr->block_headlen = best;

#if DO_ADDR_COMPRESSION == 1
  for (i = common_head + 1; i < msg->addr_len; i++) {
    /* remember best block compression */
    acs[i].total = size;
  }
#endif
  return;
}

/**
 * calculate the tlv_flags for the tlv value (same length/value).
 * @param addr pointer to address object
 * @param first true if this is the first address of this message
 */
static void
_calculate_tlv_flags(struct pbb_writer_address *addr, bool first) {
  struct pbb_writer_addrtlv *tlv;

  if (first) {
    avl_for_each_element(&addr->addrtlv_tree, tlv, addrtlv_node) {
      tlv->same_length = false;
      tlv->same_value = false;
    }
    return;
  }

  avl_for_each_element(&addr->addrtlv_tree, tlv, addrtlv_node) {
    struct pbb_writer_addrtlv *prev = NULL;

    /* check if this is the first tlv of this type */
    if (avl_is_first(&tlv->tlvtype->tlv_tree, &tlv->tlv_node)) {
      tlv->same_length = false;
      tlv->same_value = false;
      continue;
    }

    prev = avl_prev_element(tlv, tlv_node);

    if (tlv->address->index > prev->address->index + 1) {
      tlv->same_length = false;
      tlv->same_value = false;
      continue;
    }

    /* continous tlvs */
    tlv->same_length = tlv->length == prev->length;
    tlv->same_value = tlv->same_length &&
        (tlv->length == 0 || tlv->value == prev->value
            || memcmp(tlv->value, prev->value, tlv->length) == 0);
  }
}

/**
 * Update the address compression session with a new address.
 *
 * @param acs pointer to address compression session
 * @param msg pointer to message object
 * @param addr pointer to new address
 * @param same_prefixlen number of addresses (up to this) with the same
 *   prefix length
 * @param first true if this is the first address of the message
 * @return new number of messages with same prefix length
 */
static int
_compress_address(struct _pbb_internal_addr_compress_session *acs,
    struct pbb_writer_message *msg, struct pbb_writer_address *addr,
    int same_prefixlen, bool first) {
  struct pbb_writer_address *last_addr = NULL;
  struct pbb_writer_addrtlv *tlv;
  int i, common_head;
  uint8_t addrlen;
  bool special_prefixlen;

  addrlen = msg->addr_len;
  common_head = 0;
  special_prefixlen = addr->prefixlen != addrlen * 8;

  /* add size for address part (and header if necessary) */
  if (!first) {
    /* get previous address */
    last_addr = list_prev_element(addr, addr_node);

    /* remember how meny entries with the same prefixlength we had */
    if (last_addr->prefixlen == addr->prefixlen) {
      same_prefixlen++;
    } else {
      same_prefixlen = 1;
    }

    /* add bytes to continue encodings with same prefix */
#if DO_ADDR_COMPRESSION == 1
    for (common_head = 0; common_head < addrlen; common_head++) {
      if (last_addr->addr[common_head] != addr->addr[common_head]) {
        break;
      }
    }
#endif
    _close_addrblock(acs, msg, last_addr, common_head);
  }

  /* calculate new costs for next address including tlvs */
#if DO_ADDR_COMPRESSION == 1
  for (i = 0; i < addrlen; i++) {
#else
  i = 0;
  {
#endif
    int new_cost = 0, continue_cost = 0;
    bool closed = false;

#if DO_ADDR_COMPRESSION == 1
    closed = first || (i > common_head);
#else
    closed = true;
#endif
    /* cost of new address header */
    new_cost = 2 + (i > 0 ? 1 : 0) + msg->addr_len;
    if (special_prefixlen) {
      new_cost++;
    }

    if (!closed) {
      /* cost of continuing the last address header */
      continue_cost = msg->addr_len - i;
      if (acs[i].multiplen) {
        /* will stay multi_prefixlen */
        continue_cost++;
      }
      else if (same_prefixlen == 1) {
        /* will become multi_prefixlen */
        continue_cost += (acs[i].ptr->index - addr->index + 1);
      }
    }

    /* calculate costs for breaking/continuing tlv sequences */
    avl_for_each_element(&addr->addrtlv_tree, tlv, addrtlv_node) {
      struct pbb_writer_tlvtype *tlvtype = tlv->tlvtype;
      int cost;

      cost = 2 + (tlv->tlvtype->exttype ? 1 : 0) + 2 + tlv->length;
      if (tlv->length > 255) {
        cost++;
      }
      if (tlv->length > 0) {
          cost++;
      }

      new_cost += cost;
      if (!tlv->same_length || closed) {
        /* this TLV does not continue over the border of an address block */
        continue_cost += cost;
        continue;
      }

      if (tlvtype->int_tlvblock_multi[i]) {
        continue_cost += tlv->length;
      }
      else if (!tlv->same_value) {
        continue_cost += tlv->length * tlvtype->int_tlvblock_count[i];
      }
    }

    if (closed || acs[i].total + continue_cost > acs[addrlen-1].total + new_cost) {
      /* new address block */
      acs[i].ptr = addr;
      acs[i].multiplen = false;

      acs[i].total = acs[addrlen-1].total;
      acs[i].current = new_cost;

      closed = true;
    }
    else {
      acs[i].current = continue_cost;
      closed = false;
    }

    /* update internal tlv calculation */
    avl_for_each_element(&addr->addrtlv_tree, tlv, addrtlv_node) {
      struct pbb_writer_tlvtype *tlvtype = tlv->tlvtype;
      if (closed) {
        tlvtype->int_tlvblock_count[i] = 1;
        tlvtype->int_tlvblock_multi[i] = false;
      }
      else {
        tlvtype->int_tlvblock_count[i]++;
        tlvtype->int_tlvblock_multi[i] |= (!tlv->same_value);
      }
    }
  }
  return same_prefixlen;
}

/**
 * Write the address blocks to the message buffer.
 * @param writer pointer to writer context
 * @param msg pointer to message context
 * @param first_addr pointer to first address to be written
 * @param last_addr pointer to last address to be written
 */
static void
_write_addresses(struct pbb_writer *writer, struct pbb_writer_message *msg,
    struct pbb_writer_address *first_addr, struct pbb_writer_address *last_addr) {
  struct pbb_writer_address *addr_start, *addr_end, *addr;
  struct pbb_writer_tlvtype *tlvtype;
  struct pbb_writer_addrtlv *tlv_start, *tlv_end, *tlv;

  uint8_t *start, *ptr, *flag, *tlvblock_length;
  uint16_t total_len;

  assert(first_addr->block_end);

  addr_start = first_addr;
  ptr = &writer->msg.buffer[writer->msg.header + writer->msg.added + writer->msg.set];

  /* remember start */
  start = ptr;

  /* loop through address blocks */
  do {
    uint8_t head_len = 0, tail_len = 0, mid_len = 0;
#if DO_ADDR_COMPRESSION == 1
    bool zero_tail = false;
#endif

    addr_end = addr_start->block_end;
#if DO_ADDR_COMPRESSION == 1
    if (addr_start != addr_end) {
      /* only use head/tail for address blocks with multiple addresses */
      int tail;
      head_len = addr_start->block_headlen;
      tail_len = msg->addr_len - head_len - 1;

      /* calculate tail length and netmask length */
      list_for_element_range(addr_start, addr_end, addr, addr_node) {
        /* stop if no tail is left */
        if (tail_len == 0) {
          break;
        }

        for (tail = 1; tail <= tail_len; tail++) {
          if (addr_start->addr[msg->addr_len - tail] != addr->addr[msg->addr_len - tail]) {
            tail_len = tail - 1;
            break;
          }
        }
      }

      zero_tail = tail_len > 0;
      for (tail = 0; zero_tail && tail < tail_len; tail++) {
        if (addr_start->addr[msg->addr_len - tail - 1] != 0) {
          zero_tail = false;
        }
      }
    }
#endif
    mid_len = msg->addr_len - head_len - tail_len;

    /* write addrblock header */
    *ptr++ = addr_end->index - addr_start->index + 1;

    /* erase flag */
    flag = ptr;
    *ptr++ = 0;

#if DO_ADDR_COMPRESSION == 1
    /* write head */
    if (head_len) {
      *flag |= PBB_ADDR_FLAG_HEAD;
      *ptr++ = head_len;
      memcpy(ptr, &addr_start->addr[0], head_len);
      ptr += head_len;
    }

    /* write tail */
    if (tail_len > 0) {
      *ptr++ = tail_len;
      if (zero_tail) {
        *flag |= PBB_ADDR_FLAG_ZEROTAIL;
      } else {
        *flag |= PBB_ADDR_FLAG_FULLTAIL;
        memcpy(ptr, &addr_start->addr[msg->addr_len - tail_len], tail_len);
        ptr += tail_len;
      }
    }
#endif
    /* loop through addresses in block for MID part */
    list_for_element_range(addr_start, addr_end, addr, addr_node) {
      memcpy(ptr, &addr->addr[head_len], mid_len);
      ptr += mid_len;
    }

    /* loop through addresses in block for prefixlen part */
    if (addr_start->block_multiple_prefixlen) {
      /* multiple prefixlen */
      *flag |= PBB_ADDR_FLAG_MULTIPLEN;
      list_for_element_range(addr_start, addr_end, addr, addr_node) {
        *ptr++ = addr->prefixlen;
      }
    } else if (addr_start->prefixlen != msg->addr_len * 8) {
      /* single prefixlen */
      *flag |= PBB_ADDR_FLAG_SINGLEPLEN;
      *ptr++ = addr_start->prefixlen;
    }

    /* remember pointer for tlvblock length */
    tlvblock_length = ptr;
    ptr += 2;

    /* loop through all tlv types */
    list_for_each_element(&msg->tlvtype_head, tlvtype, tlvtype_node) {

      /* find first/last tlv for this address block */
      tlv_start = avl_find_ge_element(&tlvtype->tlv_tree, &addr_start->index, tlv_start, tlv_node);

      while (tlv_start != NULL && tlv_start->address->index <= addr_end->index) {
        bool same_value;

        /* get end of local TLV-Block and value-mode */
        same_value = true;
        tlv_end = tlv_start;

        avl_for_element_to_last(&tlvtype->tlv_tree, tlv_start, tlv, tlv_node) {
          if (tlv != tlv_start && tlv->address->index <= addr_end->index && tlv->same_length) {
            tlv_end = tlv;
            same_value &= tlv->same_value;
          }
        }

        /* write tlv */
        *ptr++ = tlvtype->type;

        /* remember flag pointer */
        flag = ptr;
        *ptr++ = 0;
        if (tlvtype->exttype) {
          *flag |= PBB_TLV_FLAG_TYPEEXT;
          *ptr++ = tlvtype->exttype;
        }

        /* copy original length field */
        total_len = tlv_start->length;

        if (tlv_start->address == addr_start && tlv_end->address == addr_end) {
          /* no index necessary */
        } else if (tlv_start == tlv_end) {
          *flag |= PBB_TLV_FLAG_SINGLE_IDX;
          *ptr++ = tlv_start->address->index - addr_start->index;
        } else {
          *flag |= PBB_TLV_FLAG_MULTI_IDX;
          *ptr++ = tlv_start->address->index - addr_start->index;
          *ptr++ = tlv_end->address->index - addr_start->index;

          /* length field is single_length*num for multivalue tlvs */
          if (!same_value) {
            total_len = total_len * ((tlv_end->address->index - tlv_start->address->index) + 1);
            *flag |= PBB_TLV_FLAG_MULTIVALUE;
          }
        }


        /* write length field and corresponding flags */
        if (total_len > 255) {
          *flag |= PBB_TLV_FLAG_EXTVALUE;
          *ptr++ = total_len >> 8;
        }
        if (total_len > 0) {
          *flag |= PBB_TLV_FLAG_VALUE;
          *ptr++ = total_len & 255;
        }

        if (tlv_start->length > 0) {
          /* write value */
          if (same_value) {
            memcpy(ptr, tlv_start->value, tlv_start->length);
            ptr += tlv_start->length;
          } else {
            avl_for_element_range(tlv_start, tlv_end, tlv, tlv_node) {
              memcpy(ptr, tlv->value, tlv->length);
              ptr += tlv->length;
            }
          }
        }

        if (avl_is_last(&tlvtype->tlv_tree, &tlv_end->tlv_node)) {
          tlv_start = NULL;
        } else {
          tlv_start = avl_next_element(tlv_end, tlv_node);
        }
      }
    }

    tlvblock_length[0] = (ptr - tlvblock_length - 2) >> 8;
    tlvblock_length[1] = (ptr - tlvblock_length - 2) & 255;
    addr_start = list_next_element(addr_end, addr_node);
  } while (addr_end != last_addr);

  /* store size of address(tlv) data */
  msg->bin_addr_size = ptr - start;
}

/**
 * Write header of message including mandatory tlvblock length field.
 * @param writer pointer to writer context
 * @param msg pointer to message object
 */
static void
_write_msgheader(struct pbb_writer *writer, struct pbb_writer_message *msg) {
  uint8_t *ptr, *flags;
  uint16_t total_size;
  ptr = writer->msg.buffer;

  /* type */
  *ptr++ = msg->type;

  /* flags & addrlen */
  flags = ptr;
  *ptr++ = msg->addr_len - 1;

  /* size */
  total_size = writer->msg.header + writer->msg.added + writer->msg.set + msg->bin_addr_size;
  *ptr++ = total_size >> 8;
  *ptr++ = total_size & 255;

  if (msg->has_origaddr) {
    *flags |= PBB_MSG_FLAG_ORIGINATOR;
    memcpy(ptr, msg->orig_addr, msg->addr_len);
    ptr += msg->addr_len;
  }
  if (msg->has_hoplimit) {
    *flags |= PBB_MSG_FLAG_HOPLIMIT;
    *ptr++ = msg->hoplimit;
  }
  if (msg->has_hopcount) {
    *flags |= PBB_MSG_FLAG_HOPCOUNT;
    *ptr++ = msg->hopcount;
  }
  if (msg->has_seqno) {
    *flags |= PBB_MSG_FLAG_SEQNO;
    *ptr++ = msg->seqno >> 8;
    *ptr++ = msg->seqno & 255;
  }

  /* write tlv-block size */
  total_size = writer->msg.added + writer->msg.set;
  *ptr++ = total_size >> 8;
  *ptr++ = total_size & 255;
}

/**
 * Finalize a message fragment, copy it into the packet buffer and
 * cleanup message internal data.
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param first pointer to first address of this fragment
 * @param last pointer to last address of this fragment
 * @param not_fragmented true if this is the only fragment of this message
 * @param useIf pointer to callback for selecting outgoing interfaces
 */
static void
_finalize_message_fragment(struct pbb_writer *writer, struct pbb_writer_message *msg,
    struct pbb_writer_address *first, struct pbb_writer_address *last, bool not_fragmented,
    pbb_writer_ifselector useIf, void *param) {
  struct pbb_writer_content_provider *prv;
  struct pbb_writer_interface *interface;
  uint8_t *ptr;
  size_t len;

  /* reset optional tlv length */
  writer->msg.set = 0;

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_FINISH_MSGTLV;
#endif

  /* inform message providers */
  avl_for_each_element_reverse(&msg->provider_tree, prv, provider_node) {
    if (prv->finishMessageTLVs) {
      prv->finishMessageTLVs(writer, prv, first, last, not_fragmented);
    }
  }

  if (first != NULL && last != NULL) {
    _write_addresses(writer, msg, first, last);
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_FINISH_HEADER;
#endif

  /* inform message creator */
  if (msg->finishMessageHeader) {
    msg->finishMessageHeader(writer, msg, first, last, not_fragmented);
  }

  /* write header */
  _write_msgheader(writer, msg);

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_NONE;
#endif

  /* precalculate number of fixed bytes of message header */
  len = writer->msg.header + writer->msg.added;

  list_for_each_element(&writer->interfaces, interface, node) {
    /* do we need to handle this interface ? */
    if (!useIf(writer, interface, param)) {
      continue;
    }

    /* calculate total size of packet and message, see if it fits into the current packet */
    if (interface->pkt.header + interface->pkt.added + interface->pkt.set + interface->bin_msgs_size
        + writer->msg.header + writer->msg.added + writer->msg.set + msg->bin_addr_size
        > interface->pkt.max) {

      /* flush the old packet */
      pbb_writer_flush(writer, interface, false);

      /* begin a new one */
      _pbb_writer_begin_packet(writer, interface);
    }


    /* get pointer to end of pkt buffer */
    ptr = &interface->pkt.buffer[interface->pkt.header + interface->pkt.added
                                 + interface->pkt.allocated + interface->bin_msgs_size];

    /* copy message header and message tlvs into packet buffer */
    memcpy(ptr, writer->msg.buffer, len + writer->msg.set);

    /* copy address blocks and address tlvs into packet buffer */
    ptr += len + writer->msg.set;
    memcpy(ptr, &writer->msg.buffer[len + writer->msg.allocated], msg->bin_addr_size);

    /* increase byte count of packet */
    interface->bin_msgs_size += len + writer->msg.set + msg->bin_addr_size;
  }

  /* clear length value of message address size */
  msg->bin_addr_size = 0;

  /* reset message tlv variables */
  writer->msg.set = 0;

  /* clear message buffer */
#if DEBUG_CLEANUP == 1
  memset(&writer->msg.buffer[len], 0, writer->msg.max - len);
#endif
}
