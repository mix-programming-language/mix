#include "value.h"
#include <stdlib.h> /* malloc() */

struct mix_value* mix_value_create() {
    struct mix_value* v = (struct mix_value*)malloc(sizeof(struct mix_value));
    if (v) {
        v->refcount = 0;
        v->type = NULL;
        qbuf_init(&v->s);
    }
    return v;
}

void mix_value_release(struct mix_value* v) {
    if (v) {
        if (v->refcount > 0) {
            --v->refcount;
            if (v->refcount > 0) {
                return;
            }
        }

        if (v->type && v->type->value == MIX_VARTYPE_STR) {
            qbuf_destroy(&v->s);
        }
        free(v);
    }
}
