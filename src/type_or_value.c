#include "type_or_value.h"

void mix_type_or_value_destroy(struct mix_type_or_value* tov) {
    if (tov->type == MIX_TOV_TYPE || tov->type == MIX_TOV_ATOMIC_VALUE) {
        mix_type_release(tov->t);
    } else if (tov->type == MIX_TOV_SHARED_VALUE) {
        mix_shared_value_release(tov->v);
    }
}

struct mix_type* mix_type_or_value_get_type(struct mix_type_or_value* tov) {
    if (tov->type == MIX_TOV_UNKNOWN) {
        return NULL;
    }
    if (tov->type == MIX_TOV_TYPE || tov->type == MIX_TOV_ATOMIC_VALUE) {
        return tov->t;
    }
    return tov->v->type;
}

void mix_type_or_value_copy_construct(struct mix_type_or_value* src_item,
                                      struct mix_type_or_value* new_item) {
    new_item->type = src_item->type;

    if (src_item->type == MIX_TOV_UNKNOWN) {
        return;
    }

    if (src_item->type == MIX_TOV_SHARED_VALUE) {
        mix_shared_value_acquire(src_item->v);
        new_item->v = src_item->v;
        return;
    }

    /* MIX_TOV_TYPE or MIX_TOV_ATOMIC_VALUE */
    mix_type_acquire(src_item->t);
    new_item->t = src_item->t;
    new_item->l = src_item->l;
}

void mix_type_or_value_move_construct(struct mix_type_or_value* src_item,
                                      struct mix_type_or_value* new_item) {
    *new_item = *src_item;
    src_item->type = MIX_TOV_UNKNOWN;
    src_item->v = NULL;
}

void mix_type_or_value_move(struct mix_type_or_value* src_item,
                            struct mix_type_or_value* dst_item) {
    mix_type_or_value_destroy(dst_item);
    mix_type_or_value_move_construct(src_item, dst_item);
}

void mix_type_or_value_copy(struct mix_type_or_value* src_item,
                            struct mix_type_or_value* dst_item) {
    mix_type_or_value_destroy(dst_item);
    mix_type_or_value_copy_construct(src_item, dst_item);
}
