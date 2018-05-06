/**
 * Copyright (c) 2018 Martin Roth (mhroth@gmail.com).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tinypipe.h"

#if __SSE__
  #include <xmmintrin.h>
  #define hv_sfence() _mm_sfence()
#elif __arm__
  #if __ARM_ACLE
    #include <arm_acle.h>
    // https://msdn.microsoft.com/en-us/library/hh875058.aspx#BarrierRestrictions
    // http://doxygen.reactos.org/d8/d47/armintr_8h_a02be7ec76ca51842bc90d9b466b54752.html
    #define hv_sfence() __dmb(0xE) /* _ARM_BARRIER_ST */
  #else
    // http://stackoverflow.com/questions/19965076/gcc-memory-barrier-sync-synchronize-vs-asm-volatile-memory
    #define hv_sfence() __sync_synchronize()
  #endif
#elif _WIN32 || _WIN64
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms684208(v=vs.85).aspx
  #define hv_sfence() _WriteBarrier()
#else
  #define hv_sfence() __asm__ volatile("" : : : "memory")
#endif

#define HLP_STOP 0
#define HLP_LOOP -1
#define TPIPE_SET_INT32_AT_BUFFER(a, b) (*((int32_t *) (a)) = (b))
#define TPIPE_GET_INT32_AT_BUFFER(a) (*((int32_t *) (a)))

int tpipe_init(TinyPipe *q, int numBytes) {
  assert(numBytes > 0);
  q->buffer = (char *) malloc(numBytes);
  assert(q->buffer != NULL);
  q->writeHead = q->buffer;
  q->readHead = q->buffer;
  q->len = numBytes;
  q->remainingBytes = numBytes;
  TPIPE_SET_INT32_AT_BUFFER(q->buffer, HLP_STOP);
  return numBytes;
}

void tpipe_free(TinyPipe *q) {
  free(q->buffer);
}

int tpipe_hasData(TinyPipe *q) {
  int x = TPIPE_GET_INT32_AT_BUFFER(q->readHead);
  if (x == HLP_LOOP) {
    q->readHead = q->buffer;
    x = TPIPE_GET_INT32_AT_BUFFER(q->readHead);
  }
  return x;
}

char *tpipe_getWriteBuffer(TinyPipe *q, int bytesToWrite) {
  char *const readHead = q->readHead;
  char *const oldWriteHead = q->writeHead;
  const int totalByteRequirement = bytesToWrite + 2 * sizeof(int32_t);

  // check if there is enough space to write the data in the remaining
  // length of the buffer
  if (totalByteRequirement <= q->remainingBytes) {
    char *const newWriteHead = oldWriteHead + sizeof(int32_t) + bytesToWrite;

    // check if writing would overwrite existing data in the pipe (return NULL if so)
    if ((oldWriteHead < readHead) && (newWriteHead >= readHead)) return NULL;
    else return (oldWriteHead + sizeof(int32_t));
  } else {
    // there isn't enough space, try looping around to the start
    if (totalByteRequirement <= q->len) {
      if ((oldWriteHead < readHead) || ((q->buffer + totalByteRequirement) > readHead)) {
        return NULL; // overwrite condition
      } else {
        q->writeHead = q->buffer;
        q->remainingBytes = q->len;
        TPIPE_SET_INT32_AT_BUFFER(q->buffer, HLP_STOP);
        hv_sfence();
        TPIPE_SET_INT32_AT_BUFFER(oldWriteHead, HLP_LOOP);
        return q->buffer + sizeof(int32_t);
      }
    } else {
      return NULL; // there isn't enough space to write the data
    }
  }
}

void tpipe_produce(TinyPipe *q, int numBytes) {
  assert(q->remainingBytes >= (numBytes + (int) (2*sizeof(int32_t))));
  q->remainingBytes -= (sizeof(int32_t) + numBytes);
  char *const oldWriteHead = q->writeHead;
  q->writeHead += (sizeof(int32_t) + numBytes);
  TPIPE_SET_INT32_AT_BUFFER(q->writeHead, HLP_STOP);

  // save everything before this point to memory
  hv_sfence();

  // then save this
  TPIPE_SET_INT32_AT_BUFFER(oldWriteHead, numBytes);
}

char *tpipe_getReadBuffer(TinyPipe *q, int *numBytes) {
  *numBytes = TPIPE_GET_INT32_AT_BUFFER(q->readHead);
  char *const readBuffer = q->readHead + sizeof(int32_t);
  return readBuffer;
}

void tpipe_consume(TinyPipe *q) {
  assert(TPIPE_GET_INT32_AT_BUFFER(q->readHead) != HLP_STOP);
  q->readHead += sizeof(int32_t) + TPIPE_GET_INT32_AT_BUFFER(q->readHead);
}

void tpipe_clear(TinyPipe *q) {
  q->writeHead = q->buffer;
  q->readHead = q->buffer;
  q->remainingBytes = q->len;
  memset(q->buffer, 0, q->len);
}

int tpipe_getTotalData(TinyPipe *q) {
  char *p = q->readHead;
  int len = 0;
  int d = 0;
  while ((d = TPIPE_GET_INT32_AT_BUFFER(p)) != HLP_STOP) {
    if (d == HLP_LOOP) {
      p = q->buffer;
    } else {
      len += d;
      p += (sizeof(int32_t) + d);
    }
  }
  return len;
}

int tpipe_write(TinyPipe *q, char *data, int numBytes) {
  char *buffer = tpipe_getWriteBuffer(q, numBytes);
  if (buffer == NULL) return 0;
  memcpy(buffer, data, numBytes);
  tpipe_produce(q, numBytes);
  return 1;
}
