
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

#ifndef _COMMON_AUTOBUF_H
#define _COMMON_AUTOBUF_H

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "common/common_types.h"

static const size_t AUTOBUFCHUNK = 4096;

/**
 * Auto-sized buffer handler, mostly used for generation of
 * large string buffers.
 */
struct autobuf {
  /* total number of bytes allocated in the buffer */
  size_t size;

  /* currently number of used bytes */
  size_t len;

  /* pointer to allocated memory */
  char *buf;
};

EXPORT void abuf_set_memory_handler(
    void *(*custom_malloc)(size_t),
    void *(*custom_realloc)(void *, size_t),
    void (*custom_free)(void *));

EXPORT int abuf_init(struct autobuf *autobuf, size_t initial_size);
EXPORT void abuf_free(struct autobuf *autobuf);
EXPORT int abuf_vappendf(struct autobuf *autobuf, const char *fmt,
    va_list ap) __attribute__ ((format(printf, 2, 0)));
EXPORT int abuf_appendf(struct autobuf *autobuf, const char *fmt,
    ...) __attribute__ ((format(printf, 2, 3)));
EXPORT int abuf_puts(struct autobuf * autobuf, const char *s);
EXPORT int abuf_strftime(struct autobuf * autobuf,
    const char *format, const struct tm * tm);
EXPORT int abuf_memcpy(struct autobuf * autobuf,
    const void *p, const size_t len);
EXPORT int abuf_memcpy_prefix(struct autobuf *autobuf,
    const void *p, const size_t len);
EXPORT void abuf_pull(struct autobuf * autobuf, size_t len);
EXPORT int abuf_template_init(const char **keys, size_t length,
    const char *format, size_t *indexTable, size_t indexLength);
EXPORT int abuf_templatef(struct autobuf *autobuf, const char *format,
    char **values, size_t *indexTable, size_t indexCount);

/**
 * Clears the content of an autobuf
 * @param autobuf
 */
static INLINE void
abuf_clear(struct autobuf *autobuf) {
  autobuf->len = 0;
  memset(autobuf->buf, 0, autobuf->size);
}

/**
 * Append a single byte to an autobuffer
 * @param autobuf
 * @param c byte to append
 * @return -1 if an error happened, 0 otherwise
 */
static INLINE int
abuf_append_uint8(struct autobuf *autobuf, const uint8_t c) {
  return abuf_memcpy(autobuf, &c, 1);
}

/**
 * Append a uint16 to an autobuffer
 * @param autobuf
 * @param s uint16 to append
 * @return -1 if an error happened, 0 otherwise
 */
static INLINE int
abuf_append_uint16(struct autobuf *autobuf, const uint16_t s) {
  return abuf_memcpy(autobuf, &s, 2);
}

/**
 * Append a uint32 to an autobuffer
 * @param autobuf
 * @param l uint32 to append
 * @return -1 if an error happened, 0 otherwise
 */
static INLINE int
abuf_append_uint32(struct autobuf *autobuf, const uint32_t l) {
  return abuf_memcpy(autobuf, &l, 4);
}

#endif

/*
 * Local Variables:
 * mode: c
 * style: linux
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
