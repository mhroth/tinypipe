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



#ifndef _TINYPIPE_H_
#define _TINYPIPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  /*
   * This pipe assumes that there is only one producer thread and one consumer
   * thread. This data structure does not support any other configuration.
   */
  typedef struct TinyPipe {
    char *buffer;
    char *writeHead;
    char *readHead;
    int32_t len;
    int32_t remainingBytes; // total bytes from write head to end
  } TinyPipe;

  /**
   * Initialise the pipe with a given length, in bytes.
   *
   * @param q  The pipe.
   *
   * @return  Returns the size of the pipe in bytes.
   */
  int tpipe_init(TinyPipe *q, int numBytes);

  /**
   * Frees the internal buffer.
   *
   * @param q  The light pipe.
   */
  void tpipe_free(TinyPipe *q);

  /**
   * Indicates if data is available for reading.
   *
   * @param q  The light pipe.
   * @return Returns the number of bytes available for reading. Zero if no bytes are available.
   */
  int tpipe_hasData(TinyPipe *q);

  /**
  * Returns a pointer to a location in the pipe where numBytes can be written.
  *
  * @param q  The pipe.
  * @param numBytes  The number of bytes to be written.
  * @return  A pointer to a location where those bytes can be written. Returns
  *          NULL if no more space is available. Successive calls to this
  *          function may eventually return a valid pointer because the readhead
  *          has been advanced on another thread.
  */
  char *tpipe_getWriteBuffer(TinyPipe *q, int numBytes);

  /**
  * Indicates to the pipe how many bytes have been written.
  *
  * @param q  The pipe.
  * @param numBytes  The number of bytes written. In general this should be the
  *                  same value as was passed to the preceeding call to
  *                  tpipe_getWriteBuffer().
  */
  void tpipe_produce(TinyPipe *q, int numBytes);

  /**
  * Returns the current read buffer, indicating the number of bytes available
  * for reading.
  *
  * @param q  The pipe.
  * @param numBytes  This value will be filled with the number of bytes available
  *                  for reading.
  *
  * @return  A pointer to the read buffer.
  */
  char *tpipe_getReadBuffer(TinyPipe *q, int *numBytes);

  /**
   * Indicates that the next set of bytes have been read and are no longer needed.
   *
   * @param q  The pipe.
   */
  void tpipe_consume(TinyPipe *q);

  /**
   * Resets the queue to the initialised state. This should be done when only one thread is accessing the pipe.
   *
   * @param q  The pipe.
   */
  void tpipe_clear(TinyPipe *q);

  /**
   * Returns the total amount of data that is currently in the pipe.
   *
   * @param q  The pipe.
   *
   * @return  The number of bytes ready to be read from the pipe.
   */
  int tpipe_getTotalData(TinyPipe *q);

  /**
   * A convenience function to write a number of bytes to the pipe;
   *
   * @param q  The pipe.
   * @param data  The data pointer.
   * @param numBytes  The number of bytes to write.
   *
   * @return 1 if bytes were successfully written to the pipe. 0 otherwise.
   */
  int tpipe_write(TinyPipe *q, char *data, int numBytes);

#ifdef __cplusplus
}
#endif

#endif // _TINYPIPE_H_
