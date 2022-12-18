#ifndef __MIX_SHARED_VALUE_H__
#define __MIX_SHARED_VALUE_H__

#include "type.h"
#include "cutils/qbuf.h"

struct mix_shared_value {
    uint32_t refcount;
    struct mix_type* type;
    union {
        struct qbuf s;
    };
};

struct mix_shared_value* mix_shared_value_create();

void mix_shared_value_release(struct mix_shared_value*);

static inline void mix_shared_value_acquire(struct mix_shared_value* v) {
    ++v->refcount;
}

#endif
