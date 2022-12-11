#ifndef __MIX_RUNTIME_STACK_ITEM_H__
#define __MIX_RUNTIME_STACK_ITEM_H__

#include "value.h"

struct runtime_stack_item {
#define STACK_ITEM_TYPE_UNKNOWN 0
#define STACK_ITEM_TYPE_TYPE 1
#define STACK_ITEM_TYPE_VALUE 2
    int type;

    union {
        struct mix_type* t;
        struct mix_value* v;
    };
};

#endif
