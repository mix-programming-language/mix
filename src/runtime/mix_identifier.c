#include "mix_identifier.h"
#include <stdlib.h>

struct mix_identifier* mix_identifier_new() {
    struct mix_identifier* var = (struct mix_identifier*)malloc(sizeof(struct mix_identifier));
    if (var) {
        qbuf_init(&var->name);
        var->tov.type = MIX_TOV_UNKNOWN;
    }
    return var;
}

void mix_identifier_delete(struct mix_identifier* var) {
    if (var) {
        qbuf_destroy(&var->name);
        mix_type_or_value_destroy(&var->tov);
        free(var);
    }
}
