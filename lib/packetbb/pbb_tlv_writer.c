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

#include <string.h>

#include "common/common_types.h"
#include "packetbb/pbb_tlv_writer.h"
#include "packetbb/pbb_context.h"

static size_t _calc_tlv_size(bool exttype, size_t length);
static void _write_tlv(uint8_t *ptr, uint8_t type, uint8_t exttype,
    uint8_t idx1, uint8_t idx2, void *value, size_t length);

/**
 * Internal function to initialize a data buffer (message or packet)
 * for writer.
 * This function should not be called by the user of the pbb API!
 *
 * @param data pointer to tlvdata object
 * @param max maximum number of bytes allowed for current context
 * @param mtu number of bytes of allocated buffer
 */
void
_pbb_tlv_writer_init(struct pbb_tlv_writer_data *data, size_t max,
    size_t mtu __attribute__ ((unused))) {
  data->header = 0;
  data->added = 0;
  data->allocated = 0;
  data->set = 0;
  data->max = max;

#if DEBUG_CLEANUP == 1
  memset(data->buffer, 0, mtu);
#endif
}

/**
 * Internal function to add a tlv to a tlvdata object.
 * This function should not be called by the user of the pbb API!
 *
 * @param data pointer to tlvdata object
 * @param type tlv type
 * @param exttype tlv extended type (0 for no extended type)
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes of tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to buffer, PBB_... otherwise
 */
enum pbb_result
_pbb_tlv_writer_add(struct pbb_tlv_writer_data *data,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
  size_t s;

  s = _calc_tlv_size(exttype != 0, length);
  if (data->header + data->added + data->allocated + s > data->max) {
    /* not enough space, even in message of maximum length */
    return PBB_MTU_TOO_SMALL;
  }

  _write_tlv(&data->buffer[data->header + data->added], type, exttype, 0, 255, value, length);
  data->added += s;
  return PBB_OKAY;

}

/**
 * Internal function to allocate memory for a tlv in a tlvdata object.
 * This function should not be called by the user of the pbb API!
 *
 * @param data pointer to tlvdata object
 * @param has_exttype true if tlv needs an extended tlv type
 * @param length number of bytes of tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to buffer, PBB_... otherwise
 */
enum pbb_result
_pbb_tlv_writer_allocate(struct pbb_tlv_writer_data *data,
    bool has_exttype, size_t length) {
  size_t s;

  s = _calc_tlv_size(has_exttype, length);
  if (data->header + data->added + data->allocated + s > data->max) {
    /* not enough space, even in message of maximum length */
    return PBB_MTU_TOO_SMALL;
  }
  data->allocated += s;
  return PBB_OKAY;
}

/**
 * Internal function to set a tlv in a tlvdata object,
 * which memory has been already allocated.
 * This function should not be called by the user of the pbb API!
 *
 * @param data pointer to tlvdata object
 * @param type tlv type
 * @param exttype tlv extended type (0 for no extended type)
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes of tlv value, 0 if no value
 * @return PBB_OKAY if tlv has been added to buffer, PBB_... otherwise
 */
enum pbb_result
_pbb_tlv_writer_set(struct pbb_tlv_writer_data *data,
    uint8_t type, uint8_t exttype, void *value, size_t length) {
  size_t s;

  s = _calc_tlv_size(exttype != 0, length);
  if (data->allocated - data->set < s) {
    /* not enough space reserved */
    return PBB_MTU_TOO_SMALL;
  }

  _write_tlv(&data->buffer[data->header + data->added + data->set], type, exttype, 0, 255, value, length);
  data->set += s;
  return PBB_OKAY;
}

/**
 * Calculates the length of an TLV in bytes
 * @param has_exttype true if tlv needs an extended tlv type
 * @param length number of bytes of tlv value, 0 if no value
 * @return number of bytes for TLV including header
 */
static size_t _calc_tlv_size(bool exttype, size_t length) {
  size_t s = 2;
  if (exttype) {
    s++;
  }

  s += length;
  if (length > 255) {
    s += 2;
  }
  else if (length > 0) {
    s++;
  }
  return s;
}

/**
 * Writes a TLV into a binary buffer.
 * This function does NOT do a length check before writing
 * @param ptr pointer to buffer
 * @param type tlv type
 * @param has_exttype extended tlv type, 0 if no extended type
 * @param idx1 lower index of tlv in address block, 0 if no address tlv
 * @param idx2 upper index of tlv in address block, 255 if no address tlv
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 */
static void
_write_tlv(uint8_t *ptr, uint8_t type, uint8_t exttype,
    uint8_t idx1, uint8_t idx2, void *value, size_t length) {
  uint8_t flags = 0;

  /* calculate flags field */
  if (exttype > 0) {
    flags |= PBB_TLV_FLAG_TYPEEXT;
  }
  if (idx1 == idx2) {
    flags |= PBB_TLV_FLAG_SINGLE_IDX;
  }
  else if (idx1 > 0 || idx2 < 255) {
    flags |= PBB_TLV_FLAG_MULTI_IDX;
  }
  if (length > 255) {
    flags |= PBB_TLV_FLAG_EXTVALUE;
  }
  if (length > 0) {
    flags |= PBB_TLV_FLAG_VALUE;
  }


  *ptr++ = type;
  *ptr++ = flags;
  if (exttype) {
    *ptr++ = exttype;
  }
  if (idx1 > 0 || idx2 < 255) {
    *ptr++ = idx1;
    if (idx1 != idx2) {
      *ptr++ = idx2;
    }
  }
  if (length > 255) {
    *ptr++ = length >> 8;
  }
  if (length > 0) {
    *ptr++ = length & 255;
    memcpy(ptr, value, length);
  }
}
