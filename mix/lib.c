#include "value.h"
#include "context.h"
#include "runtime_stack_item.h"

/* -------------------------------------------------------------------------- */

#include <stdio.h>

static int std_io_write(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    fwrite(qbuf_data(&item->v->s), qbuf_size(&item->v->s), 1, stdout);
    return 0;
}

mix_retcode_t mix_lib_new_std_io_write(struct mix_context* ctx) {
    return MIX_RC_NOT_IMPLEMENTED;
}

/* -------------------------------------------------------------------------- */
