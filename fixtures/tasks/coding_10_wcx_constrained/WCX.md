# wcx: count lines, words, bytes and the longest line

Write `wcx.c`, a program that reads standard input to the end and prints exactly one line:

    <lines> <words> <bytes> <maxlen>

four non-negative decimal integers separated by single spaces, followed by a newline.

* `lines`: the number of newline bytes (`\n`) in the input.
* `words`: the number of maximal runs of bytes that are not whitespace. Whitespace is exactly these six bytes:
  space, `\t`, `\n`, `\v`, `\f`, `\r`. Every other byte (including NUL and bytes above 127) belongs to a word.
* `bytes`: the total number of bytes read.
* `maxlen`: the length in bytes of the longest line, not counting its terminating `\n`. A last line without a
  trailing newline counts as a line for this value. A `\r` is an ordinary byte for `maxlen`.

## Constraints

* Plain C11. It must compile cleanly with `gcc -std=c11 -Wall -Wextra -Werror -O2 -o wcx wcx.c`.
* **No dynamic memory**: no `malloc`, `calloc`, `realloc`, `getline`, `getdelim` or `strdup`. Memory use must not
  depend on the input: a single line of 64 MiB has to work.
* Exit status 0 on success.
