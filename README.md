# TinyPipe
Tiny Pipe is a small C library implementing a lockless queue for passing messages between one producer and one consumer thread. Implementations are included with support for x86 and ARM architectures.

## Usage

### Initialisation and Freeing
```c
#include "tinypipe.h"

Tinypipe pipe;
tpipe_init(&pipe, 10*1024); // 10KB pipe!

// do stuff

tpipe_free(&pipe);
```

### Reading
```c
while (tpipe_hasData(&pipe)) {
  char *buffer = NULL;
  int len = 0;
  buffer = tpipe_getReadBuffer(&pipe, &len);
  assert(buffer != NULL);
  assert(len > 0);

  // do stuff with the received buffer (e.g. parse an OSC message)
  tosc_message osc;
  tosc_parseMessage(&osc, buffer, len);
  // ...

  // consume the buffer (i.e. indicate that the data is no longer needed)
  // Important! Once the buffer is consumed, the data may not persist.
  // Copy the data it is needed later.
  tpipe_consume(&pipe);
}
```

### Writing
```c
int max_len = 1024; // the maximum required bytes is 1KB
char *buffer = tpipe_getReadBuffer(&pipe, max_len);
assert(buffer != NULL);

// write stuff to the buffer, which is guaranteed to have enough space to write the maximum requested length

// indicate that the buffer has been filled, but only with 512 bytes (<= max_len)
int used_len = 512;
tpipe_produce(&pipe, used_len);
```

### Clearing
It's easy to clear the pipe back to the initialised state.
```c
tpipe_clear(&pipe);
```

## License
This code is released under the [ISC License](https://opensource.org/licenses/ISC). It is strongly based on Enzien Audio's [HvLightPipe](https://github.com/enzienaudio/heavy/blob/master/src/TinyPipe.h), also released under the ISC license.
