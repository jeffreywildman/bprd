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
#include <string.h>

#include "common/common_types.h"
#include "common/list.h"
#include "packetbb/pbb_writer.h"

static void _write_pktheader(struct pbb_writer_interface *interf);

/**
 * Internal function to start generation of a packet
 * This function should not be called by the user of the pbb API!
 *
 * @param writer pointer to writer context
 */
void
_pbb_writer_begin_packet(struct pbb_writer *writer, struct pbb_writer_interface *interface) {
  struct pbb_writer_pkthandler *handler;

  /* cleanup packet buffer data */
  _pbb_tlv_writer_init(&interface->pkt, interface->mtu, interface->mtu);

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_ADD_PKTHEADER;
#endif
  /* add packet header */
  if (interface->addPacketHeader) {
    interface->addPacketHeader(writer, interface);
  }
  else {
    pbb_writer_set_pkt_header(writer, interface, false);
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_ADD_PKTTLV;
#endif
  /* add packet tlvs */
  list_for_each_element(&writer->pkthandlers, handler, node) {
    handler->addPacketTLVs(writer, interface);
  }

  interface->is_flushed = false;
#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_NONE;
#endif
}

/**
 * Flush the current messages in the writer buffer and send
 * a complete packet.
 * @param writer pointer to writer context
 * @param force true if the writer should create an empty packet if necessary
 */
void
pbb_writer_flush(struct pbb_writer *writer, struct pbb_writer_interface *interface, bool force) {
  struct pbb_writer_pkthandler *handler;
  size_t len;

#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_NONE);
#endif

  assert(interface->sendPacket);

  if (interface->is_flushed) {
    if (!force) {
      return;
    }

    /* begin a new packet, buffer is flushed at the moment */
    _pbb_writer_begin_packet(writer, interface);
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_FINISH_PKTTLV;
#endif

  /* finalize packet tlvs */
  list_for_each_element_reverse(&writer->pkthandlers, handler, node) {
    handler->finishPacketTLVs(writer, interface);
  }

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_FINISH_PKTHEADER;
#endif
  /* finalize packet header */
  if (interface->finishPacketHeader) {
    interface->finishPacketHeader(writer, interface);
  }

  /* write packet header (including tlvblock length if necessary */
  _write_pktheader(interface);

  /* calculate true length of header (optional tlv block !) */
  len = 1;
  if (interface->has_seqno) {
    len += 2;
  }
  if (interface->pkt.added + interface->pkt.set > 0) {
    len += 2;
  }

  /* compress packet buffer */
  if (interface->bin_msgs_size) {
    memmove(&interface->pkt.buffer[len + interface->pkt.added + interface->pkt.set],
        &interface->pkt.buffer[interface->pkt.header + interface->pkt.added + interface->pkt.allocated],
        interface->bin_msgs_size);
  }

  /* send packet */
  interface->sendPacket(writer, interface, interface->pkt.buffer,
      len + interface->pkt.added + interface->pkt.set + interface->bin_msgs_size);

  /* cleanup length information */
  interface->pkt.set  = 0;
  interface->bin_msgs_size = 0;

  /* mark buffer as flushed */
  interface->is_flushed = true;

#if WRITER_STATE_MACHINE == 1
  writer->int_state = PBB_WRITER_NONE;
#endif

#if DEBUG_CLEANUP == 1
  memset(&interface->pkt.buffer[len + interface->pkt.added], 0,
      interface->pkt.max - len - interface->pkt.added);
#endif
}

/**
 * Adds a tlv to a packet.
 * This function must not be called outside the packet add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param interf pointer to writer interface object
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to packet, PBB_... otherwise
 */
enum pbb_result
pbb_writer_add_packettlv(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_PKTTLV);
#endif
  return _pbb_tlv_writer_add(&interf->pkt, type, exttype, value, length);
}

/**
 * Allocate memory for packet tlv.
 * This function must not be called outside the packet add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param interf pointer to writer interface object
 * @param has_exttype true if tlv has an extended type
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to packet, PBB_... otherwise
 */
enum pbb_result
pbb_writer_allocate_packettlv(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf, bool has_exttype, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_PKTTLV);
#endif
  return _pbb_tlv_writer_allocate(&interf->pkt, has_exttype, length);
}

/**
 * Sets a tlv for a packet, which memory has been already allocated.
 * This function must not be called outside the packet finish_tlv callback.
 *
 * @param writer pointer to writer context
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to packet, PBB_... otherwise
 */
enum pbb_result
pbb_writer_set_packettlv(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_FINISH_PKTTLV);
#endif
  return _pbb_tlv_writer_set(&interf->pkt, type, exttype, value, length);
}

/**
 * Initialize the header of a packet.
 * This function must not be called outside the packet add_header callback.
 *
 * @param writer pointer to writer context
 * @param has_seqno true if packet has a sequence number
 */
void pbb_writer_set_pkt_header(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf, bool has_seqno) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_PKTHEADER);
#endif

  /* we assume that we have always an TLV block and substract the 2 bytes later */
  interf->pkt.header = 1+2;

  /* handle sequence number */
  interf->has_seqno = has_seqno;
  if (has_seqno) {
    interf->pkt.header += 2;
  }
}

/**
 * Sets the sequence number of a packet.
 * This function must not be called outside the packet
 * add_header/finish_header callback.
 *
 * @param writer pointer to writer context
 * @param seqno sequence number of packet
 */
void
pbb_writer_set_pkt_seqno(struct pbb_writer *writer __attribute__ ((unused)),
    struct pbb_writer_interface *interf, uint16_t seqno) {
#if WRITER_STATE_MACHINE == 1
  assert(writer->int_state == PBB_WRITER_ADD_PKTHEADER
      || writer->int_state == PBB_WRITER_FINISH_PKTHEADER);
#endif
  interf->seqno = seqno;
}

/**
 * Write the header of a packet into the packet buffer
 * @param writer pointer to writer interface object
 */
static void
_write_pktheader(struct pbb_writer_interface *interf) {
  uint8_t *ptr;
  size_t len;

  ptr = interf->pkt.buffer;
  *ptr++ = 0;
  if (interf->has_seqno) {
    interf->pkt.buffer[0] |= PBB_PKT_FLAG_SEQNO;
    *ptr++ = (interf->seqno >> 8);
    *ptr++ = (interf->seqno & 255);
  }

  /* tlv-block ? */
  len = interf->pkt.added + interf->pkt.set;
  if (len > 0) {
    interf->pkt.buffer[0] |= PBB_PKT_FLAG_TLV;
    *ptr++ = (len >> 8);
    *ptr++ = (len & 255);
  }
}
