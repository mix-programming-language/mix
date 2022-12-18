#ifndef __MIX_TYPEDEF_INTERNAL_H__
#define __MIX_TYPEDEF_INTERNAL_H__

#include "mix/typedef.h"

enum {
    MIX_TYPE_TYPEDEF = MIX_TYPE_TRAIT + 1,
    MIX_TYPE_VARIADIC_ARG, /* for function arg */
};

#define MIX_TYPE_IS_INT(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_I64)
#define MIX_TYPE_IS_FLOAT(t) ((t) == MIX_TYPE_F32 || (t) == MIX_TYPE_F64)
#define MIX_TYPE_IS_NUM(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_F64)
#define MIX_TYPE_IS_ATOMIC(t) ((t) >= MIX_TYPE_I8 && (t) <= MIX_TYPE_FUNC)

#endif
