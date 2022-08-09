#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/////////////////////////////////////////////////////////////////////////////
#include "libpick/pty.h"
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
struct choices_t {
  size_t          size;
  size_t          length;
  struct choice_t *v;
};
struct choice_t {
  char    *description;
  char    *string;
  size_t  length;
  ssize_t match_start;                  /* inclusive match start offset */
  ssize_t match_end;                    /* exclusive match end offset */
  double  score;
  int     mark;
};
struct pick_ctx_t {
  struct choices_t choices;
  struct choice_t  choice;
  struct Vector    *choices_s_v;
  char             description_seperator;
};
/////////////////////////////////////////////////////////////////////////////
char *do_pick(struct pick_ctx_t *CTX);
struct pick_ctx_t *pick_init_ctx(void);

/////////////////////////////////////////////////////////////////////////////
