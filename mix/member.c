#include "member.h"
#include <stdlib.h>

struct mix_member* mix_member_new() {
    struct mix_member* m = (struct mix_member*)malloc(sizeof(struct mix_member));
    if (m) {
        qbuf_init(&m->name);
        m->type = NULL;
    }
    return m;
}

void mix_member_delete(struct mix_member* m) {
    if (m) {
        qbuf_destroy(&m->name);
        free(m);
    }
}
