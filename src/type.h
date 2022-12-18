#ifndef __MIX_TYPE_H__
#define __MIX_TYPE_H__

#include "typedef_internal.h"
#include "cutils/qbuf.h"
#include "cutils/vector.h"

/* num, str and ... */
struct mix_type {
    uint32_t refcount;
    mix_type_t value;
    struct qbuf name;
};

struct mix_struct_type {
    struct mix_type t;
    uint32_t size;
    struct vector field_list;
};

struct mix_trait_type {
    struct mix_type t;
    struct vector func_list;
};

struct mix_func_type {
    struct mix_type t;
    struct mix_type* ret_type;
    struct vector arg_type_list;
};

struct mix_typedef_type {
    struct mix_type t;
    struct mix_type* orig;
};

struct mix_type* mix_type_create(mix_type_t);

static inline void mix_type_acquire(struct mix_type* type) {
    ++type->refcount;
}

void mix_type_release(struct mix_type* type);

#endif
