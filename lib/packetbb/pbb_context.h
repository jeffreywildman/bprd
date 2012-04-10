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

#ifndef PBB_CONTEXT_H_
#define PBB_CONTEXT_H_

#include "common/common_types.h"

/*
 * Return values for reader callbacks and API calls (and internal functions)
 * The PBB_DROP_... constants are ordered, higher values mean
 * dropping more of the context.
 * All return values less than zero represent an error.
 *
 * PBB_DROP_TLV means to drop the current tlv
 * PBB_DROP_ADDRESS means to drop the current address
 * PBB_DROP_MESSAGE means to drop the current message
 * PBB_DROP_PACKET means to drop the whole packet
 *
 * PBB_OKAY means everything is okay
 *
 * PBB_UNSUPPORTED_VERSION
 *   version field of packetbb is not 0
 * PBB_END_OF_BUFFER
 *   end of packetbb data stream before end of message/tlv
 * PBB_BAD_TLV_IDXFLAGS
 *   illegal combination of thassingleindex and thasmultiindex flags
 * PBB_BAD_TLV_VALUEFLAGS
 *   illegal combination of thasvalue and thasextlen flag
 * PBB_OUT_OF_MEMORY
 *   dynamic memory allocation failed
 * PBB_EMPTY_ADDRBLOCK
 *   address block with 0 addresses found
 * PBB_BAD_MSG_TAILFLAGS
 *   illegal combination of ahasfulltail and ahaszerotail flag
 * PBB_BAD_MSG_PREFIXFLAGS
 *   illegal combination of ahassingleprelen and ahasmultiprelen flag
 * PBB_DUPLICATE_TLV
 *   address tlv already exists
 * PBB_OUT_OF_ADDRTLV_MEM
 *   internal buffer for address tlv values too small
 * PBB_MTU_TOO_SMALL
 *   non-fragmentable part of message does not fit into max sizes packet
 * PBB_NO_MSGCREATOR
 *   cannot create a message without a message creator
 * PBB_FW_MESSAGE_TOO_LONG
 *   bad format of forwarded message, does not fit into max sized packet
 * PBB_FW_BAD_SIZE
 *   bad format of forwarded message, size field wrong
 */
enum pbb_result {
#if DISALLOW_CONSUMER_CONTEXT_DROP == 0
  PBB_DROP_PACKET          =  5,
  PBB_DROP_MESSAGE         =  4,
  PBB_DROP_MSG_BUT_FORWARD =  3,
  PBB_DROP_ADDRESS         =  2,
  PBB_DROP_TLV             =  1,
#endif
  PBB_OKAY                 =  0,
  PBB_UNSUPPORTED_VERSION  = -1,
  PBB_END_OF_BUFFER        = -2,
  PBB_BAD_TLV_IDXFLAGS     = -3,
  PBB_BAD_TLV_VALUEFLAGS   = -4,
  PBB_BAD_TLV_LENGTH       = -5,
  PBB_OUT_OF_MEMORY        = -6,
  PBB_EMPTY_ADDRBLOCK      = -7,
  PBB_BAD_MSG_TAILFLAGS    = -8,
  PBB_BAD_MSG_PREFIXFLAGS  = -9,
  PBB_DUPLICATE_TLV        = -10,
  PBB_OUT_OF_ADDRTLV_MEM   = -11,
  PBB_MTU_TOO_SMALL        = -12,
  PBB_NO_MSGCREATOR        = -13,
  PBB_FW_MESSAGE_TOO_LONG  = -14,
  PBB_FW_BAD_SIZE          = -15,
};

/* maximum address length */
/* defined as a macro because it's used to define length of arrays */
#define PBB_MAX_ADDRLEN ((int)16)

/* packet flags */
static const int PBB_PKT_FLAGMASK         = 0x0f;

static const int PBB_PKT_FLAG_SEQNO       = 0x08;
static const int PBB_PKT_FLAG_TLV         = 0x04;

/* message flags */
static const int PBB_MSG_FLAG_ORIGINATOR  = 0x80;
static const int PBB_MSG_FLAG_HOPLIMIT    = 0x40;
static const int PBB_MSG_FLAG_HOPCOUNT    = 0x20;
static const int PBB_MSG_FLAG_SEQNO       = 0x10;

static const int PBB_MSG_FLAG_ADDRLENMASK = 0x0f;

/* addressblock flags */
static const int PBB_ADDR_FLAG_HEAD       = 0x80;
static const int PBB_ADDR_FLAG_FULLTAIL   = 0x40;
static const int PBB_ADDR_FLAG_ZEROTAIL   = 0x20;
static const int PBB_ADDR_FLAG_SINGLEPLEN = 0x10;
static const int PBB_ADDR_FLAG_MULTIPLEN  = 0x08;

/* tlv flags */
static const int PBB_TLV_FLAG_TYPEEXT     = 0x80;
static const int PBB_TLV_FLAG_SINGLE_IDX  = 0x40;
static const int PBB_TLV_FLAG_MULTI_IDX   = 0x20;
static const int PBB_TLV_FLAG_VALUE       = 0x10;
static const int PBB_TLV_FLAG_EXTVALUE    = 0x08;
static const int PBB_TLV_FLAG_MULTIVALUE  = 0x04;

#endif /* PBB_CONTEXT_H_ */
