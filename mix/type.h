#ifndef __MIX_TYPE_H__
#define __MIX_TYPE_H__

#include "typedef.h"
#include "cutils/qbuf.h"
#include "cutils/vector.h"

/* builtin types */
struct mix_type {
    struct qbuf name;
    mix_var_type_t value;
};

struct mix_type_struct {
    struct mix_type t;
    uint32_t size;
    struct vector field_list;
};

struct mix_type_trait {
    struct mix_type t;
    struct vector func_list;
};

struct mix_type_func {
    struct mix_type t;
    struct mix_type* ret_type;
    struct vector arg_type_list;
};

struct mix_type_alias {
    struct mix_type t;
    struct mix_type* orig;
};

struct mix_type* mix_type_new(mix_var_type_t);
void mix_type_delete(struct mix_type* type);

#endif
