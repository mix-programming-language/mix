#include "lex.h"
#include "context.h"
#include "parser.h"
#include "value.h"
#include "block.h"
#include "member.h"
#include "runtime_stack_item.h"
#include "cutils/hash_func.h"
#include "cutils/utils.h"
#include <stdlib.h> /* malloc() */

/* -------------------------------------------------------------------------- */

struct prefix_var {
    struct qbuf prefix;
    struct variable* var;
};

static int prefix_var_equal(const void* a, const void* b) {
    const struct qbuf_ref* prefix_a = (const struct qbuf_ref*)prefix_a;
    const struct qbuf_ref* prefix_b = (const struct qbuf_ref*)prefix_b;
    return qbuf_ref_equal(prefix_a, prefix_b);
}

static const void* prefix_var_getkey(const void* data) {
    const struct prefix_var* v = (const struct prefix_var*)data;
    return qbuf_get_ref(&v->prefix);
}

static unsigned long prefix_var_hash(const void* key) {
    const struct qbuf_ref* k = (const struct qbuf_ref*)key;
    return bkd_hash(k->base, k->size);
}

static const struct robin_hood_hash_operations g_prefix_var_ops = {
    .equal = prefix_var_equal,
    .getkey = prefix_var_getkey,
    .hash = prefix_var_hash,
};

static struct mix_type* create_type(const char* name, uint32_t name_sz,
                                    mix_var_type_t type, struct robin_hood_hash* type_hash) {
    struct mix_type* t = mix_type_new(type);
    if (!t) {
        return NULL;
    }
    qbuf_assign(&t->name, name, name_sz);

    void* ret = robin_hood_hash_insert(type_hash, t);
    if (!ret) {
        mix_type_delete(t);
        return NULL;
    }

    return t;
}

static mix_retcode_t add_builtin_type(struct robin_hood_hash* type_hash) {
    struct mix_type* type = create_type("i8", 2, MIX_VARTYPE_I8, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i16", 3, MIX_VARTYPE_I16, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i32", 3, MIX_VARTYPE_I32, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i64", 3, MIX_VARTYPE_I64, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("f32", 3, MIX_VARTYPE_F32, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("f64", 3, MIX_VARTYPE_F64, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("str", 3, MIX_VARTYPE_STR, type_hash);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    return MIX_RC_OK;
}

struct mix_context* mix_context_new() {
    struct mix_context* ctx = (struct mix_context*)malloc(sizeof(struct mix_context));
    if (!ctx) {
        return NULL;
    }

    int err = robin_hood_hash_init(&ctx->prefix_var_hash, 10,
                                   ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR, &g_prefix_var_ops);
    if (err) {
        goto err1;
    }

    struct mix_block* root_block = mix_block_new();
    if (!root_block) {
        goto err2;
    }

    mix_retcode_t rc = add_builtin_type(&root_block->type_hash);
    if (rc != MIX_RC_OK) {
        goto err3;
    }

    vector_init(&ctx->runtime_stack, sizeof(struct runtime_stack_item));

    vector_init(&ctx->block_stack, sizeof(struct mix_block*));
    vector_push_back(&ctx->block_stack, &root_block);

    return ctx;

err3:
    mix_block_delete(root_block);
err2:
    robin_hood_hash_destroy(&ctx->prefix_var_hash);
err1:
    free(ctx);
    return NULL;
}

static void destroy_block_func(void* item, void* nil) {
    struct mix_block* b = (struct mix_block*)(*(void**)item);
    mix_block_delete(b);
}

void mix_context_delete(struct mix_context* ctx) {
    if (ctx) {
        vector_destroy(&ctx->block_stack, NULL, destroy_block_func);
        vector_destroy(&ctx->runtime_stack, NULL, NULL); /* TODO */
        free(ctx);
    }
}

/* -------------------------------------------------------------------------- */

static mix_retcode_t push_integer(struct mix_context* ctx, int64_t value, struct qbuf_ref* tname) {
    struct mix_value* v = mix_value_create();
    if (!v) {
        return MIX_RC_NOMEM;
    }

    v->l = value;
    v->type = mix_parser_lookup_type(ctx, tname);

    struct runtime_stack_item item = {
        .type = STACK_ITEM_TYPE_VALUE,
        .v = v,
    };
    vector_push_back(&ctx->runtime_stack, &item);

    return MIX_RC_OK;
}

mix_retcode_t mix_context_push_i8(struct mix_context* ctx, int8_t value) {
    struct qbuf_ref tname = { .base = "i8", .size = 2 };
    return push_integer(ctx, value, &tname);
}

mix_retcode_t mix_context_push_i16(struct mix_context* ctx, int16_t value) {
    struct qbuf_ref tname = { .base = "i16", .size = 3 };
    return push_integer(ctx, value, &tname);
}

mix_retcode_t mix_context_push_i32(struct mix_context* ctx, int32_t value) {
    struct qbuf_ref tname = { .base = "i32", .size = 3 };
    return push_integer(ctx, value, &tname);
}

mix_retcode_t mix_context_push_i64(struct mix_context* ctx, int64_t value) {
    struct qbuf_ref tname = { .base = "i64", .size = 3 };
    return push_integer(ctx, value, &tname);
}

static mix_retcode_t push_float(struct mix_context* ctx, double value, struct qbuf_ref* tname) {
    struct mix_value* v = mix_value_create();
    if (!v) {
        return MIX_RC_NOMEM;
    }

    v->d = value;
    v->type = mix_parser_lookup_type(ctx, tname);

    struct runtime_stack_item item = {
        .type = STACK_ITEM_TYPE_VALUE,
        .v = v,
    };
    vector_push_back(&ctx->runtime_stack, &item);

    return MIX_RC_OK;
}

mix_retcode_t mix_context_push_f32(struct mix_context* ctx, float value) {
    struct qbuf_ref tname = { .base = "f32", .size = 3 };
    return push_float(ctx, value, &tname);
}

mix_retcode_t mix_context_push_f64(struct mix_context* ctx, double value) {
    struct qbuf_ref tname = { .base = "f64", .size = 3 };
    return push_float(ctx, value, &tname);
}

mix_retcode_t mix_context_push_str(struct mix_context* ctx, const char* str, uint32_t len) {
    struct mix_value* v = mix_value_create();
    if (!v) {
        return MIX_RC_NOMEM;
    }

    qbuf_assign(&v->s, str, len);
    struct qbuf_ref tname = { .base = "str", .size = 3 };
    v->type = mix_parser_lookup_type(ctx, &tname);

    struct runtime_stack_item item = {
        .type = STACK_ITEM_TYPE_VALUE,
        .v = v,
    };
    vector_push_back(&ctx->runtime_stack, &item);

    return MIX_RC_OK;
}

mix_retcode_t mix_context_push_func(struct mix_context* ctx, const char* func_type_name,
                                    int (*f)(struct mix_context*)) {
    return MIX_RC_NOT_IMPLEMENTED;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_new_func_begin(struct mix_context* ctx) {
    struct mix_type* t = mix_type_new(MIX_VARTYPE_FUNC);
    if (!t) {
        return MIX_RC_NOMEM;
    }

    struct runtime_stack_item item = {
        .type = STACK_ITEM_TYPE_TYPE,
        .t = t,
    };
    vector_push_back(&ctx->runtime_stack, &item);

    return MIX_RC_OK;
}

mix_retcode_t mix_context_new_func_set_returned_type(struct mix_context* ctx,
                                                     const char* type_name) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    struct mix_type_func* type = container_of(item->t, struct mix_type_func, t);

    struct qbuf_ref qname = { .base = type_name, .size = strlen(type_name) };
    type->ret_type = mix_parser_lookup_type(ctx, &qname);
    if (!type->ret_type) {
        return MIX_RC_NOT_FOUND;
    }

    return MIX_RC_OK;
}

mix_retcode_t mix_context_new_func_add_arg_str(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    struct mix_type_func* func = container_of(item->t, struct mix_type_func, t);
    struct qbuf_ref tname = { .base = "str", .size = 3 };
    struct mix_type* t = mix_parser_lookup_type(ctx, &tname);
    vector_push_back(&func->arg_type_list, &t);
    return MIX_RC_OK;
}

mix_retcode_t mix_context_new_func_end(struct mix_context* ctx, mix_func_t func) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    struct mix_block* root_block = mix_parser_get_root_block(ctx);

    struct mix_type* anom_type = item->t;
    vector_push_back(&root_block->anomymous_type, item->t);
    vector_pop_back(&ctx->runtime_stack, NULL); /* pops the function type */

    struct mix_value* v = mix_value_create();
    if (!v) {
        return MIX_RC_NOMEM;
    }
    v->type = anom_type;
    v->func = func;

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

int8_t mix_context_to_i8(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->l;
}

int16_t mix_context_to_i16(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->l;
}

int32_t mix_context_to_i32(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->l;
}

int64_t mix_context_to_i64(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->l;
}

float mix_context_to_f32(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->d;
}

double mix_context_to_f64(struct mix_context* ctx) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    return item->v->d;
}

const char* mix_context_to_str(struct mix_context* ctx, uint32_t* len) {
    struct runtime_stack_item* item = vector_back(&ctx->runtime_stack);
    if (len) {
        *len = qbuf_size(&item->v->s);
    }
    return qbuf_data(&item->v->s);
}

/* -------------------------------------------------------------------------- */

#include "mix.tab.h"

mix_retcode_t mix_context_eval(struct mix_context* ctx, const char* buf, uint32_t sz) {
    struct mix_lex lex;
    mix_lex_init(&lex, buf, sz);
    yyparse(ctx, &lex, buf, sz);
    mix_lex_destroy(&lex);
    return MIX_RC_OK;
}
