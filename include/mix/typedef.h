#ifndef __MIX_TYPEDEF_H__
#define __MIX_TYPEDEF_H__

#include <stdint.h>

enum {
    MIX_TYPE_NIL = 0,
    MIX_TYPE_I8,
    MIX_TYPE_I16,
    MIX_TYPE_I32,
    MIX_TYPE_I64,
    MIX_TYPE_F32,
    MIX_TYPE_F64,
    MIX_TYPE_FUNC,
    MIX_TYPE_STR,
    MIX_TYPE_STRUCT,
    MIX_TYPE_TRAIT,
};

typedef uint32_t mix_type_t;

#define MIX_TYPE_IS_INT(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_I64)
#define MIX_TYPE_IS_FLOAT(t) ((t) == MIX_TYPE_F32 || (t) == MIX_TYPE_F64)
#define MIX_TYPE_IS_NUM(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_F64)
#define MIX_TYPE_IS_ATOMIC(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_FUNC)

const char* mix_get_type_name(mix_type_t t);

#endif
