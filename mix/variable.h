#ifndef __MIX_VARIABLE_H__
#define __MIX_VARIABLE_H__

#include "value.h"
#include "cutils/qbuf.h"

struct mix_variable {
    struct qbuf name;
    const struct mix_type* type;
    union {
        long l;
        double d;
        struct mix_value* value;
    };
};

struct mix_variable* mix_variable_new();
void mix_variable_delete(struct mix_variable*);

#endif
