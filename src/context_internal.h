#ifndef __MIX_CONTEXT_INTERNAL_H__
#define __MIX_CONTEXT_INTERNAL_H__

#include "cutils/vector.h"
#include "cutils/robin_hood_hash.h"
#include "logger/logger.h"

struct mix_context {
    int32_t runtime_stack_base_idx;

    /* struct mix_type_or_value */
    struct vector runtime_stack;

    /* struct mix_block* */
    struct vector block_stack;

    /* available libs. prefix => vector of identifiers */
    struct robin_hood_hash libs;

    /* logging */
    struct logger* logger;
    struct logger dummy_logger;
};

static inline struct mix_block* __get_root_block(struct mix_context* ctx) {
    return (struct mix_block*)(*((void**)vector_front(&ctx->block_stack)));
}

static inline struct mix_block* __get_current_block(struct mix_context* ctx) {
    return (struct mix_block*)(*((void**)vector_back(&ctx->block_stack)));
}

static inline struct mix_type_or_value* __get_top(struct mix_context* ctx) {
    return vector_back(&ctx->runtime_stack);
}

#endif
