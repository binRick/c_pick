#include "config.h"
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#define tty_putp(capability, fatal)    do {                 \
    if (tputs((capability), 1, tty_putc) == ERR && (fatal)) \
    errx(1, #capability ": unknown terminfo capability");   \
} while (0)

/////////////////////////////////////////////////////////////////////////////
struct choices_t {
  size_t          size;
  size_t          length;
  struct choice_t *v;
};
struct choice_t {
  const char *description;
  const char *string;
  size_t     length;
  ssize_t    match_start;               /* inclusive match start offset */
  ssize_t    match_end;                 /* exclusive match end offset */
  double     score;
};
struct pick_ctx_t {
  struct choices_t choices;
  struct choice_t  choice;
  char             *description_seperator;
};
/////////////////////////////////////////////////////////////////////////////
char *do_pick(struct pick_ctx_t *CTX);

/////////////////////////////////////////////////////////////////////////////
