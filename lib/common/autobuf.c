
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#include "common/autobuf.h"

/**
 * Rounds up a size value to a certain power of 2
 * @param val original size
 * @param pow2 power of 2 (1024, 4096, ...)
 * @return rounded up size
 */
static inline size_t
ROUND_UP_TO_POWER_OF_2(size_t val, size_t pow2) {
  return (val + pow2 - 1) & ~(pow2 - 1);
}

static int _autobuf_enlarge(struct autobuf *autobuf, size_t new_size);
static int abuf_find_template(const char **keys, size_t tmplLength,
    const char *txt, size_t txtLength);
static void *_malloc(size_t size);

static void *(*autobuf_malloc)(size_t) = _malloc;
static void *(*autobuf_realloc)(void *, size_t) = realloc;
static void (*autobuf_free)(void *) = free;

/**
 * Allows to overwrite the memory handler functions for autobufs
 * @param custom_malloc overwrites malloc handler, NULL restores default one
 * @param custom_realloc overwrites realloc handler, NULL restores default one
 * @param custom_free overwrites free handler, NULL restores default one
 */
void
abuf_set_memory_handler(
    void *(*custom_malloc)(size_t),
    void *(*custom_realloc)(void *, size_t),
    void (*custom_free)(void *)) {
  autobuf_malloc = custom_malloc ? custom_malloc : _malloc;
  autobuf_realloc = custom_realloc ? custom_realloc : realloc;
  autobuf_free = custom_free ? custom_free : free;
}

/**
 * Initialize an autobuffer and allocate a chunk of memory
 * @param autobuf pointer to autobuf object
 * @param initial_size size of allocated memory, might be 0
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_init(struct autobuf *autobuf, size_t initial_size)
{
  autobuf->len = 0;
  if (initial_size <= 0) {
    autobuf->size = AUTOBUFCHUNK;
  }
  else {
    autobuf->size = ROUND_UP_TO_POWER_OF_2(initial_size, AUTOBUFCHUNK);
  }
  autobuf->buf = autobuf_malloc(autobuf->size);
  if (autobuf->buf == NULL) {
    autobuf->size = 0;
    return -1;
  }
  *autobuf->buf = '\0';
  return 0;
}

/**
 * Free all currently used memory of an autobuffer.
 * The buffer can still be used afterwards !
 * @param autobuf pointer to autobuf object
 */
void
abuf_free(struct autobuf *autobuf)
{
  autobuf_free(autobuf->buf);
  autobuf->buf = NULL;
  autobuf->len = 0;
  autobuf->size = 0;
}

/**
 * vprintf()-style function that appends the output to an autobuffer
 * @param autobuf pointer to autobuf object
 * @param format printf format string
 * @param ap variable argument list pointer
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_vappendf(struct autobuf *autobuf,
    const char *format, va_list ap)
{
  int rc;
  size_t min_size;
  va_list ap2;

  if (autobuf == NULL) return 0;

  va_copy(ap2, ap);
  rc = vsnprintf(autobuf->buf + autobuf->len, autobuf->size - autobuf->len, format, ap);
  va_end(ap);
  min_size = autobuf->len + (size_t)rc;
  if (min_size >= autobuf->size) {
    if (_autobuf_enlarge(autobuf, min_size) < 0) {
      autobuf->buf[autobuf->len] = '\0';
      return -1;
    }
    vsnprintf(autobuf->buf + autobuf->len, autobuf->size - autobuf->len, format, ap2);
  }
  va_end(ap2);
  autobuf->len = min_size;
  return 0;
}

/**
 * printf()-style function that appends the output to an autobuffer.
 * The function accepts a variable number of arguments based on the format string.
 * @param autobuf pointer to autobuf object
 * @param format printf format string
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_appendf(struct autobuf *autobuf, const char *fmt, ...)
{
  int rv;
  va_list ap;

  if (autobuf == NULL) return 0;

  va_start(ap, fmt);
  rv = abuf_vappendf(autobuf, fmt, ap);
  va_end(ap);
  return rv;
}

/**
 * Appends a null-terminated string to an autobuffer
 * @param autobuf pointer to autobuf object
 * @param s string to append to the buffer
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_puts(struct autobuf *autobuf, const char *s)
{
  size_t len;

  if (s == NULL) return 0;
  if (autobuf == NULL) return 0;

  len  = strlen(s);
  if (_autobuf_enlarge(autobuf, autobuf->len + len + 1) < 0) {
    return -1;
  }
  strcpy(autobuf->buf + autobuf->len, s);
  autobuf->len += len;
  return 0;
}

/**
 * Appends a formatted time string to an autobuffer
 * @param autobuf pointer to autobuf object
 * @param format strftime() format string
 * @param tm pointer to time data
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_strftime(struct autobuf *autobuf, const char *format, const struct tm *tm)
{
  size_t rc;

  if (autobuf == NULL) return 0;

  rc = strftime(autobuf->buf + autobuf->len, autobuf->size - autobuf->len, format, tm);
  if (rc == 0) {
    /* we had an error! Probably the buffer too small. So we add some bytes. */
    if (_autobuf_enlarge(autobuf, autobuf->size + AUTOBUFCHUNK) < 0) {
      autobuf->buf[autobuf->len] = '\0';
      return -1;
    }
    rc = strftime(autobuf->buf + autobuf->len, autobuf->size - autobuf->len, format, tm);
  }
  autobuf->len += rc;
  return rc == 0 ? -1 : 0;
}

/**
 * Copies a binary buffer to the end of an autobuffer.
 * @param autobuf pointer to autobuf object
 * @param p pointer to memory block to be copied
 * @param len length of memory block
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_memcpy(struct autobuf *autobuf, const void *p, const size_t len)
{
  if (autobuf == NULL) return 0;

  if (_autobuf_enlarge(autobuf, autobuf->len + len) < 0) {
    return -1;
  }
  memcpy(autobuf->buf + autobuf->len, p, len);
  autobuf->len += len;

  /* null-terminate autobuf */
  autobuf->buf[autobuf->len] = 0;

  return 0;
}

/**
 * Append a memory block to the beginning of an autobuffer.
 * @param autobuf pointer to autobuf object
 * @param p pointer to memory block to be copied as a prefix
 * @param len length of memory block
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_memcpy_prefix(struct autobuf *autobuf,
    const void *p, const size_t len)
{
  if (autobuf == NULL) return 0;

  if (_autobuf_enlarge(autobuf, autobuf->len + len) < 0) {
    return -1;
  }
  memmove(&autobuf->buf[len], autobuf->buf, autobuf->len);
  memcpy(autobuf->buf, p, len);
  autobuf->len += len;

  /* null-terminate autobuf */
  autobuf->buf[autobuf->len] = 0;

  return 0;
}

/**
 * Remove a prefix from an autobuffer. This function can be used
 * to create an autobuffer based fifo.
 * @param autobuf pointer to autobuf object
 * @param len number of bytes to be removed
 */
void
abuf_pull(struct autobuf * autobuf, size_t len) {
  char *p;
  size_t newsize;

  if (autobuf == NULL) return;

  if (len != autobuf->len) {
    memmove(autobuf->buf, &autobuf->buf[len], autobuf->len - len);
  }
  autobuf->len -= len;

  newsize = ROUND_UP_TO_POWER_OF_2(autobuf->len + 1, AUTOBUFCHUNK);
  if (newsize + 2*AUTOBUFCHUNK >= autobuf->size) {
    /* only reduce buffer size if difference is larger than two chunks */
    return;
  }

  /* generate smaller buffer */
  p = autobuf_realloc(autobuf->buf, newsize);
  if (p == NULL) {
    /* keep the longer buffer if we cannot get a smaller one */
    return;
  }
  autobuf->buf = p;
  autobuf->size = newsize;
  return;
}

/**
 * Initialize an index table for a template engine.
 * Each usage of a key in the format has to be %key%
 * @param keys array of keys for the template engine
 * @param tmplLength number of keys
 * @param format format string of the template
 * @param indexTable pointer to an size_t array with a minimum
 *   length of 3 times the number of keys used in the format string
 * @param indexLength length of the size_t array
 * @return number of indices written into index table,
 *   -1 if an error happened
 */
int
abuf_template_init (const char **keys, size_t tmplLength, const char *format, size_t *indexTable, size_t indexLength) {
  size_t pos = 0, indexCount = 0;
  size_t start = 0;
  int i = 0;
  bool escape = false;
  bool no_open_format = true;

  while (format[pos]) {
    if (!escape && format[pos] == '%') {
      if (no_open_format) {
        start = pos++;
        no_open_format = false;
        continue;
      }
      if (pos - start > 1) {
        if (indexCount + 3 > indexLength) {
          return -1;
        }

        i = abuf_find_template(keys, tmplLength, &format[start+1], pos-start-1);
        if (i != -1) {
          /* value index */
          indexTable[indexCount++] = (size_t)i;

          /* start position (including) */
          indexTable[indexCount++] = start;

          /* end position (excluding) */
          indexTable[indexCount++] = pos+1;
        }
      }
      no_open_format = true;
    }
    else if (format[pos] == '\\') {
      /* handle "\\" and "\%" in text */
      escape = !escape;
    }
    else {
      escape = false;
    }

    pos++;
  }
  return (int)indexCount;
}

/**
 * Append the result of a template engine into an autobuffer.
 * Each usage of a key will be replaced with the corresponding
 * value.
 * @param autobuf pointer to autobuf object
 * @param format format string (as supplied to abuf_template_init()
 * @param values array of values (same number as keys)
 * @param table pointer to index table initialized by abuf_template_init()
 * @param indexCount length of index table as returned by abuf_template_init()
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_templatef (struct autobuf *autobuf,
    const char *format, char **values, size_t *table, size_t indexCount) {
  size_t i, last = 0;

  if (autobuf == NULL) return 0;

  for (i=0; i<indexCount; i+=3) {
    /* copy prefix text */
    if (last < table[i+1]) {
      if (abuf_memcpy(autobuf, &format[last], table[i+1] - last) < 0) {
        return -1;
      }
    }
    if (abuf_puts(autobuf, values[table[i]]) < 0) {
      return -1;
    }
    last = table[i+2];
  }

  if (last < strlen(format)) {
    if (abuf_puts(autobuf, &format[last]) < 0) {
      return -1;
    }
  }
  return 0;
}

/**
 * Enlarge an autobuffer if necessary
 * @param autobuf pointer to autobuf object
 * @param new_size number of bytes necessary in autobuffer
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
static int
_autobuf_enlarge(struct autobuf *autobuf, size_t new_size)
{
  char *p;
  size_t roundUpSize;

  new_size++;
  if (new_size > autobuf->size) {
    roundUpSize = ROUND_UP_TO_POWER_OF_2(new_size+1, AUTOBUFCHUNK);
    p = autobuf_realloc(autobuf->buf, roundUpSize);
    if (p == NULL) {
#ifdef WIN32
      WSASetLastError(ENOMEM);
#else
      errno = ENOMEM;
#endif
      return -1;
    }
    autobuf->buf = p;

    memset(&autobuf->buf[autobuf->size], 0, roundUpSize - autobuf->size);
    autobuf->size = roundUpSize;
  }
  return 0;
}

/**
 * Find the position of one member of a string array in a text.
 * @param keys pointer to string array
 * @param tmplLength number of strings in array
 * @param txt pointer to text to search in
 * @param txtLength length of text to search in
 * @return index in array found in text, -1 if no string matched
 */
static int
abuf_find_template(const char **keys, size_t tmplLength, const char *txt, size_t txtLength) {
  size_t i;

  for (i=0; i<tmplLength; i++) {
    if (strncmp(keys[i], txt, txtLength) == 0 && keys[i][txtLength] == 0) {
      return (int)i;
    }
  }
  return -1;
}

/**
 * Internal implementation of malloc that clears buffer.
 * Maps to calloc(size, 1).
 * @param size number of bytes to be allocated
 * @return pointer to allocated memory, NULL if out of memory
 */
static void *
_malloc(size_t size) {
  return calloc(size, 1);
}

/*
 * Local Variables:
 * mode: c
 * style: linux
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
