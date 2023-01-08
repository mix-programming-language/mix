#ifndef __MIX_RUNTIME_MIX_TYPE_OR_VALUE_H__
#define __MIX_RUNTIME_MIX_TYPE_OR_VALUE_H__

#include "runtime/mix_shared_value.h"
#include "mix/mix_func_t.h"
#include <stdint.h>

struct mix_type_or_value {
#define MIX_TOV_UNKNOWN 0
#define MIX_TOV_TYPE 1
#define MIX_TOV_ATOMIC_VALUE 2
#define MIX_TOV_SHARED_VALUE 3
    int type;

    union {
        struct {
            struct mix_type* t; /* used in TYPE and ATOMIC_VALUE */
            union {
                int64_t l;
                double d;
                mix_func_t f;
            };
        };
        struct mix_shared_value* v; /* used in SHARED_VALUE */
    };
};

void mix_type_or_value_destroy(struct mix_type_or_value*);

struct mix_type* mix_type_or_value_get_type(struct mix_type_or_value*);

/* copies `src_item` to `new_item`. */
void mix_type_or_value_copy_construct(struct mix_type_or_value* src_item,
                                      struct mix_type_or_value* new_item);

/*
  moves `src_item` to `new_item`. `src_item` will be destroyed.
  `new_item` can be uninitialized.
*/
void mix_type_or_value_move_construct(struct mix_type_or_value* src_item,
                                      struct mix_type_or_value* new_item);

/*
  moves `src_item` to `dst_item`. `src_item` will be destroyed.
  `dst_item` MUST be initialized.
*/
void mix_type_or_value_move(struct mix_type_or_value* src_item,
                            struct mix_type_or_value* dst_item);

/* copies `src_item` to `dst_item`. `dst_item` MUST be initialized. */
void mix_type_or_value_copy(struct mix_type_or_value* src_item,
                            struct mix_type_or_value* dst_item);

#endif
