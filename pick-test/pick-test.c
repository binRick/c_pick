#include "pick/libpick.h"


int main(int argc, char *argv[]){
  struct pick_ctx_t *CTX = malloc(sizeof(struct pick_ctx_t));

  char              *picked = do_pick(CTX);

  fprintf(stdout, "You picked '%s'\n", picked);
}

