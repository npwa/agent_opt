# ringbuf

Implement every function declared in `ringbuf.h` in a single file `ringbuf.c`.

- Plain C11, no external libraries, must compile cleanly with `-Wall -Werror`.
- The struct definition belongs in `ringbuf.c` (the header keeps it opaque).
- Reads and writes must wrap around the end of the internal storage correctly.
- Never read or write outside the allocated storage; `rb_destroy(NULL)` is allowed.
- A write larger than the free space stores what fits and returns that count.
