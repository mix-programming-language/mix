#include "mix_lib_info.h"
#include "mix_identifier.h"
#include <stdlib.h>

struct mix_lib_info* mix_lib_info_new(const struct qbuf_ref* qname) {
    struct mix_lib_info* info = (struct mix_lib_info*)malloc(sizeof(struct mix_lib_info));
    if (info) {
        qbuf_init(&info->name);
        qbuf_assign(&info->name, qname->base, qname->size);
        vector_init(&info->identifier_list, sizeof(struct mix_identifier*));
    }
    return info;
}

static void destroy_identifier_func(void* item, void* arg) {
    struct mix_identifier* var = (struct mix_identifier*)(*(void**)item);
    mix_identifier_delete(var);
}

void mix_lib_info_delete(struct mix_lib_info* info) {
    if (info) {
        qbuf_destroy(&info->name);
        vector_destroy(&info->identifier_list, NULL, destroy_identifier_func);
        free(info);
    }
}
