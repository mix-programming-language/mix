#include "typedef_internal.h"
#include "mix_type.h"
#include "mix_member.h"
#include "cutils/utils.h" /* container_of() */
#include <stdlib.h>

static void __init_basic_type(struct mix_type* t, mix_type_t type) {
    t->refcount = 0;
    t->value = type;
    qbuf_init(&t->name);
}

static inline void __destroy_basic_type(struct mix_type* t) {
    qbuf_destroy(&t->name);
}

static struct mix_type* __create_basic_type(mix_type_t type) {
    struct mix_type* t = (struct mix_type*)malloc(sizeof(struct mix_type));
    if (t) {
        __init_basic_type(t, type);
    }
    return t;
}

static void __release_basic_type(struct mix_type* t) {
    __destroy_basic_type(t);
    free(t);
}

static struct mix_type* __create_func_type(mix_type_t type) {
    struct mix_func_type* t = (struct mix_func_type*)malloc(sizeof(struct mix_func_type));
    if (!t) {
        return NULL;
    }

    __init_basic_type(&t->t, type);
    t->ret_type = NULL;
    vector_init(&t->arg_type_list, sizeof(struct mix_type*));
    return &t->t;
}

static void destroy_func_arg(void* item, void* arg) {
    struct mix_type* t = (struct mix_type*)(*(void**)item);
    mix_type_release(t);
}

static void __release_func_type(struct mix_type* t) {
    struct mix_func_type* tt = container_of(t, struct mix_func_type, t);
    mix_type_release(tt->ret_type);
    vector_destroy(&tt->arg_type_list, NULL, destroy_func_arg);
    __destroy_basic_type(t);
    free(tt);
}

static struct mix_type* __create_struct_type(mix_type_t type) {
    struct mix_struct_type* t = (struct mix_struct_type*)malloc(sizeof(struct mix_struct_type));
    if (!t) {
        return NULL;
    }
    t->size = 0;
    __init_basic_type(&t->t, type);
    vector_init(&t->field_list, sizeof(struct mix_member*));
    return &t->t;
}

static void __release_struct_type(struct mix_type* t) {
    struct mix_struct_type* tt = container_of(t, struct mix_struct_type, t);
    /* TODO */
    vector_destroy(&tt->field_list, NULL, NULL);
    __destroy_basic_type(t);
    free(tt);
}

static struct mix_type* __create_trait_type(mix_type_t type) {
    struct mix_trait_type* t = (struct mix_trait_type*)malloc(sizeof(struct mix_trait_type));
    if (!t) {
        return NULL;
    }
    __init_basic_type(&t->t, type);
    vector_init(&t->func_list, sizeof(struct mix_member*));
    return &t->t;
}

static void __release_trait_type(struct mix_type* t) {
    struct mix_trait_type* tt = container_of(t, struct mix_trait_type, t);
    /* TODO */
    vector_destroy(&tt->func_list, NULL, NULL);
    __destroy_basic_type(t);
    free(tt);
}

static struct mix_type* __create_typedef_type(mix_type_t type) {
    struct mix_typedef_type* t = (struct mix_typedef_type*)malloc(sizeof(struct mix_typedef_type));
    if (!t) {
        return NULL;
    }
    __init_basic_type(&t->t, type);
    t->orig = NULL;
    return &t->t;
}

static void __release_typedef_type(struct mix_type* t) {
    struct mix_typedef_type* tt = container_of(t, struct mix_typedef_type, t);
    mix_type_release(tt->orig);
    __destroy_basic_type(t);
    free(tt);
}

static const struct {
    struct mix_type* (*create_type)(mix_type_t);
    void (*release_type)(struct mix_type*);
} g_type_handler[] = {
    {NULL, NULL},
    {__create_basic_type, __release_basic_type}, /* i8 */
    {__create_basic_type, __release_basic_type}, /* i16 */
    {__create_basic_type, __release_basic_type}, /* i32 */
    {__create_basic_type, __release_basic_type}, /* i64 */
    {__create_basic_type, __release_basic_type}, /* f32 */
    {__create_basic_type, __release_basic_type}, /* f64 */
    {__create_func_type, __release_func_type}, /* func */
    {__create_basic_type, __release_basic_type}, /* str */
    {__create_struct_type, __release_struct_type}, /* struct */
    {__create_trait_type, __release_trait_type}, /* trait */
    {__create_typedef_type, __release_typedef_type}, /* typedef */
    {__create_basic_type, __release_basic_type}, /* variadic arg */
};

struct mix_type* mix_type_create(mix_type_t type) {
    return g_type_handler[type].create_type(type);
}

void mix_type_release(struct mix_type* t) {
    if (t) {
        if (t->refcount > 1) {
            --t->refcount;
            return;
        }

        g_type_handler[t->value].release_type(t);
    }
}
