#include "type.h"
#include "block.h"
#include "variable.h"
#include "parser.h"
#include "debug_utils.h"
#include "cutils/robin_hood_hash.h"
#include <stdlib.h> /* malloc() */

/* -------------------------------------------------------------------------- */

struct type_lookup_arg {
    const struct qbuf_ref* tname;
    struct mix_type* res;
};

static int lookup_type_func(void* item, void* arg_for_callback) {
    struct mix_block* block = (struct mix_block*)(*(void**)item);
    struct type_lookup_arg* targ = (struct type_lookup_arg*)arg_for_callback;
    targ->res = (struct mix_type*)robin_hood_hash_lookup(&block->type_hash, targ->tname);
    return (targ->res) ? 1 : 0;
}

struct mix_type* mix_parser_lookup_type(struct mix_context* ctx, const struct qbuf_ref* tname) {
    struct type_lookup_arg targ = { .tname = tname, .res = NULL };
    vector_foreach(&ctx->block_stack, &targ, lookup_type_func);
    return targ.res;
}

/* -------------------------------------------------------------------------- */

struct mix_variable* mix_parser_new_variable(struct mix_context* ctx, const struct qbuf_ref* var_name,
                                              struct mix_type* type) {
    struct mix_variable* var = mix_variable_new();
    if (!var) {
        return NULL;
    }

    qbuf_assign(&var->name, var_name->base, var_name->size);
    var->type = type;

    struct mix_block* root_block = (struct mix_block*)(*(void**)vector_front(&ctx->block_stack));
    const void* ret_var = robin_hood_hash_insert(&root_block->var_hash, var);
    if (ret_var != var) {
        mix_variable_delete(var);
        return NULL;
    }

    return var;
}

/* -------------------------------------------------------------------------- */

struct var_lookup_arg {
    const struct qbuf_ref* vname;
    struct mix_variable* res;
};

static int lookup_variable_func(void* item, void* arg_for_callback) {
    struct mix_block* b = (struct mix_block*)(*(void**)item);
    struct var_lookup_arg* varg = (struct var_lookup_arg*)arg_for_callback;
    varg->res = (struct mix_variable*)robin_hood_hash_lookup(&b->var_hash, varg->vname);
    return (varg->res) ? 1 : 0;
}

struct mix_variable* mix_parser_lookup_variable(struct mix_context* ctx, const struct qbuf_ref* var_name) {
    struct var_lookup_arg varg = { .vname = var_name, .res = NULL };
    vector_foreach(&ctx->block_stack, &varg, lookup_variable_func);
    return varg.res;
}

/* -------------------------------------------------------------------------- */

struct mix_block* mix_parser_get_root_block(struct mix_context* ctx) {
    return (struct mix_block*)(*((void**)vector_front(&ctx->block_stack)));
}
