#include "mix_shared_value.h"
#include <stdlib.h> /* malloc() */

struct mix_shared_value* mix_shared_value_create() {
    struct mix_shared_value* v = (struct mix_shared_value*)malloc(sizeof(struct mix_shared_value));
    if (v) {
        v->refcount = 0;
        v->type = NULL;
        qbuf_init(&v->s);
    }
    return v;
}

void mix_shared_value_release(struct mix_shared_value* v) {
    if (v) {
        if (v->refcount > 1) {
            --v->refcount;
            return;
        }

        if (v->type) {
            mix_type_release(v->type);
            if (v->type->value == MIX_TYPE_STR) {
                qbuf_destroy(&v->s);
            }
        }

        free(v);
    }
}
