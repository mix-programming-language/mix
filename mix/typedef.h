#ifndef __MIX_TYPEDEF_H__
#define __MIX_TYPEDEF_H__

#include <stdint.h>

enum {
    MIX_VARTYPE_UNKNOWN = 0,
    MIX_VARTYPE_I8,
    MIX_VARTYPE_I16,
    MIX_VARTYPE_I32,
    MIX_VARTYPE_I64,
    MIX_VARTYPE_F32,
    MIX_VARTYPE_F64,
    MIX_VARTYPE_STR,
    MIX_VARTYPE_STRUCT,
    MIX_VARTYPE_TRAIT,
    MIX_VARTYPE_FUNC,
    MIX_VARTYPE_ALIAS,
};

typedef uint32_t mix_var_type_t;

#define MIX_IS_INT(t) ((t) >= MIX_VARTYPE_I8 && (t) <= MIX_VARTYPE_I64)
#define MIX_IS_FLOAT(t) ((t) == MIX_VARTYPE_F32 || (t) == MIX_VARTYPE_F64)
#define MIX_IS_ATOMIC_TYPE(t) ((t) >= MIX_VARTYPE_I8 && (t) <= MIX_VARTYPE_F64)
#define MIX_IS_BUILTIN_TYPE(t) ((t) >= MIX_VARTYPE_I8 && (t) <= MIX_VARTYPE_STR)

static inline const char* mix_get_type_str(mix_var_type_t t) {
    static const char* type_str[] = {
        "unknown", "i8", "i16", "i32", "i64", "f32", "f64", "str", "struct", "trait", "func", "<alias>",
    };
    return type_str[t];
}

#endif
