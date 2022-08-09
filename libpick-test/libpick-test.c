#include "libpick/libpick.h"
#include "c_vector/include/vector.h"


int main(int argc, char *argv[]){
  struct pick_ctx_t *CTX = pick_init_ctx();
  vector_push(CTX->choices_s_v,"opt 1");
  vector_push(CTX->choices_s_v,"opt 2");
  vector_push(CTX->choices_s_v,"opt 3|opt 3 desc");

  char *picked = do_pick(CTX);
  if(picked)
      fprintf(stdout, "You picked '%s'\n", picked);
  else
      fprintf(stdout, "You didn't pick an option\n");
}

