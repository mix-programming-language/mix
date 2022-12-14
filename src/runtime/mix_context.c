#include "mix_context.h"
#include "mix_identifier.h"
#include "mix_shared_value.h"
#include "mix_type_or_value.h"
#include "mix_lib_info.h"
#include "parser/mix_parser.h"
#include "common/typedef_internal.h"
#include "misc/utils.h" /* read_file_content() */
#include "misc/qbuf_ref_hash_utils.h"
#include "cutils/utils.h" /* container_of() */
#include "logger/dummy_logger.h"
#include <stdlib.h> /* malloc() */

/* -------------------------------------------------------------------------- */

void mix_set_logger(struct mix_context* ctx, struct logger* l) {
    ctx->logger = l;
}

struct logger* mix_get_logger(struct mix_context* ctx) {
    return ctx->logger;
}

int32_t mix_get_stack_size(struct mix_context* ctx) {
    return vector_size(&ctx->runtime_stack) - ctx->runtime_stack_base_idx;
}

struct mix_type_or_value* __get_item(struct mix_context* ctx, int32_t idx) {
    int32_t stk_sz = mix_get_stack_size(ctx);
    if (idx < 0) {
        idx += stk_sz;
        if (idx < 0) {
            return NULL;
        }
    }
    return (struct mix_type_or_value*)vector_at(&ctx->runtime_stack,
                                                ctx->runtime_stack_base_idx + idx);
}

/* -------------------------------------------------------------------------- */

static const void* lib_info_getkey_func(const void* value) {
    const struct mix_lib_info* info = (const struct mix_lib_info*)value;
    return qbuf_get_ref(&info->name);
}

static const struct robin_hood_hash_operations g_lib_info_ops = {
    .equal = qbuf_ref_equal_func,
    .hash = qbuf_ref_hash_func,
    .getkey = lib_info_getkey_func,
};

static const void* type_hash_getkey_func(const void* data) {
    const struct mix_type* t = (const struct mix_type*)data;
    return qbuf_get_ref(&t->name);
}

static const struct robin_hood_hash_operations g_type_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = type_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

static const void* var_hash_getkey_func(const void* data) {
    const struct mix_identifier* var = (const struct mix_identifier*)data;
    return qbuf_get_ref(&var->name);
}

static const struct robin_hood_hash_operations g_var_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = var_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

static struct mix_type* create_type(const char* name, int32_t name_sz,
                                    mix_type_t type, struct robin_hood_hash* type_hash,
                                    struct logger* l) {
    struct mix_type* t = mix_type_create(type);
    if (!t) {
        logger_error(l, "create type [%s] failed.", name);
        return NULL;
    }
    qbuf_assign(&t->name, name, name_sz);

    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(
        type_hash, qbuf_get_ref(&t->name), t);
    if (!res.inserted) {
        mix_type_release(t);
        logger_error(l, "create type [%s] failed: out of memory.", name);
        return NULL;
    }

    mix_type_acquire(t); /* for hash table */
    return t;
}

static mix_retcode_t add_builtin_type(struct robin_hood_hash* type_hash, struct logger* l) {
    struct mix_type* type = create_type("i8", 2, MIX_TYPE_I8, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i16", 3, MIX_TYPE_I16, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i32", 3, MIX_TYPE_I32, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("i64", 3, MIX_TYPE_I64, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("f32", 3, MIX_TYPE_F32, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("f64", 3, MIX_TYPE_F64, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("str", 3, MIX_TYPE_STR, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    type = create_type("...", 3, MIX_TYPE_VARIADIC_ARG, type_hash, l);
    if (!type) {
        return MIX_RC_NOMEM;
    }

    return MIX_RC_OK;
}

static void __destroy_type_func(void* data, void* nil) {
    mix_type_release((struct mix_type*)data);
}

static void __destroy_var_func(void* data, void* nil) {
    mix_identifier_delete((struct mix_identifier*)data);
}

static void __destroy_libs_func(void* data, void* nil) {
    mix_lib_info_delete((struct mix_lib_info*)data);
}

struct mix_context* mix_context_new(struct logger* l) {
    struct mix_context* ctx = (struct mix_context*)malloc(sizeof(struct mix_context));
    if (!ctx) {
        return NULL;
    }

    dummy_logger_init(&ctx->dummy_logger);
    if (l) {
        ctx->logger = l;
    } else {
        ctx->logger = &ctx->dummy_logger;
    }

    int err = robin_hood_hash_init(&ctx->type_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                                   &g_type_hash_ops);
    if (err) {
        logger_error(ctx->logger, "init type hash failed.");
        goto err1;
    }

    err = robin_hood_hash_init(&ctx->var_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                               &g_var_hash_ops);
    if (err) {
        logger_error(ctx->logger, "init var hash failed.");
        goto err2;
    }

    err = robin_hood_hash_init(&ctx->libs, 10, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                               &g_lib_info_ops);
    if (err) {
        logger_error(ctx->logger, "init lib hash failed.");
        goto err3;
    }

    mix_retcode_t rc = add_builtin_type(&ctx->type_hash, ctx->logger);
    if (rc != MIX_RC_OK) {
        logger_error(ctx->logger, "reserve builtin type failed: %s.", mix_get_retcode_str(rc));
        goto err4;
    }

    ctx->runtime_stack_base_idx = 0;
    vector_init(&ctx->runtime_stack, sizeof(struct mix_type_or_value));

    return ctx;

err4:
    robin_hood_hash_destroy(&ctx->libs, NULL, NULL);
err3:
    robin_hood_hash_destroy(&ctx->var_hash, NULL, NULL);
err2:
    robin_hood_hash_destroy(&ctx->type_hash, NULL, __destroy_type_func);
err1:
    free(ctx);
    return NULL;
}

static void __destroy_tov_func(void* data, void* nil) {
    struct mix_type_or_value* tov = (struct mix_type_or_value*)data;
    mix_type_or_value_destroy(tov);
}

void mix_context_delete(struct mix_context* ctx) {
    if (ctx) {
        vector_destroy(&ctx->runtime_stack, NULL, __destroy_tov_func);
        robin_hood_hash_destroy(&ctx->var_hash, NULL, __destroy_var_func);
        robin_hood_hash_destroy(&ctx->type_hash, NULL, __destroy_type_func);
        robin_hood_hash_destroy(&ctx->libs, NULL, __destroy_libs_func);
        free(ctx);
    }
}

/* -------------------------------------------------------------------------- */

mix_type_t mix_get_type(struct mix_context* ctx, int32_t idx) {
    struct mix_type_or_value* tov = __get_item(ctx, idx);
    if (!tov) {
        return MIX_TYPE_NIL;
    }

    struct mix_type* type = mix_type_or_value_get_type(tov);
    return type->value;
}

static const struct qbuf_ref g_type2name[] = {
    {NULL, 0},
    {"i8", 2},
    {"i16", 3},
    {"i32", 3},
    {"i64", 3},
    {"f32", 3},
    {"f64", 3},
};

mix_retcode_t __push_integer(struct mix_context* ctx, int64_t value, mix_type_t t) {
    struct mix_type* type = robin_hood_hash_lookup(&ctx->type_hash, &g_type2name[t]);

    struct mix_type_or_value item = {
        .type = MIX_TOV_ATOMIC_VALUE,
        .t = type,
        .l = value,
    };

    int ret = vector_push_back(&ctx->runtime_stack, &item);
    if (ret != 0) {
        logger_error(ctx->logger, "push integer failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    mix_type_acquire(type);
    return MIX_RC_OK;
}

mix_retcode_t mix_push_i8(struct mix_context* ctx, int8_t value) {
    return __push_integer(ctx, value, MIX_TYPE_I8);
}

mix_retcode_t mix_push_i16(struct mix_context* ctx, int16_t value) {
    return __push_integer(ctx, value, MIX_TYPE_I16);
}

mix_retcode_t mix_push_i32(struct mix_context* ctx, int32_t value) {
    return __push_integer(ctx, value, MIX_TYPE_I32);
}

mix_retcode_t mix_push_i64(struct mix_context* ctx, int64_t value) {
    return __push_integer(ctx, value, MIX_TYPE_I64);
}

mix_retcode_t __push_float(struct mix_context* ctx, double value, mix_type_t t) {
    struct mix_type* type = robin_hood_hash_lookup(&ctx->type_hash, &g_type2name[t]);

    struct mix_type_or_value item = {
        .type = MIX_TOV_ATOMIC_VALUE,
        .t = type,
        .d = value,
    };

    int ret = vector_push_back(&ctx->runtime_stack, &item);
    if (ret != 0) {
        logger_error(ctx->logger, "push float failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    mix_type_acquire(type);
    return MIX_RC_OK;
}

mix_retcode_t mix_push_f32(struct mix_context* ctx, float value) {
    return __push_float(ctx, value, MIX_TYPE_F32);
}

mix_retcode_t mix_push_f64(struct mix_context* ctx, double value) {
    return __push_float(ctx, value, MIX_TYPE_F64);
}

mix_retcode_t mix_push_str(struct mix_context* ctx, const char* str, int32_t len) {
    struct mix_shared_value* v = mix_shared_value_create();
    if (!v) {
        return MIX_RC_NOMEM;
    }

    struct qbuf_ref tname = {.base = "str", .size = 3};
    v->type = robin_hood_hash_lookup(&ctx->type_hash, &tname);
    mix_type_acquire(v->type);
    qbuf_assign(&v->s, str, len);

    struct mix_type_or_value item = {
        .type = MIX_TOV_SHARED_VALUE,
        .v = v,
    };

    int ret = vector_push_back(&ctx->runtime_stack, &item);
    if (ret != 0) {
        logger_error(ctx->logger, "push str failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    mix_shared_value_acquire(v);
    return MIX_RC_OK;
}

mix_retcode_t mix_push_func(struct mix_context* ctx, const char* func_type_name,
                            int (*f)(struct mix_context*)) {
    return MIX_RC_NOT_IMPLEMENTED;
}

mix_retcode_t mix_pop(struct mix_context* ctx, int32_t nr_item) {
    int32_t stk_sz = vector_size(&ctx->runtime_stack);
    int32_t new_sz = (stk_sz < nr_item) ? 0 : (stk_sz - nr_item);
    int ret = vector_resize(&ctx->runtime_stack, new_sz, NULL, __destroy_tov_func);
    return (ret == 0) ? MIX_RC_OK : MIX_RC_INTERNAL_ERROR;
}

mix_retcode_t mix_replace(struct mix_context* ctx, int32_t idx) {
    int32_t stk_sz = mix_get_stack_size(ctx);
    if (idx < 0) {
        idx += stk_sz;
        if (idx < 0) {
            return MIX_RC_INVALID;
        }
    }
    if (idx + 1 < stk_sz) {
        mix_type_or_value_copy(vector_at(&ctx->runtime_stack, ctx->runtime_stack_base_idx + stk_sz - 1),
                               vector_at(&ctx->runtime_stack, ctx->runtime_stack_base_idx + idx));
        mix_pop(ctx, 1);
    }
    return MIX_RC_OK;
}

mix_retcode_t mix_remove(struct mix_context* ctx, int32_t idx) {
    int32_t stk_sz = mix_get_stack_size(ctx);
    if (idx < 0) {
        idx += stk_sz;
        if (idx < 0) {
            return MIX_RC_INVALID;
        }
    }
    vector_remove(&ctx->runtime_stack, idx, NULL, __destroy_tov_func);
    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_new_func_type_begin(struct mix_context* ctx) {
    struct mix_type* t = mix_type_create(MIX_TYPE_FUNC);
    if (!t) {
        logger_error(ctx->logger, "allocate func type failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    struct mix_type_or_value item = {
        .type = MIX_TOV_TYPE,
        .t = t,
    };

    int ret = vector_push_back(&ctx->runtime_stack, &item);
    if (ret != 0) {
        logger_error(ctx->logger, "enlarge runtime stack failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    mix_type_acquire(t);
    return MIX_RC_OK;
}

static mix_retcode_t __set_func_ret_type(struct mix_context* ctx, const struct qbuf_ref* tname) {
    struct mix_type* type = robin_hood_hash_lookup(&ctx->type_hash, tname);
    if (!type) {
        logger_error(ctx->logger, "type [%s] not found.", tname->base);
        return MIX_RC_NOT_FOUND;
    }

    struct mix_type_or_value* tov = __get_top(ctx);
#ifndef NDEBUG
    if (!tov) {
        logger_error(ctx->logger, "runtime stack is empty.");
        return MIX_RC_INVALID;
    }
    if (tov->type != MIX_TOV_TYPE) {
        logger_error(ctx->logger, "top item type [%s] is not an atomic value.",
                     mix_get_type_name(tov->type));
        return MIX_RC_INVALID;
    }
    if (tov->t->value != MIX_TYPE_FUNC) {
        logger_error(ctx->logger, "top item is not a function object.");
        return MIX_RC_INVALID;
    }
#endif
    struct mix_func_type* func_type = container_of(tov->t, struct mix_func_type, t);
    mix_type_acquire(type);
    func_type->ret_type = type;
    return MIX_RC_OK;
}

mix_retcode_t mix_new_func_type_set_ret_i8(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i8", .size = 2};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_i16(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i16", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_i32(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i32", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_i64(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i64", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_f32(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "f32", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_f64(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "f64", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_set_ret_str(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "str", .size = 3};
    return __set_func_ret_type(ctx, &tname);
}

static mix_retcode_t __add_func_arg_type(struct mix_context* ctx,
                                         const struct qbuf_ref* tname) {
    struct mix_type* type = robin_hood_hash_lookup(&ctx->type_hash, tname);
    if (!type) {
        logger_error(ctx->logger, "type [%s] not found.", tname->base);
        return MIX_RC_NOT_FOUND;
    }

    struct mix_type_or_value* tov = __get_top(ctx);
#ifndef NDEBUG
    if (!tov) {
        logger_error(ctx->logger, "runtime stack is empty.");
        return MIX_RC_INVALID;
    }
    if (tov->type != MIX_TOV_TYPE) {
        logger_error(ctx->logger, "top tov is not a type object.");
        return MIX_RC_INVALID;
    }
    if (tov->t->value != MIX_TYPE_FUNC) {
        logger_error(ctx->logger, "top tov is not a function object.");
        return MIX_RC_INVALID;
    }
#endif

    struct mix_func_type* func_type = container_of(tov->t, struct mix_func_type, t);
    int ret = vector_push_back(&func_type->arg_type_list, &type);
    if (ret != 0) {
        logger_error(ctx->logger, "push to runtime stack failed: out of memory.");
        return MIX_RC_NOMEM;
    }
    mix_type_acquire(type);
    return MIX_RC_OK;
}

mix_retcode_t mix_new_func_type_add_arg_i8(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i8", .size = 2};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_i16(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i16", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_i32(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i32", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_i64(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "i64", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_f32(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "f32", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_f64(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "f64", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_str(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "str", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_add_arg_variadic(struct mix_context* ctx) {
    struct qbuf_ref tname = {.base = "...", .size = 3};
    return __add_func_arg_type(ctx, &tname);
}

mix_retcode_t mix_new_func_type_end(struct mix_context* ctx) {
#ifndef NDEBUG
    struct mix_type_or_value* dbg_item = __get_top(ctx);
    if (!dbg_item) {
        logger_error(ctx->logger, "runtime stack is empty.");
        return MIX_RC_INVALID;
    }
    if (dbg_item->type != MIX_TOV_TYPE) {
        logger_error(ctx->logger, "top dbg_item is not a type dbg_item.");
        return MIX_RC_INVALID;
    }
    if (dbg_item->t->value != MIX_TYPE_FUNC) {
        logger_error(ctx->logger, "top dbg_item is not a function type.");
        return MIX_RC_INVALID;
    }
#endif

    return MIX_RC_OK;
}

mix_retcode_t mix_new_func(struct mix_context* ctx, mix_func_t func) {
    struct mix_type_or_value* tov = __get_top(ctx);
#ifndef NDEBUG
    if (!tov) {
        logger_error(ctx->logger, "runtime stack is empty.");
        return MIX_RC_INVALID;
    }
    if (tov->type != MIX_TOV_TYPE) {
        logger_error(ctx->logger, "top item is not a type item.");
        return MIX_RC_INVALID;
    }
    if (tov->t->value != MIX_TYPE_FUNC) {
        logger_error(ctx->logger, "top item is not a function type.");
        return MIX_RC_INVALID;
    }
#endif

    tov->type = MIX_TOV_ATOMIC_VALUE;
    tov->f = func;

    return MIX_RC_OK;
}

mix_retcode_t mix_call(struct mix_context* ctx, int32_t argc) {
    int32_t stk_sz = mix_get_stack_size(ctx);
    int32_t func_idx = stk_sz - argc - 1;
    struct mix_type_or_value* func_tov = __get_item(ctx, func_idx);

#ifndef NDEBUG
    if (!func_tov) {
        logger_error(ctx->logger, "get function failed.");
        return MIX_RC_INVALID;
    }
    if (func_tov->type != MIX_TOV_ATOMIC_VALUE) {
        logger_error(ctx->logger, "object in idx [%u] is not an atomic value.", func_idx);
        return MIX_RC_INVALID;
    }
    if (func_tov->t->value != MIX_TYPE_FUNC) {
        logger_error(ctx->logger, "object is not a function.");
        return MIX_RC_INVALID;
    }
#endif

    struct mix_func_type* func_type = container_of(func_tov->t, struct mix_func_type, t);

    int32_t prev_idx = ctx->runtime_stack_base_idx;
    ctx->runtime_stack_base_idx = func_idx + 1;

    func_tov->f(ctx);

    --ctx->runtime_stack_base_idx; /* including the function itself */
    if (func_type->ret_type) {
        /* the function object is replaced by the returned value */
        mix_replace(ctx, 0 /* based on ctx->runtime_stack_base_idx */);
        ++ctx->runtime_stack_base_idx; /* now the function is replaced by the result and should be reserved */
    }

    mix_pop(ctx, mix_get_stack_size(ctx));
    ctx->runtime_stack_base_idx = prev_idx;

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

#define TO_INTEGER(idx)                                     \
    struct mix_type_or_value* item = __get_item(ctx, idx);  \
    return item->l                                          \

int8_t mix_to_i8(struct mix_context* ctx, int32_t idx) {
    TO_INTEGER(idx);
}

int16_t mix_to_i16(struct mix_context* ctx, int32_t idx) {
    TO_INTEGER(idx);
}

int32_t mix_to_i32(struct mix_context* ctx, int32_t idx) {
    TO_INTEGER(idx);
}

int64_t mix_to_i64(struct mix_context* ctx, int32_t idx) {
    TO_INTEGER(idx);
}

#define TO_FLOAT(idx)                                       \
    struct mix_type_or_value* item = __get_item(ctx, idx);  \
    return item->d                                          \

float mix_to_f32(struct mix_context* ctx, int32_t idx) {
    TO_FLOAT(idx);
}

double mix_to_f64(struct mix_context* ctx, int32_t idx) {
    TO_FLOAT(idx);
}

const char* mix_to_str(struct mix_context* ctx, int32_t idx, int32_t* len) {
    struct mix_type_or_value* item = __get_item(ctx, idx);
    struct qbuf* str = &item->v->s;
    if (len) {
        *len = qbuf_size(str);
    }
    return qbuf_data(str);
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_get(struct mix_context* ctx, const char* name) {
    struct qbuf_ref qname = {.base = name, .size = strlen(name)};
    struct mix_identifier* var = (struct mix_identifier*)robin_hood_hash_lookup(&ctx->var_hash, &qname);
    if (!var) {
        logger_error(ctx->logger, "cannot find variable [%s].", name);
        return MIX_RC_NOT_FOUND;
    }

    struct mix_type_or_value item = {
        .type = var->tov.type,
    };

    if (var->tov.type == MIX_TOV_ATOMIC_VALUE) {
        mix_type_acquire(var->tov.t);
        item.t = var->tov.t;
        item.d = var->tov.d;
    } else {
        mix_shared_value_acquire(var->tov.v);
        item.v = var->tov.v;
    }

    vector_push_back(&ctx->runtime_stack, &item);
    return MIX_RC_OK;
}

mix_retcode_t mix_set(struct mix_context* ctx, const char* name) {
    struct mix_identifier* var = mix_identifier_new();
    if (!var) {
        return MIX_RC_NOMEM;
    }

    struct qbuf_ref qname = {.base = name, .size = strlen(name)};
    struct robin_hood_hash_insertion_res res =
        robin_hood_hash_insert(&ctx->var_hash, &qname, NULL);
    if (!res.inserted) {
        mix_identifier_delete(var);
        return MIX_RC_EXISTS;
    }

    struct mix_type_or_value* tov = __get_top(ctx);
    var->tov.type = tov->type;
    if (tov->type == MIX_TOV_ATOMIC_VALUE) {
        mix_type_acquire(tov->t);
        var->tov.t = tov->t;
        var->tov.l = tov->l;
    } else {
        mix_shared_value_acquire(tov->v);
        var->tov.v = tov->v;
    }
    qbuf_assign(&var->name, qname.base, qname.size);
    *res.pvalue = var;

    mix_pop(ctx, 1);

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_register(struct mix_context* ctx, const char* lib_name,
                           const char* name) {
    struct qbuf_ref qlib_name = {.base = lib_name, .size = strlen(lib_name)};
    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(
        &ctx->libs, &qlib_name, NULL);
    if (!res.pvalue) {
        logger_error(ctx->logger, "create lib info failed: out of memory.");
        return MIX_RC_NOMEM;
    }

    struct mix_lib_info* info;
    if (res.inserted) {
        info = mix_lib_info_new(&qlib_name);
        if (!info) {
            logger_error(ctx->logger, "create lib info failed: out of memory.");
            return MIX_RC_NOMEM;
        }
        *res.pvalue = info;
    } else {
        info = (struct mix_lib_info*)(*res.pvalue);
    }

    struct mix_identifier* var = mix_identifier_new();
    if (!var) {
        logger_error(ctx->logger, "create new variable failed: out of memory.");
        return MIX_RC_NOMEM;
    }
    qbuf_assign(&var->name, name, strlen(name));

    int ret = vector_push_back(&info->identifier_list, &var);
    if (ret != 0) {
        logger_error(ctx->logger, "add new variable failed: out of memory.");
        mix_identifier_delete(var);
        return MIX_RC_NOMEM;
    }

    struct mix_type_or_value* tov = __get_top(ctx);
    mix_type_or_value_move_construct(tov, &var->tov);
    mix_pop(ctx, 1);

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_eval_buffer(struct mix_context* ctx, const char* buf,
                              int32_t sz, const char* lib_name) {
    struct mix_parser parser;
    mix_retcode_t rc = mix_parser_init(&parser, ctx);
    if (rc != MIX_RC_OK) {
        logger_error(ctx->logger, "init parser failed: %s.", mix_get_retcode_str(rc));
        return rc;
    }

    rc = mix_parser_parse(&parser, buf, sz, ctx->logger);
    if (rc != MIX_RC_OK) {
        logger_error(ctx->logger, "parse failed.");
        return rc;
    }

    mix_parser_destroy(&parser);

    return rc;
}

mix_retcode_t mix_eval_file(struct mix_context* ctx, const char* fpath,
                            const char* lib_name) {
    struct qbuf file_content;
    qbuf_init(&file_content);

    mix_retcode_t ret = read_file_content(fpath, &file_content);
    if (ret != MIX_RC_OK) {
        return ret;
    }

    ret = mix_eval_buffer(ctx, qbuf_data(&file_content), qbuf_size(&file_content),
                          lib_name);
    qbuf_destroy(&file_content);

    return ret;
}
