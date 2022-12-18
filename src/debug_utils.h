#ifndef __MIX_DEBUG_UTILS_H__
#define __MIX_DEBUG_UTILS_H__

#include "type_or_value.h"
#include "cutils/qbuf_ref.h"
#include <stdio.h>
#include <string.h>

static inline const char* make_tmp_str_s(const struct qbuf_ref* s) {
    static char buf[1024];
    memcpy(buf, s->base, s->size);
    buf[s->size] = '\0';
    return buf;
}

static inline const char* make_tmp_str_sl(const char* s, int l) {
    static char buf[1024];
    memcpy(buf, s, l);
    buf[l] = '\0';
    return buf;
}

static inline const char* make_tmp_str(const struct qbuf_ref* s, char buf[]) {
    memcpy(buf, s->base, s->size);
    buf[s->size] = '\0';
    return buf;
}

static int print_stack_func(void* item, void* arg) {
    uint32_t* counter = (uint32_t*)arg;
    struct mix_type_or_value* tov = (struct mix_type_or_value*)item;
    struct mix_type* type = mix_type_or_value_get_type(tov);
    printf("    * [%u] name [%s] type [%s] refcount [%u]\n", *counter,
           make_tmp_str_s(qbuf_get_ref(&type->name)),
           mix_get_type_name(type->value), type->refcount);
    ++(*counter);
    return 0;
}

static inline void __print_tov_stack(const char* file, int line, struct vector* vec) {
    uint32_t counter = 0;
    printf("----- %s:%u -----\n", file, line);
    vector_foreach(vec, &counter, print_stack_func);
    printf("----- end -----\n");
}

#define print_tov_stack(vec) __print_tov_stack(__FILE__, __LINE__, (vec))

#endif
