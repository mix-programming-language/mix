#include "type.h"
#include "typedef_internal.h"
#include "member.h"
#include "cutils/utils.h" /* container_of() */
#include <stdlib.h>

#ifndef NDEBUG
#include "debug_utils.h"
#endif

static inline void init_type(struct mix_type* t, mix_type_t type) {
    t->refcount = 0;
    t->value = type;
    qbuf_init(&t->name);
}

struct mix_type* mix_type_create(mix_type_t type) {
    if (MIX_TYPE_IS_NUM(type) || type == MIX_TYPE_STR || type == MIX_TYPE_VARIADIC_ARG) {
        struct mix_type* t = (struct mix_type*)malloc(sizeof(struct mix_type));
        if (t) {
            init_type(t, type);
            return t;
        }
    } else if (type == MIX_TYPE_STRUCT) {
        struct mix_struct_type* t = (struct mix_struct_type*)malloc(sizeof(struct mix_struct_type));
        if (t) {
            t->size = 0;
            init_type(&t->t, type);
            vector_init(&t->field_list, sizeof(struct mix_member*));
            return &t->t;
        }
    } else if (type == MIX_TYPE_TRAIT) {
        struct mix_trait_type* t = (struct mix_trait_type*)malloc(sizeof(struct mix_trait_type));
        if (t) {
            init_type(&t->t, type);
            vector_init(&t->func_list, sizeof(struct mix_member*));
            return &t->t;
        }
    } else if (type == MIX_TYPE_FUNC) {
        struct mix_func_type* t = (struct mix_func_type*)malloc(sizeof(struct mix_func_type));
        if (t) {
            init_type(&t->t, type);
            t->ret_type = NULL;
            vector_init(&t->arg_type_list, sizeof(struct mix_type*));
            return &t->t;
        }
    } else if (type == MIX_TYPE_TYPEDEF) {
        struct mix_typedef_type* t = (struct mix_typedef_type*)malloc(sizeof(struct mix_typedef_type));
        if (t) {
            init_type(&t->t, type);
            t->orig = NULL;
            return &t->t;
        }
    }

    return NULL;
}

static void destroy_struct_type(struct mix_type* t) {
    struct mix_struct_type* tt = container_of(t, struct mix_struct_type, t);
    /* TODO */
    vector_destroy(&tt->field_list, NULL, NULL);
    free(tt);
}

static void destroy_trait_type(struct mix_type* t) {
    struct mix_trait_type* tt = container_of(t, struct mix_trait_type, t);
    /* TODO */
    vector_destroy(&tt->func_list, NULL, NULL);
    free(tt);
}

static void destroy_func_arg(void* item, void* arg) {
    struct mix_type* t = (struct mix_type*)(*(void**)item);
    mix_type_release(t);
}

static void destroy_func_type(struct mix_type* t) {
    struct mix_func_type* tt = container_of(t, struct mix_func_type, t);
    mix_type_release(tt->ret_type);
    vector_destroy(&tt->arg_type_list, NULL, destroy_func_arg);
    free(tt);
}

static void destroy_typedef_type(struct mix_type* t) {
    struct mix_typedef_type* tt = container_of(t, struct mix_typedef_type, t);
    mix_type_release(tt->orig);
    free(tt);
}

static void mix_type_destroy(struct mix_type* t) {
    if (t) {
        qbuf_destroy(&t->name);
        if (t->value == MIX_TYPE_STRUCT) {
            destroy_struct_type(t);
        } else if (t->value == MIX_TYPE_TRAIT) {
            destroy_trait_type(t);
        } else if (t->value == MIX_TYPE_FUNC) {
            destroy_func_type(t);
        } else if (t->value == MIX_TYPE_TYPEDEF) {
            destroy_typedef_type(t);
        } else {
            free(t);
        }
    }
}

void mix_type_release(struct mix_type* t) {
    if (t) {
        if (t->refcount > 1) {
            --t->refcount;
            return;
        }
        mix_type_destroy(t);
    }
}
