#ifndef __MIX_VALUE_H__
#define __MIX_VALUE_H__

#include "type.h"
#include "context.h"
#include "cutils/qbuf.h"

struct mix_value {
    uint32_t refcount;
    struct mix_type* type;
    union {
        int64_t l;
        double d;
        mix_func_t func;
        struct qbuf s;
    };
};

struct mix_value* mix_value_create();

void mix_value_release(struct mix_value*);

static inline void mix_value_acquire(struct mix_value* v) {
    ++v->refcount;
}

#endif
