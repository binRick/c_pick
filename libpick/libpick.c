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
//////////////////////////////////////////////
#include "c_stringfn/include/stringfn.h"
#include "c_vector/vector/vector.h"
#include "libpick.h"
#include "log/log.h"
//////////////////////////////////////////////
#define PL_STDOUT    0x01 /* standout flag */
#define PL_MARKED    0x02 /* marked item */
enum key {
  UNKNOWN   = 0,
  ALT_ENTER = 1,
  BACKSPACE = 2,
  DEL       = 3,
  ENTER     = 4,
  CTRL_A    = 5,
  CTRL_C    = 6,
  CTRL_E    = 7,
  CTRL_K    = 8,
  CTRL_L    = 9,
  CTRL_O    = 10,
  CTRL_U    = 11,
  CTRL_W    = 12,
  CTRL_Z    = 13,
  RIGHT     = 14,
  LEFT      = 15,
  LINE_DOWN = 16,
  LINE_UP   = 17,
  PAGE_DOWN = 18,
  PAGE_UP   = 19,
  END       = 20,
  HOME      = 21,
  PRINTABLE = 22,
  CTRL_T    = 23,
  TAB       = 24,
};

static int                       choicecmp(const void *, const void *);
static void                      delete_between(char *, size_t, size_t, size_t);
static int                       filter_choices(size_t);
void load_choices(struct pick_ctx_t *CTX);
static enum key                  get_key(const char **);
static void                      handle_sigwinch(int);
static int                       isu8cont(unsigned char);
static int                       isu8start(unsigned char);
static int                       isword(const char *);
static size_t                    min_match(const char *, size_t, ssize_t *, ssize_t *);
static size_t                    print_choices(size_t, size_t);
static void                      print_line(const char *, size_t, int, ssize_t, ssize_t);
static const struct choice_t *selected_choice(struct pick_ctx_t *CTX);
static size_t                    skipescseq(const char *);
static const char *strcasechr(const char *, const char *);
static void                      toggle_sigwinch(int);
static int                       tty_getc(void);
static const char *tty_getcap(char *);
static void                      tty_init(int);
static const char *tty_parm1(char *, int);
static int                       tty_putc(int);
static void                      tty_restore(int);
static void                      tty_size(void);
static int                       xmbtowc(wchar_t *, const char *);
static struct Vector *do_pick(struct pick_ctx_t *CTX);
static char *strip_picked_desc(struct pick_ctx_t *CTX, char *s);

static struct termios        tio;
struct choices_t             choices = { 0 };
static FILE                  *tty_in, *tty_out;
static char                  *query;
static size_t                query_length, query_size;
static volatile sig_atomic_t gotsigwinch;
static unsigned int          choices_lines, tty_columns, tty_lines;
static int                   sort                 = 1;
static int                   use_alternate_screen = 1;
static int                   use_keypad           = 1;

char *do_pick_single(struct pick_ctx_t *CTX){
  CTX->mutliple = false;
  struct Vector *V = do_pick(CTX);
  char          *s = NULL;
  if (vector_size(V) > 0) {
    s = vector_get(V, 0);
  }

  return(s);
}
struct Vector *do_pick_multiple(struct pick_ctx_t *CTX){
  CTX->mutliple = true;
  return(do_pick(CTX));
}

struct pick_ctx_t *pick_init_ctx(){
  struct pick_ctx_t *C = malloc(sizeof(struct pick_ctx_t));

  C->choices_s_v           = vector_new();
  C->description_seperator = '|';
  C->mutliple              = false;
  return(C);
}

static char *strip_picked_desc(struct pick_ctx_t *CTX, char *s){
  char                   *S           = s;
  struct StringFNStrings choice_split = stringfn_split(s, CTX->description_seperator);

  if (choice_split.count > 1) {
    S = strdup(stringfn_mut_trim(choice_split.strings[0]));
    stringfn_release_strings_struct(choice_split);
  }
  return(S);
}

static struct Vector *do_pick(struct pick_ctx_t *CTX){
  struct Vector         *V = vector_new();
  const struct choice_t *choice;

  setlocale(LC_CTYPE, "");
  if (query == NULL) {
    query_size = 64;
    if ((query = calloc(query_size, sizeof(char))) == NULL) {
      err(1, NULL);
    }
  }
  {
    load_choices(CTX);
  }
  {
    tty_init(1);
    choice = selected_choice(CTX);
    tty_restore(1);
  }
  if (true == CTX->mutliple && choices.length > 0) {
    for ( size_t i = 0; i < choices.length; i++ ) {
      if (choices.v[i].mark) {
        vector_push(V, strip_picked_desc(CTX, strdup(choices.v[i].string)));
      }
    }
  }
  if (vector_size(V) == 0 && choice != NULL) {
    vector_push(V, strip_picked_desc(CTX, strdup(choice->string)));
  }

  if (choices.v) {
    free(choices.v);
  }
  if (query) {
    free(query);
  }

  return(V);
} /* do_pick */

void load_choices(struct pick_ctx_t *CTX){
  choices.length = vector_size(CTX->choices_s_v);
  choices.v      = reallocarray(NULL, choices.length, sizeof(struct choice_t));
  choices.size   = sizeof(struct choice_t) * choices.length;
  for (size_t i = 0; i < vector_size(CTX->choices_s_v); i++) {
    char            *c = (char *)vector_get(CTX->choices_s_v, i);
    struct choice_t *C = malloc(sizeof(struct choice_t));
    C->string      = c;
    C->description = "";
    C->length      = strlen(C->string);
    C->score       = 0;
    C->match_start = -1;
    C->match_end   = -1;
    choices.v[i]   = *C;
  }
}

const struct choice_t *selected_choice(struct pick_ctx_t *CTX){
  const char *buf;
  size_t     cursor_position, i, j, length, xscroll;
  size_t     choices_count = 0;
  size_t     selection     = 0;
  size_t     yscroll       = 0;
  int        dochoices     = 0;
  int        dofilter      = 1;
  int        query_grew    = 0;

  cursor_position = query_length;

  for ( ;;) {
    /*
     * If the user didn't add more characters to the query all
     * choices have to be reconsidered as potential matches.
     * In the opposite scenario, there's no point in reconsidered
     * all choices again since the ones that didn't match the
     * previous query will clearly not match the current one due to
     * the fact that previous query is a left-most substring of the
     * current one.
     */
    if (!query_grew) {
      choices_count = choices.length;
    }
    query_grew = 0;
    if (dofilter) {
      if ((dochoices = filter_choices(choices_count))) {
        dofilter = selection = yscroll = 0;
      }
    }

    tty_putp(cursor_invisible, 0);
    tty_putp(carriage_return, 1);               /* move cursor to first column */
    if (cursor_position >= tty_columns) {
      xscroll = cursor_position - tty_columns + 1;
    }else{
      xscroll = 0;
    }
    print_line(&query[xscroll], query_length - xscroll, 0, -1, -1);
    if (dochoices) {
      if (selection - yscroll >= choices_lines) {
        yscroll = selection - choices_lines + 1;
      }
      choices_count = print_choices(yscroll, selection);
    }
    tty_putp(carriage_return, 1);               /* move cursor to first column */
    for (i = j = 0; i < cursor_position; j++) {
      while (isu8cont(query[++i])) {
        continue;
      }
    }
    if (j > 0) {
      /*
       * parm_right_cursor interprets 0 as 1, therefore only
       * move the cursor if the position is non zero.
       */
      tty_putp(tty_parm1(parm_right_cursor, j), 1);
    }
    tty_putp(cursor_normal, 0);
    fflush(tty_out);

    switch (get_key(&buf)) {
    case CTRL_T:
    case TAB:
      if (true == CTX->mutliple) {
        choices.v[selection].mark = (choices.v[selection].mark) ? 0 : 1;
      }else{
      }
      if (selection < choices_count - 1) {
        selection++;
        if (selection - yscroll == choices_lines) {
          yscroll++;
        }
      }
      break;
    case ENTER:
      if (choices_count > 0) {
        return(&choices.v[selection]);
      }
      break;
    case ALT_ENTER:
      choices.v[choices.length].string      = query;
      choices.v[choices.length].description = "";
      return(&choices.v[choices.length]);

    case CTRL_C:
      return(NULL);

    case CTRL_Z:
      tty_restore(0);
      kill(getpid(), SIGTSTP);
      tty_init(0);
      break;
    case BACKSPACE:
      if (cursor_position > 0) {
        for (length = 1;
             isu8cont(query[cursor_position - length]);
             length++) {
          continue;
        }
        delete_between(query, query_length,
                       cursor_position - length, cursor_position);
        cursor_position -= length;
        query_length    -= length;
        dofilter         = 1;
      }
      break;
    case DEL:
      if (cursor_position < query_length) {
        for (length = 1;
             isu8cont(query[cursor_position + length]);
             length++) {
          continue;
        }
        delete_between(query, query_length,
                       cursor_position, cursor_position + length);
        query_length -= length;
        dofilter      = 1;
      }
      break;
    case CTRL_U:
      delete_between(query, query_length, 0, cursor_position);
      query_length   -= cursor_position;
      cursor_position = 0;
      dofilter        = 1;
      break;
    case CTRL_K:
      delete_between(query, query_length, cursor_position,
                     query_length);
      query_length = cursor_position;
      dofilter     = 1;
      break;
    case CTRL_L:
      tty_size();
      break;
    case CTRL_O:
      sort     = !sort;
      dofilter = 1;
      break;
    case CTRL_W:
      if (cursor_position == 0) {
        break;
      }

      for (i = cursor_position; i > 0;) {
        while (isu8cont(query[--i])) {
          continue;
        }
        if (isword(query + i)) {
          break;
        }
      }
      for (j = i; i > 0; i = j) {
        while (isu8cont(query[--j])) {
          continue;
        }
        if (!isword(query + j)) {
          break;
        }
      }
      delete_between(query, query_length, i, cursor_position);
      query_length   -= cursor_position - i;
      cursor_position = i;
      dofilter        = 1;
      break;
    case CTRL_A:
      cursor_position = 0;
      break;
    case CTRL_E:
      cursor_position = query_length;
      break;
    case LINE_DOWN:
      if (selection < choices_count - 1) {
        selection++;
        if (selection - yscroll == choices_lines) {
          yscroll++;
        }
      }
      break;
    case LINE_UP:
      if (selection > 0) {
        selection--;
        if (yscroll > selection) {
          yscroll--;
        }
      }
      break;
    case LEFT:
      while (cursor_position > 0
             && isu8cont(query[--cursor_position])) {
        continue;
      }
      break;
    case RIGHT:
      while (cursor_position < query_length
             && isu8cont(query[++cursor_position])) {
        continue;
      }
      break;
    case PAGE_DOWN:
      if (selection + choices_lines < choices_count) {
        yscroll = selection += choices_lines;
      }else{
        selection = choices_count - 1;
      }
      break;
    case PAGE_UP:
      if (selection > choices_lines) {
        yscroll = selection -= choices_lines;
      }else{
        yscroll = selection = 0;
      }
      break;
    case END:
      if (choices_count > 0) {
        selection = choices_count - 1;
      }
      break;
    case HOME:
      yscroll = selection = 0;
      break;
    case PRINTABLE:
      length = strlen(buf);

      if (query_length + length >= query_size) {
        query_size = 2 * query_length + length;
        if ((query = reallocarray(query, query_size,
                                  sizeof(char))) == NULL) {
          err(1, NULL);
        }
      }

      if (cursor_position < query_length) {
        memmove(query + cursor_position + length,
                query + cursor_position,
                query_length - cursor_position);
      }

      memcpy(query + cursor_position, buf, length);
      cursor_position    += length;
      query_length       += length;
      query[query_length] = '\0';
      dofilter            = query_grew = 1;
      break;
    case UNKNOWN:
      break;
    } /* switch */
  }
}     /* selected_choice */

/*
 * Filter the first nchoices number of choices using the current query and
 * regularly check for new user input in order to abort filtering. This
 * improves the performance when the cardinality of the choices is large.
 * Returns non-zero if the filtering was not aborted.
 */
int filter_choices(size_t nchoices) {
  struct choice_t *c;
  struct pollfd   pfd;
  size_t          i, match_length;
  int             nready;

  for (i = 0; i < nchoices; i++) {
    c = &choices.v[i];
    if (min_match(c->string, 0,
                  &c->match_start, &c->match_end) == INT_MAX) {
      c->match_start = c->match_end = -1;
      c->score       = 0;
    } else if (!sort) {
      c->score = 1;
    } else {
      match_length = c->match_end - c->match_start;
      c->score     = (double)query_length / match_length / c->length;
    }

    if (i > 0 && i % 50 == 0) {
      pfd.fd     = fileno(tty_in);
      pfd.events = POLLIN;
      if ((nready = poll(&pfd, 1, 0)) == -1) {
        err(1, "poll");
      }
      if (nready == 1 && pfd.revents & (POLLIN | POLLHUP)) {
        return(0);
      }
    }
  }
  qsort(choices.v, nchoices, sizeof(struct choice_t), choicecmp);

  return(1);
}

int choicecmp(const void *p1, const void *p2) {
  const struct choice_t *c1, *c2;

  c1 = p1;
  c2 = p2;
  if (c1->score < c2->score) {
    return(1);
  }
  if (c1->score > c2->score) {
    return(-1);
  }
  /*
   * The two choices have an equal score.
   * Sort based on the address of string since it reflects the initial
   * input order.
   * The comparison is inverted since the choice with the lowest address
   * must come first.
   */
  if (c1->string < c2->string) {
    return(-1);
  }
  if (c1->string > c2->string) {
    return(1);
  }
  return(0);
}

size_t min_match(const char *string, size_t offset, ssize_t *start, ssize_t *end) {
  const char *e, *q, *s;
  size_t     length;

  q = query;
  if (*q == '\0' || (s = e = strcasechr(&string[offset], q)) == NULL) {
    return(INT_MAX);
  }

  for ( ;;) {
    for (e++, q++; isu8cont(*q); e++, q++) {
      continue;
    }
    if (*q == '\0') {
      break;
    }
    if ((e = strcasechr(e, q)) == NULL) {
      return(INT_MAX);
    }
  }

  length = e - s;
  /* LEQ is used to obtain the shortest left-most match. */
  if (length == query_length
      || length <= min_match(string, s - string + 1, start, end)) {
    *start = s - string;
    *end   = e - string;
  }

  return(*end - *start);
}

/*
 * Returns a pointer to first occurrence of the first character in s2 in s1 with
 * respect to Unicode characters disregarding case.
 */
const char *strcasechr(const char *s1, const char *s2) {
  wchar_t wc1, wc2;
  size_t  i;
  int     nbytes;

  if (xmbtowc(&wc2, s2) == 0) {
    return(NULL);
  }

  for (i = 0; s1[i] != '\0';) {
    if ((nbytes = skipescseq(s1 + i)) > 0) {/* A match inside an escape sequence is ignored. */
    }else if ((nbytes = xmbtowc(&wc1, s1 + i)) == 0) {
      nbytes = 1;
    }else if (wcsncasecmp(&wc1, &wc2, 1) == 0) {
      return(s1 + i);
    }
    i += nbytes;
  }

  return(NULL);
}

/*
 * Returns the length of a CSI or OSC escape sequence located at the beginning
 * of str.
 */
size_t skipescseq(const char *str) {
  size_t i;
  int    csi = 0;
  int    osc = 0;

  if (str[0] == '\033' && str[1] == '[') {
    csi = 1;
  }else if (str[0] == '\033' && str[1] == ']') {
    osc = 1;
  }else{
    return(0);
  }

  for (i = 2; str[i] != '\0'; i++) {
    if ((csi && str[i] >= '@' && str[i] <= '~')
        || (osc && str[i] == '\a')) {
      break;
    }
  }

  return(i + 1);
}

void tty_init(int doinit) {
  struct termios new_attributes;

  if (doinit && (tty_in = fopen("/dev/tty", "r")) == NULL) {
    err(1, "fopen");
  }

  tcgetattr(fileno(tty_in), &tio);
  new_attributes                = tio;
  new_attributes.c_iflag       |= ICRNL;        /* map CR to NL */
  new_attributes.c_lflag       &= ~(ECHO | ICANON | IEXTEN | ISIG);
  new_attributes.c_cc[VMIN]     = 1;
  new_attributes.c_cc[VTIME]    = 0;
  new_attributes.c_cc[VDISCARD] = _POSIX_VDISABLE;
  tcsetattr(fileno(tty_in), TCSANOW, &new_attributes);

  if (doinit && (tty_out = fopen("/dev/tty", "w")) == NULL) {
    err(1, "fopen");
  }

  if (doinit) {
    setupterm((char *)0, fileno(tty_out), (int *)0);
  }

  tty_size();

  if (use_keypad) {
    tty_putp(keypad_xmit, 0);
  }
  if (use_alternate_screen) {
    tty_putp(enter_ca_mode, 0);
  }

  toggle_sigwinch(0);
}

int tty_putc(int c) {
  if (putc(c, tty_out) == EOF) {
    err(1, "putc");
  }

  return(c);
}

void handle_sigwinch(int sig) {
  gotsigwinch = sig == SIGWINCH;
}

void toggle_sigwinch(int enable) {
  struct sigaction sa;

  sa.sa_flags   = 0;
  sa.sa_handler = enable ? handle_sigwinch : SIG_IGN;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGWINCH, &sa, NULL) == -1) {
    err(1, "sigaction: SIGWINCH");
  }
}

void tty_restore(int doclose) {
  tcsetattr(fileno(tty_in), TCSANOW, &tio);
  if (doclose) {
    fclose(tty_in);
  }

  tty_putp(carriage_return, 1);         /* move cursor to first column */
  tty_putp(clr_eos, 1);

  if (use_keypad) {
    tty_putp(keypad_local, 0);
  }
  if (use_alternate_screen) {
    tty_putp(exit_ca_mode, 0);
  }

  if (doclose) {
    fclose(tty_out);
  }else{
    fflush(tty_out);
  }
}

void tty_size(void) {
  struct winsize ws;
  const char     *cp;
  int            sz;

  if (ioctl(fileno(tty_in), TIOCGWINSZ, &ws) != -1) {
    tty_columns = ws.ws_col;
    tty_lines   = ws.ws_row;
  }

  if (tty_columns == 0 && (sz = tigetnum("cols")) > 0) {
    tty_columns = sz;
  }
  if (tty_lines == 0 && (sz = tigetnum("lines")) > 0) {
    tty_lines = sz;
  }

  if ((cp = getenv("COLUMNS")) != NULL
      && (sz = strtonum(cp, 1, INT_MAX, NULL)) > 0) {
    tty_columns = sz;
  }
  if ((cp = getenv("LINES")) != NULL
      && (sz = strtonum(cp, 1, INT_MAX, NULL)) > 0) {
    tty_lines = sz;
  }

  if (tty_columns == 0) {
    tty_columns = 80;
  }
  if (tty_lines == 0) {
    tty_lines = 24;
  }

  choices_lines = tty_lines - 1;        /* available lines, minus query line */
}

void print_line(const char *str, size_t len, int flags, ssize_t enter_underline,
                ssize_t exit_underline){
  size_t       i;
  wchar_t      wc;
  unsigned int col;
  int          nbytes, width;

  if (flags & PL_MARKED) {        // *commander uses yellow...
    tty_putp(enter_bold_mode, 1); // it works but i liked without it
    tty_putp(tty_parm1(set_a_foreground, 3), 1);
  }

  if (flags & PL_STDOUT) {
    tty_putp(enter_standout_mode, 1);
  }

  col = i = 0;
  while (col < tty_columns) {
    if (enter_underline == (ssize_t)i) {
      tty_putp(enter_underline_mode, 1);
    }else if (exit_underline == (ssize_t)i) {
      tty_putp(exit_underline_mode, 1);
    }
    if (i == len) {
      break;
    }

    if (str[i] == '\t') {
      width = 8 - (col & 7);
      if (col + width > tty_columns) {
        break;
      }
      col += width;

      for ( ; width > 0; width--) {
        tty_putc(' ');
      }

      i++;
      continue;
    }

    /*
     * A NUL will be present prior the NUL-terminator if
     * descriptions are enabled.
     */
    if (str[i] == '\0') {
      tty_putc(' ');
      i++;
      col++;
      continue;
    }

    /*
     * Output every character, even invalid ones and escape
     * sequences. Even tho they don't occupy any columns.
     */
    if ((nbytes = skipescseq(str + i)) > 0) {
      width = 0;
    } else if ((nbytes = xmbtowc(&wc, str + i)) == 0) {
      nbytes = 1;
      width  = 0;
    } else if ((width = wcwidth(wc)) < 0) {
      width = 0;
    }

    if (col + width > tty_columns) {
      break;
    }
    col += width;

    for ( ; nbytes > 0; nbytes--, i++) {
      tty_putc(str[i]);
    }
  }
  for ( ; col < tty_columns; col++) {
    tty_putc(' ');
  }

  /*
   * If exit_underline is greater than columns the underline attribute
   * will spill over on the next line unless all attributes are exited.
   */
  tty_putp(exit_attribute_mode, 1);
} /* print_line */

/*
 * Output as many choices as possible starting from offset and return the number
 * of choices with a positive score. If the query is empty, all choices are
 * considered having a positive score.
 */
size_t print_choices(size_t offset, size_t selection) {
  const struct choice_t *choice;
  size_t                i;

  for (i = offset; i < choices.length; i++) {
    choice = choices.v + i;
    if (choice->score == 0 && query_length > 0) {
      break;
    }

    if (i - offset < choices_lines) {
      print_line(choice->string, choice->length,
                 ((i == selection) ? PL_STDOUT : 0) | \
                 ((choice->mark) ? PL_MARKED : 0),
                 choice->match_start,
                 choice->match_end);
    }
  }

  if (i - offset < choices.length && i - offset < choices_lines) {
    /*
     * Printing the choices did not consume all available
     * lines and there could still be choices left from the
     * last print in the lines not yet consumed.
     *
     * The clr_eos capability clears the screen from the
     * current column to the end. If the last visible choice
     * is selected, the standout in the last and current
     * column will be also be cleared. Therefore, move down
     * one line before clearing the screen.
     */
    tty_putc('\n');
    tty_putp(clr_eos, 1);
    tty_putp(tty_parm1(parm_up_cursor, (i - offset) + 1), 1);
  } else if (i > 0) {
    /*
     * parm_up_cursor interprets 0 as 1, therefore only move
     * upwards if any choices where printed.
     */
    tty_putp(tty_parm1(parm_up_cursor,
                       i < choices_lines ? i : choices_lines), 1);
  }

  return(i);
}

enum key get_key(const char **key) {
#define CAP(k, s)    { k, s, NULL, 0, -1 }
#define KEY(k, s)    { k, NULL, s, sizeof(s) - 1, -1 }
#define TIO(k, i)    { k, NULL, NULL, 0, i }
  static struct {
    enum key key;
    char *cap;
    const char *str;
    size_t len;
    int tio;
  } keys[] = {
    KEY(ALT_ENTER,
        "\033\n"),
    KEY(BACKSPACE,
        "\177"),
    KEY(BACKSPACE,
        "\b"),
    KEY(CTRL_A,
        "\001"),
    TIO(CTRL_C,
        VINTR),
    KEY(CTRL_E,
        "\005"),
    KEY(CTRL_K,
        "\013"),
    KEY(CTRL_L,
        "\014"),
    KEY(CTRL_O,
        "\017"),
    KEY(CTRL_U,
        "\025"),
    KEY(TAB,
        "\t"),
    KEY(CTRL_T,
        "\024"),
    KEY(CTRL_W,
        "\027"),
    KEY(CTRL_W,
        "\033\177"),
    KEY(CTRL_W,
        "\033\b"),
    TIO(CTRL_Z,
        VSUSP),
    CAP(DEL,
        "kdch1"),
    KEY(DEL,
        "\004"),
    CAP(END,
        "kend"),
    KEY(END,
        "\033>"),
    KEY(ENTER,
        "\n"),
    CAP(ENTER,
        "kent"),
    CAP(HOME,
        "khome"),
    KEY(HOME,
        "\033<"),
    CAP(LEFT,
        "kcub1"),
    KEY(LEFT,
        "\002"),
    KEY(LEFT,
        "\033OD"),
    CAP(LINE_DOWN,
        "kcud1"),
    KEY(LINE_DOWN,
        "\016"),
    KEY(LINE_DOWN,
        "\033OB"),
    CAP(LINE_UP,
        "kcuu1"),
    KEY(LINE_UP,
        "\020"),
    KEY(LINE_UP,
        "\033OA"),
    CAP(PAGE_DOWN,
        "knp"),
    KEY(PAGE_DOWN,
        "\026"),
    KEY(PAGE_DOWN,
        "\033 "),
    CAP(PAGE_UP,
        "kpp"),
    KEY(PAGE_UP,
        "\033v"),
    CAP(RIGHT,
        "kcuf1"),
    KEY(RIGHT,
        "\006"),
    KEY(RIGHT,
        "\033OC"),
    KEY(UNKNOWN,
        NULL),
  };
  static unsigned char buf[8];
  size_t len;
  int c,
  i;

  memset(buf,
         0,
         sizeof(buf));
  *key = (const char *)buf;
  len  = 0;

  /*
   * Allow SIGWINCH on the first read. If the signal is received, return
   * CTRL_L which will trigger a resize.
   */
  toggle_sigwinch(1);
  buf[len++] = tty_getc();
  toggle_sigwinch(0);
  if (gotsigwinch) {
    gotsigwinch = 0;
    return(CTRL_L);
  }

  for ( ;;) {
    for (i = 0; keys[i].key != UNKNOWN; i++) {
      if (keys[i].tio >= 0) {
        if (len == 1
            && tio.c_cc[keys[i].tio] == *buf
            && tio.c_cc[keys[i].tio] != _POSIX_VDISABLE) {
          return(keys[i].key);
        }
        continue;
      }

      if (keys[i].str == NULL) {
        keys[i].str = tty_getcap(keys[i].cap);
        keys[i].len = strlen(keys[i].str);
      }
      if (strncmp(keys[i].str,
                  *key,
                  len) != 0) {
        continue;
      }

      if (len == keys[i].len) {
        return(keys[i].key);
      }

      /* Partial match found, continue reading. */
      break;
    }
    if (keys[i].key == UNKNOWN) {
      break;
    }

    if (len == sizeof(buf) - 1) {
      break;
    }
    buf[len++] = tty_getc();
  }

  if (len > 1 && buf[0] == '\033' && (buf[1] == '[' || buf[1] == 'O')) {
    /*
     * An escape sequence which is not a supported key is being
     * read. Discard the rest of the sequence.
     */
    c = buf[len - 1];
    while (c < '@' || c > '~') {
      c = tty_getc();
    }

    return(UNKNOWN);
  }

  if (!isu8start(buf[0])) {
    if (isprint(buf[0])) {
      return(PRINTABLE);
    }

    return(UNKNOWN);
  }

  /*
   * Ensure a whole Unicode character is read. The number of MSBs in the
   * first octet of a Unicode character is equal to the number of octets
   * the character consists of, followed by a zero. Therefore, as long as
   * the MSB is not zero there is still bytes left to read.
   */
  for ( ;;) {
    if (((buf[0] << len) & 0x80) == 0) {
      break;
    }
    if (len == sizeof(buf) - 1) {
      return(UNKNOWN);
    }

    buf[len++] = tty_getc();
  }

  return(PRINTABLE);
} /* get_key */

int tty_getc(void) {
  int c;

  if ((c = getc(tty_in)) == ERR && !gotsigwinch) {
    err(1, "getc");
  }

  return(c);
}

const char *tty_getcap(char *cap) {
  char *str;

  str = tigetstr(cap);
  if (str == (char *)(-1) || str == NULL) {
    return("");
  }

  return(str);
}

const char *tty_parm1(char *cap, int a) {
  return(tparm(cap, a, 0, 0, 0, 0, 0, 0, 0, 0));
}

void delete_between(char *string, size_t length, size_t start, size_t end) {
  memmove(string + start, string + end, length - end + 1);
}

int isu8cont(unsigned char c) {
  return(MB_CUR_MAX > 1 && (c & (0x80 | 0x40)) == 0x80);
}

int isu8start(unsigned char c) {
  return(MB_CUR_MAX > 1 && (c & (0x80 | 0x40)) == (0x80 | 0x40));
}

int isword(const char *s) {
  wchar_t wc;

  if (xmbtowc(&wc, s) == 0) {
    return(0);
  }

  return(iswalnum(wc) || wc == L'_');
}

int xmbtowc(wchar_t *wc, const char *s) {
  int n;

  n = mbtowc(wc, s, MB_CUR_MAX);
  if (n == -1) {
    /* Assign in order to suppress ignored return value warning. */
    n = mbtowc(NULL, NULL, MB_CUR_MAX);
    return(0);
  }

  return(n);
}
