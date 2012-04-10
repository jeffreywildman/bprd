
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004-2011, the olsr.org team - see HISTORY file
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
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common_types.h"
#include "common/string.h"
#include "common/netaddr.h"

static const uint32_t _zero_addr[4] = { 0,0,0,0 };

static char *_mac_to_string(char *dst, const void *bin, size_t dst_size,
    size_t bin_size, char separator);
static int _mac_from_string(void *bin, size_t bin_size,
    const char *src, char separator);
static int _subnetmask_to_prefixlen(const char *src);
static int _read_hexdigit(const char c);
static bool _binary_is_in_subnet(const struct netaddr *subnet,
    const void *bin);
/**
 * Read the binary representation of an address into a netaddr object
 * @param dst pointer to netaddr object
 * @param binary source pointer
 * @param len length of source buffer
 * @param addr_type address type of source
 * @return 0 if successful read binary data, -1 otherwise
 */
int
netaddr_from_binary(struct netaddr *dst, void *binary, size_t len, uint8_t addr_type) {
  memset(dst->addr, 0, sizeof(dst->addr));
  if (addr_type == AF_INET && len >= 4) {
    /* ipv4 */
    memcpy(dst->addr, binary, 4);
    dst->prefix_len = 32;
  }
  else if (addr_type == AF_INET6 && len >= 16){
    /* ipv6 */
    memcpy(dst->addr, binary, 16);
    dst->prefix_len = 128;
  }
  else if (addr_type == AF_MAC48 && len >= 6) {
    /* mac48 */
    memcpy(&dst->addr, binary, 6);
    dst->prefix_len = 48;
  }
  else if (addr_type == AF_EUI64 && len >= 8) {
    /* eui 64 */
    memcpy(dst->addr, binary, 8);
    dst->prefix_len = 64;
  }
  else {
    /* unknown address type */
    return -1;
  }

  /* copy address type */
  dst->type = addr_type;

  return 0;
}

/**
 * Writes a netaddr object into a binary buffer
 * @param dst binary buffer
 * @param src netaddr source
 * @param len length of destination buffer
 * @return 0 if successful read binary data, -1 otherwise
 */
int
netaddr_to_binary(void *dst, struct netaddr *src, size_t len) {
  if (src->type == AF_INET && len >= 4) {
    /* ipv4 */
    memcpy(dst, src->addr, 4);
  }
  else if (src->type == AF_INET6 && len >= 16) {
    /* ipv6 */
    memcpy(dst, src->addr, 16);
  }
  else if (src->type == AF_MAC48 && len >= 6) {
    /* 48 bit MAC address */
    memcpy(dst, src->addr, 6);
  }
  else if (src->type == AF_EUI64 && len >= 8) {
    /* 64 bit EUI */
    memcpy(dst, src->addr, 8);
  }
  else {
    /* unknown address type */
    return -1;
  }
  return 0;
}

/**
 * Reads the address and address-type part of an
 * netaddr_socket into a netaddr object
 * @param dst netaddr object
 * @param src netaddr_socket source
 * @return 0 if successful read binary data, -1 otherwise
 */
int
netaddr_from_socket(struct netaddr *dst, union netaddr_socket *src) {
  memset(dst->addr, 0, sizeof(dst->addr));
  if (src->std.sa_family == AF_INET) {
    /* ipv4 */
    memcpy(dst->addr, &src->v4.sin_addr, 4);
    dst->prefix_len = 32;
  }
  else if (src->std.sa_family == AF_INET6){
    /* ipv6 */
    memcpy(dst->addr, &src->v6.sin6_addr, 16);
    dst->prefix_len = 128;
  }
  else {
    /* unknown address type */
    return -1;
  }
  dst->type = (uint8_t)src->std.sa_family;
  return 0;
}

/**
 * Writes the address and address-type of a netaddr object
 * into a netaddr_socket.
 * @param dst pointer to netaddr_socket
 * @param src netaddr source
 * @return 0 if successful read binary data, -1 otherwise
 */
int
netaddr_to_socket(union netaddr_socket *dst, struct netaddr *src) {
  /* copy address type */
  dst->std.sa_family = src->type;

  if (src->type == AF_INET) {
    /* ipv4 */
    memcpy(&dst->v4.sin_addr, src->addr, 4);
  }
  else if (src->type == AF_INET6) {
    /* ipv6 */
    memcpy(&dst->v6.sin6_addr, src->addr, 16);
  }
  else {
    /* unknown address type */
    return -1;
  }

  /* copy address type */
  dst->std.sa_family= src->type;
  return 0;
}


int
netaddr_to_autobuf(struct autobuf *abuf, struct netaddr *src) {
  switch (src->type) {
    case AF_INET:
      /* ipv4 */
      return abuf_memcpy(abuf, src->addr, 4);

    case AF_INET6:
      /* ipv6 */
      return abuf_memcpy(abuf, src->addr, 16);

    case AF_MAC48:
      /* 48 bit MAC address */
      return abuf_memcpy(abuf, src->addr, 6);

    case AF_EUI64:
      /* 64 bit EUI */
      return abuf_memcpy(abuf, src->addr, 8);

    default:
      /* unknown address type */
      return -1;
  }
}

/**
 * Initialize a netaddr_socket with a netaddr and a port number
 * @param combined pointer to netaddr_socket to be initialized
 * @param addr pointer to netaddr source
 * @param port port number for socket
 * @return 0 if successful read binary data, -1 otherwise
 */
int
netaddr_socket_init(union netaddr_socket *combined, struct netaddr *addr, uint16_t port) {
  /* initialize memory block */
  memset(combined, 0, sizeof(*combined));

  if (addr->type == AF_INET) {
    /* ipv4 */
    memcpy(&combined->v4.sin_addr, addr->addr, 4);
    combined->v4.sin_port = htons(port);
  }
  else if (addr->type == AF_INET6) {
    /* ipv6 */
    memcpy(&combined->v6.sin6_addr, addr->addr, 16);
    combined->v6.sin6_port = htons(port);
  }
  else {
    /* unknown address type */
    return -1;
  }

  /* copy address type */
  combined->std.sa_family = addr->type;
  return 0;
}

/**
 * @param sock pointer to netaddr_socket
 * @return port of socket
 */
uint16_t
netaddr_socket_get_port(union netaddr_socket *sock) {
  switch (sock->std.sa_family) {
    case AF_INET:
      return ntohs(sock->v4.sin_port);
    case AF_INET6:
      return ntohs(sock->v6.sin6_port);
    default:
      return 0;
  }
}

/**
 * Converts a netaddr into a string
 * @param dst target string buffer
 * @param src netaddr source
 * @param forceprefix true if a prefix should be appended even with maximum
 *   prefix length, false if only shorter prefixes should be appended
 * @return pointer to target buffer, NULL if an error happened
 */
const char *
netaddr_to_prefixstring(struct netaddr_str *dst,
    const struct netaddr *src, bool forceprefix) {
  const char *result = NULL;
  int maxprefix;

  if (src->type == AF_INET) {
    result = inet_ntop(AF_INET, src->addr, dst->buf, sizeof(*dst));
    maxprefix = 32;
  }
  else if (src->type == AF_INET6) {
    result = inet_ntop(AF_INET6, src->addr, dst->buf, sizeof(*dst));
    maxprefix = 128;
  }
  else if (src->type == AF_MAC48) {
    result = _mac_to_string(dst->buf, src->addr, sizeof(*dst), 6, ':');
    maxprefix = 48;
  }
  else if (src->type == AF_EUI64) {
    result = _mac_to_string(dst->buf, src->addr, sizeof(*dst), 8, '-');
    maxprefix = 64;
  }

  if (result != NULL && (forceprefix || src->prefix_len < maxprefix)) {
    /* append prefix */
    snprintf(dst->buf + strlen(result), 5, "/%d", src->prefix_len);
  }
  return result;
}

/**
 * Generates a netaddr from a string.
 * @param dst pointer to netaddr object
 * @param src pointer to input string
 * @return -1 if an error happened because of an unknown string,
 *   0 otherwise
 */
int
netaddr_from_string(struct netaddr *dst, const char *src) {
  struct netaddr_str buf;
  unsigned int colon_count, minus_count;
  int result;
  int prefix_len;
  bool has_coloncolon, has_point;
  bool last_was_colon;
  char *ptr1, *ptr2, *ptr3;

  colon_count = 0;
  minus_count = 0;
  has_coloncolon = false;
  has_point = false;

  last_was_colon = false;

  result = -1;
  prefix_len = -1;

  /* copy input string in temporary buffer */
  strscpy(buf.buf, src, sizeof(buf));
  ptr1 = buf.buf;

  str_trim(&ptr1);

  ptr2 = ptr1;
  while (*ptr2 != 0 && !isspace(*ptr2) && *ptr2 != '/') {
    switch (*ptr2) {
      case ':':
        if (last_was_colon) {
          has_coloncolon = true;
        }
        colon_count++;
        break;

      case '.':
        has_point = true;
        break;

      case '-':
        minus_count++;
        break;

      default:
        break;
    }
    last_was_colon = *ptr2++ == ':';
  }

  memset(dst, 0, sizeof(*dst));
  if (*ptr2) {
    /* split strings */
    while (isspace(*ptr2)) *ptr2++ = 0;
    if (*ptr2 == '/') {
      *ptr2++ = 0;
    }
    while (isspace(*ptr2)) *ptr2++ = 0;

    if (*ptr2 == 0) {
      /* prefixlength is missing */
      return -1;
    }

    /* try to read numeric prefix length */
    prefix_len = (int)strtoul(ptr2, &ptr3, 10);
    if (ptr3 && *ptr3) {
      /* not a numeric prefix length */
      prefix_len = -1;
    }
  }

  /* use dst->prefix_len as storage for maximum prefixlen */
  if ((colon_count == 5 || minus_count == 5)
      && (colon_count == 0 || minus_count == 0)
      && !has_point && !has_coloncolon) {
    dst->type = AF_MAC48;
    dst->prefix_len = 48;
    if (colon_count > 0) {
      result = _mac_from_string(dst->addr, 6, ptr1, ':');
    }
    else {
      result = _mac_from_string(dst->addr, 6, ptr1, '-');
    }
  }
  else if (colon_count == 0 && !has_point && minus_count == 7) {
    dst->type = AF_EUI64;
    dst->prefix_len = 64;
    dst->addr[7] = 2;
    result = _mac_from_string(dst->addr, 8, ptr1, '-');
  }
  else if (colon_count == 0 && has_point && minus_count == 0) {
    dst->type = AF_INET;
    dst->prefix_len = 32;
    result = inet_pton(AF_INET, ptr1, dst->addr) == 1 ? 0 : -1;

    if (result == 0 && *ptr2 && prefix_len == -1) {
      /* we need a prefix length, but its not a numerical one */
      prefix_len = _subnetmask_to_prefixlen(ptr2);
    }
  }
  else if ((has_coloncolon || colon_count == 7) && minus_count == 0) {
    dst->type = AF_INET6;
    dst->prefix_len = 128;
    result = inet_pton(AF_INET6, ptr1, dst->addr) == 1 ? 0 : -1;
  }

  /* stop if an error happened */
  if (result) {
    return -1;
  }

  if (*ptr2) {
    if (prefix_len < 0 || prefix_len > dst->prefix_len) {
      /* prefix is too long */
      return -1;
    }

    /* store real prefix length */
    dst->prefix_len = (uint8_t)prefix_len;
  }
  return result;
}

/**
 * Converts a netaddr_socket into a string
 * @param dst target string buffer
 * @param src netaddr_socket source
 * @return pointer to target buffer, NULL if an error happened
 */
const char *
netaddr_socket_to_string(struct netaddr_str *dst, union netaddr_socket *src) {
  struct netaddr_str buf;

  if (src->std.sa_family == AF_INET) {
    snprintf(dst->buf, sizeof(*dst), "%s:%d",
        inet_ntop(AF_INET, &src->v4.sin_addr, buf.buf, sizeof(buf)),
        ntohs(src->v4.sin_port));
  }
  else if (src->std.sa_family == AF_INET6) {
    snprintf(dst->buf, sizeof(*dst), "[%s]:%d",
        inet_ntop(AF_INET6, &src->v6.sin6_addr, buf.buf, sizeof(buf)),
        ntohs(src->v6.sin6_port));
  }
  else {
    /* unknown address type */
    return NULL;
  }

  return dst->buf;
}

/**
 * Compares two addresses in network byte order.
 * Address type will be compared last.
 *
 * This function is compatible with the avl comparator
 * prototype.
 * @param k1 address 1
 * @param k2 address 2
 * @return >0 if k1>k2, <0 if k1<k2, 0 otherwise
 */
int
netaddr_avlcmp(const void *k1, const void *k2, void *ptr __attribute__((unused))) {
  return netaddr_cmp(k1, k2);
}

/**
 * Compares an netaddr object with the address part of
 * a netaddr_socket.
 * @param a1 address
 * @param a2 socket
 * @return >0 if k1>k2, <0 if k1<k2, 0 otherwise
 */
int
netaddr_cmp_to_socket(const struct netaddr *a1, const union netaddr_socket *a2) {
  int result = 0;

  result = (int)a1->type - (int)a2->std.sa_family;
  if (result) {
    return result;
  }

  if (a1->type == AF_INET) {
    result = memcmp(a1->addr, &a2->v4.sin_addr, 4);
  }
  else if (a1->type == AF_INET6) {
    /* ipv6 */
    result = memcmp(a1->addr, &a2->v6.sin6_addr, 16);
  }

  if (result) {
    return result;
  }

  return (int)a1->prefix_len - (a1->type == AF_INET ? 32 : 128);
}

/**
 * Calculates if a binary address is equals to a netaddr one.
 * @param addr netaddr pointer
 * @param bin pointer to binary address
 * @param len length of binary address
 * @param af family of binary address
 * @param prefix_len prefix length of binary address
 * @return true if matches, false otherwise
 */
bool
netaddr_isequal_binary(const struct netaddr *addr,
    const void *bin, size_t len, uint16_t af, uint8_t prefix_len) {
  if (addr->type != af || addr->prefix_len != prefix_len) {
    return false;
  }

  if (af == AF_INET && len == 4) {
    return memcmp(addr->addr, bin, 4) == 0;
  }
  if (af == AF_INET6 && len == 16) {
    return memcmp(addr->addr, bin, 16) == 0;
  }
  if (af == AF_MAC48 && len == 6) {
    return memcmp(addr->addr, bin, 6) == 0;
  }
  if (af == AF_EUI64 && len == 8) {
    return memcmp(addr->addr, bin, 8) == 0;
  }
  return false;
}

/**
 * Checks if a binary address is part of a netaddr prefix.
 * @param addr netaddr prefix
 * @param bin pointer to binary address
 * @param len length of binary address
 * @param af address family of binary address
 * @return true if part of the prefix, false otherwise
 */
bool
netaddr_binary_is_in_subnet(const struct netaddr *subnet,
    const void *bin, size_t len, uint8_t af_family) {
  if (subnet->type != af_family
      || netaddr_get_maxprefix(subnet) != len * 8) {
    return false;
  }
  return _binary_is_in_subnet(subnet, bin);
}

/**
 * Checks if a netaddr object is part of another netaddr
 * prefix.
 * @param subnet netaddr prefix
 * @param addr netaddr object that might be inside the prefix
 * @return true if addr is part of subnet, false otherwise
 */
bool
netaddr_is_in_subnet(const struct netaddr *subnet,
    const struct netaddr *addr) {
  if (subnet->type != addr->type
      || subnet->prefix_len > addr->prefix_len) {
    return false;
  }

  return _binary_is_in_subnet(subnet, addr->addr);
}

/**
 * Calculates the maximum prefix length of an address type
 * @param af_type address family type
 * @return prefix length, 0 if unknown address family
 */
uint8_t
netaddr_get_maxprefix(const struct netaddr *addr) {
  switch (addr->type) {
    case AF_INET:
      return 32;
      break;
    case AF_INET6:
      return 128;
    case AF_MAC48:
      return 48;
      break;
    case AF_EUI64:
      return 64;
      break;

    default:
      return 0;
  }
}

#ifdef WIN32
/**
 * Helper function for windows
 * @param dst
 * @param bin
 * @param dst_size
 * @param bin_size
 * @param separator
 * @return
 */
const char *
inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
  if (af == AF_INET) {
    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    memcpy(&in.sin_addr, src, sizeof(struct in_addr));
    getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in),
        dst, cnt, NULL, 0, NI_NUMERICHOST);
    return dst;
  }
  else if (af == AF_INET6) {
    struct sockaddr_in6 in;
    memset(&in, 0, sizeof(in));
    in.sin6_family = AF_INET6;
    memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
    getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6),
        dst, cnt, NULL, 0, NI_NUMERICHOST);
    return dst;
  }
  return NULL;
}

/**
 * Helper function for windows
 * @param dst
 * @param bin
 * @param dst_size
 * @param bin_size
 * @param separator
 * @return
 */
int
inet_pton(int af, const char *src, void *dst)
{
  struct addrinfo hints, *res;
  union netaddr_socket *sock;

  if (af != AF_INET && af != AF_INET6) {
    return -1;
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = af;
  hints.ai_flags = AI_NUMERICHOST;

  if (getaddrinfo(src, NULL, &hints, &res) != 0)
  {
    return -1;
  }

  if (res == NULL) {
    return 0;
  }

  sock = (union netaddr_socket *)res->ai_addr;
  if (af == AF_INET) {
    memcpy(dst, &sock->v4.sin_addr, 4);
  }
  else {
    memcpy(dst, &sock->v6.sin6_addr, 16);
  }

  freeaddrinfo(res);
  return 1;
}

#endif

/**
 * Converts a binary mac address into a string representation
 * @param dst pointer to target string buffer
 * @param bin pointer to binary source buffer
 * @param dst_size size of string buffer
 * @param bin_size size of binary buffer
 * @param separator character for separating hexadecimal octets
 * @return pointer to target buffer, NULL if an error happened
 */
static char *
_mac_to_string(char *dst, const void *bin, size_t dst_size,
    size_t bin_size, char separator) {
  static const char hex[] = "0123456789abcdef";
  char *last_separator, *_dst;
  const uint8_t *_bin;

  _bin = bin;
  _dst = dst;
  last_separator = dst;

  if (dst_size == 0) {
    return NULL;
  }

  while (bin_size > 0 && dst_size >= 3) {
    *_dst++ = hex[(*_bin) >> 4];
    *_dst++ = hex[(*_bin) & 15];

    /* copy pointer to separator */
    last_separator = _dst;

    /* write separator */
    *_dst++ = separator;

    /* advance source pointer and decrease remaining length of buffer*/
    _bin++;
    bin_size--;

    /* calculate remaining destination size */
    dst_size-=3;
  }

  *last_separator = 0;
  return dst;
}

/**
 * Convert a string mac address into a binary representation
 * @param bin pointer to target binary buffer
 * @param bin_size pointer to size of target buffer
 * @param src pointer to source string
 * @param separator character used to separate octets in source string
 * @return 0 if sucessfully converted, -1 otherwise
 */
static int
_mac_from_string(void *bin, size_t bin_size, const char *src, char separator) {
  uint8_t *_bin;
  int num, digit_2;

  _bin = bin;

  while (bin_size > 0) {
    num = _read_hexdigit(*src++);
    if (num == -1) {
      return -1;
    }
    digit_2 = _read_hexdigit(*src);
    if (digit_2 >= 0) {
      num = (num << 4) + digit_2;
      src++;
    }
    *_bin++ = (uint8_t) num;

    bin_size--;

    if (*src == 0) {
      return bin_size ? -1 : 0;
    }
    if (*src++ != separator) {
      return -1;
    }
  }
  return -1;
}

/**
 * Reads a single hexadecimal digit
 * @param c digit to be read
 * @return integer value (0-15) of digit,
 *   -1 if not a hexadecimal digit
 */
static int
_read_hexdigit(const char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

/**
 * Converts a ipv4 subnet mask into a prefix length.
 * @param src string representation of subnet mask
 * @return prefix length, -1 if source was not a wellformed
 *   subnet mask
 */
static int
_subnetmask_to_prefixlen(const char *src) {
  uint32_t v4, shift;
  int len;

  if (inet_pton(AF_INET, src, &v4) != 1) {
    return -1;
  }

  /* transform into host byte order */
  v4 = ntohl(v4);

  shift = 0xffffffff;
  for (len = 31; len >= 0; len--) {
    if (v4 == shift) {
      return len;
    }
    shift <<= 1;
  }

  /* not wellformed */
  return -1;
}

/**
 * Calculates if a binary address is part of a netaddr prefix.
 * It will assume that the length of the binary address and its
 * address family makes sense.
 * @param addr netaddr prefix
 * @param bin pointer to binary address
 * @return true if part of the prefix, false otherwise
 */
static bool
_binary_is_in_subnet(const struct netaddr *subnet, const void *bin) {
  size_t byte_length, bit_length;
  const uint8_t *_bin;

  _bin = bin;

  /* split prefix length into whole bytes and bit rest */
  byte_length = subnet->prefix_len / 8;
  bit_length = subnet->prefix_len % 8;

  /* compare whole bytes */
  if (memcmp(subnet->addr, bin, byte_length) != 0) {
    return false;
  }

  /* compare bits if necessary */
  if (bit_length != 0) {
    return (subnet->addr[byte_length] >> (8 - bit_length))
        == (_bin[byte_length] >> (8 - bit_length));
  }
  return true;
}
