#include "c_vector/include/vector.h"
#include "libpick/libpick.h"

int main(int argc, char *argv[]){
  struct pick_ctx_t *CTX = pick_init_ctx();

  vector_push(CTX->choices_s_v, "opt 0");
  vector_push(CTX->choices_s_v, "opt 1 | opt 1 desc");
  vector_push(CTX->choices_s_v, "opt 2 | opt 2 desc");
  vector_push(CTX->choices_s_v, "opt 3 | opt 3 desc");
  vector_push(CTX->choices_s_v, "opt 4 | opt 4 desc");

  struct Vector *picked_v = do_pick_multiple(CTX);

  if (picked_v) {
    fprintf(stdout, "You picked %lu options: ",
            vector_size(picked_v)
            );
    for (size_t i = 0; i < vector_size(picked_v); i++) {
      fprintf(stdout, "%s, ",
              (char *)vector_get(picked_v, i)
              );
    }
    fprintf(stdout, "\n");
  }else{
    fprintf(stdout, "You didn't pick any options\n");
  }
}
