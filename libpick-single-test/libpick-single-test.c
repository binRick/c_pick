#include "c_vector/include/vector.h"
#include "libpick/libpick.h"


int main(int argc, char *argv[]){
  struct pick_ctx_t *CTX = pick_init_ctx();

  vector_push(CTX->choices_s_v, "opt 0");
  vector_push(CTX->choices_s_v, "opt 1 | opt 1 desc");
  vector_push(CTX->choices_s_v, "opt 2 | opt 2 desc");
  vector_push(CTX->choices_s_v, "opt 3 | opt 3 desc");
  vector_push(CTX->choices_s_v, "opt 4 | opt 4 desc");

  char *picked = do_pick_single(CTX);

  if (picked) {
    fprintf(stdout, "You picked '%s'\n", picked);
  }else{
    fprintf(stdout, "You didn't pick an option\n");
  }
}

