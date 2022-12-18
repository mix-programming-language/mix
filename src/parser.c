#include "type.h"
#include "block.h"
#include "identifier.h"
#include "lib_info.h"
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
    struct type_lookup_arg targ = {.tname = tname, .res = NULL};
    vector_foreach(&ctx->block_stack, &targ, lookup_type_func);
    return targ.res;
}

/* -------------------------------------------------------------------------- */

struct mix_identifier* mix_parser_new_identifier(struct mix_context* ctx, const struct qbuf_ref* id_name) {
    struct mix_identifier* var = mix_identifier_new();
    if (!var) {
        return NULL;
    }

    struct mix_block* root_block = __get_root_block(ctx);
    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(
        &root_block->var_hash, id_name, var);
    if (!res.inserted) {
        mix_identifier_delete(var);
        return NULL;
    }

    qbuf_assign(&var->name, id_name->base, id_name->size);
    var->tov.type = MIX_TOV_UNKNOWN;
    return var;
}

/* -------------------------------------------------------------------------- */

struct var_lookup_arg {
    const struct qbuf_ref* name;
    struct mix_identifier* res;
};

static int lookup_variable_func(void* item, void* arg_for_callback) {
    struct mix_block* b = (struct mix_block*)(*(void**)item);
    struct var_lookup_arg* varg = (struct var_lookup_arg*)arg_for_callback;
    varg->res = (struct mix_identifier*)robin_hood_hash_lookup(&b->var_hash, varg->name);
    return (varg->res) ? 1 : 0;
}

struct mix_identifier* mix_parser_lookup_identifier(struct mix_context* ctx, const struct qbuf_ref* id_name) {
    struct var_lookup_arg varg = {.name = id_name, .res = NULL};
    vector_foreach(&ctx->block_stack, &varg, lookup_variable_func);
    return varg.res;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_parser_push_tov(struct mix_context* ctx, struct mix_type_or_value* tov) {
    vector_resize(&ctx->runtime_stack, vector_size(&ctx->runtime_stack) + 1, NULL, NULL);
    mix_type_or_value_copy_construct(tov, vector_back(&ctx->runtime_stack));
    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

struct load_lib_arg {
#ifndef NDEBUG
    struct mix_context* dbg_ctx;
#endif
    const struct qbuf_ref* prefix;
    struct mix_block* block;
};

static int load_lib_func(void* item, void* arg) {
    struct load_lib_arg* larg = (struct load_lib_arg*)arg;
    struct mix_identifier* var = (struct mix_identifier*)(*(void**)item);

    struct mix_identifier* new_var = mix_identifier_new();
    if (!new_var) {
        return MIX_RC_NOMEM;
    }

    qbuf_assign(&new_var->name, larg->prefix->base, larg->prefix->size);
    qbuf_append(&new_var->name, qbuf_data(&var->name), qbuf_size(&var->name));

    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(
        &larg->block->var_hash, qbuf_get_ref(&new_var->name), NULL);
    if (!res.pvalue) {
        return MIX_RC_NOMEM;
    }
    if (!res.inserted) {
        mix_identifier_delete(new_var);
        return MIX_RC_EXISTS;
    }

#ifndef NDEBUG
    struct mix_type* dbg_var_type = mix_type_or_value_get_type(&var->tov);
    uint32_t dbg_refcount_before = dbg_var_type->refcount;
#endif

    mix_type_or_value_copy_construct(&var->tov, &new_var->tov);

#ifndef NDEBUG
    uint32_t dbg_refcount_after = dbg_var_type->refcount;
    if (dbg_refcount_after != dbg_refcount_before + 1) {
        logger_error(larg->dbg_ctx->logger, "type [%s] refcount before [%u] != after [%u + 1].",
                     make_tmp_str_s(qbuf_get_ref(&dbg_var_type->name)), dbg_refcount_before,
                     dbg_refcount_after);
        return MIX_RC_INVALID;
    }
#endif

    *res.pvalue = new_var;

    return 0;
}

mix_retcode_t mix_parser_load_lib(struct mix_context* ctx, const struct qbuf_ref* prefix,
                                  const struct qbuf_ref* alias) {
    struct mix_lib_info* info = robin_hood_hash_lookup(&ctx->libs, prefix);
    if (!info) {
        logger_error(ctx->logger, "load lib [%s] failed: not found.", prefix);
        return MIX_RC_NOT_FOUND;
    }

    if (alias) {
        prefix = alias;
    }

    struct load_lib_arg larg = {
#ifndef NDEBUG
        .dbg_ctx = ctx,
#endif
        .prefix = prefix,
        .block = __get_current_block(ctx),
    };
    vector_foreach(&info->var_list, &larg, load_lib_func);

    return MIX_RC_OK;
}
