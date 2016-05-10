#include <openssl/engine.h>
static const char *engine_id = "silly";
static const char *engine_name = "A silly engine for demonstration purposes";
static int bind_silly(ENGINE *e, const char *id){
  int ret = 0;
  if (!ENGINE_set_id(e, engine_id)){
    fprintf(stderr, "ENGINE_set_id failed\n");
    goto end;
  }
  if (!ENGINE_set_name(e, engine_name)){
    printf("ENGINE_set_name failed\n");
    goto end;
  }
  ret = 1;
 end:
  return ret;
}


IMPLEMENT_DYNAMIC_BIND_FN(bind_silly)
IMPLEMENT_DYNAMIC_CHECK_FN()