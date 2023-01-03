#ifndef __MIX_RUNTIME_MIX_CONTEXT_H__
#define __MIX_RUNTIME_MIX_CONTEXT_H__

#include "mix/typedef.h"
#include "mix/retcode.h"
#include "cutils/vector.h"
#include "cutils/robin_hood_hash.h"
#include "logger/logger.h"

struct mix_context {
    int32_t runtime_stack_base_idx;

    /* struct mix_type_or_value */
    struct vector runtime_stack;

    /* struct qbuf_ref* => struct mix_type* */
    struct robin_hood_hash type_hash;

    /* struct qbuf_ref* => struct mix_identifier* */
    struct robin_hood_hash var_hash;

    /* available libs. name => vector of identifiers */
    struct robin_hood_hash libs;

    /* logging */
    struct logger* logger;
    struct logger dummy_logger;
};

static inline struct mix_type_or_value* __get_top(struct mix_context* ctx) {
    return vector_back(&ctx->runtime_stack);
}

static inline void __push_tov(struct mix_context* ctx, struct mix_type_or_value* tov) {
    vector_push_back(&ctx->runtime_stack, tov);
}

struct mix_type_or_value* __get_item(struct mix_context* ctx, int32_t idx);

mix_retcode_t __push_integer(struct mix_context*, int64_t, mix_type_t);
mix_retcode_t __push_float(struct mix_context*, double, mix_type_t);

#endif
