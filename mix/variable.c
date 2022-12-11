#include "variable.h"
#include <stdlib.h>

struct mix_variable* mix_variable_new() {
    struct mix_variable* var = (struct mix_variable*)malloc(sizeof(struct mix_variable));
    if (var) {
        qbuf_init(&var->name);
        var->type = NULL;
        var->value = NULL;
    }
    return var;
}

void mix_variable_delete(struct mix_variable* var) {
    if (var) {
        qbuf_destroy(&var->name);
        if (var->type && !MIX_IS_ATOMIC_TYPE(var->type->value)) {
            /* constant values are stored in union */
            if (var->value) {
                mix_value_release(var->value);
            }
        }
        free(var);
    }
}
