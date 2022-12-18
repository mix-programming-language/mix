#ifndef __MIX_QBUF_REF_HASH_UTILS_H__
#define __MIX_QBUF_REF_HASH_UTILS_H__

#include "cutils/qbuf_ref.h"
#include "cutils/hash_func.h"

static inline int qbuf_ref_equal_func(const void* a, const void* b) {
    return qbuf_ref_equal((const struct qbuf_ref*)a, (const struct qbuf_ref*)b);
}

static inline unsigned long qbuf_ref_hash_func(const void* key) {
    const struct qbuf_ref* k = (const struct qbuf_ref*)key;
    return bkd_hash(k->base, k->size);
}

#endif
