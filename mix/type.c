#include "type.h"
#include "member.h"
#include "cutils/utils.h" /* container_of() */
#include <stdlib.h>

static inline void init_type(struct mix_type* t, mix_var_type_t type) {
    qbuf_init(&t->name);
    t->value = type;
}

struct mix_type* mix_type_new(mix_var_type_t type) {
    if (MIX_IS_BUILTIN_TYPE(type)) {
        struct mix_type* t = (struct mix_type*)malloc(sizeof(struct mix_type));
        if (t) {
            init_type(t, type);
            return t;
        }
    } else if (type == MIX_VARTYPE_STRUCT) {
        struct mix_type_struct* t = (struct mix_type_struct*)malloc(sizeof(struct mix_type_struct));
        if (t) {
            t->size = 0;
            init_type(&t->t, type);
            vector_init(&t->field_list, sizeof(struct mix_member*));
            return &t->t;
        }
    } else if (type == MIX_VARTYPE_TRAIT) {
        struct mix_type_trait* t = (struct mix_type_trait*)malloc(sizeof(struct mix_type_trait));
        if (t) {
            init_type(&t->t, type);
            vector_init(&t->func_list, sizeof(struct mix_member*));
            /* TODO vector init */
            return &t->t;
        }
    } else if (type == MIX_VARTYPE_FUNC) {
        struct mix_type_func* t = (struct mix_type_func*)malloc(sizeof(struct mix_type_func));
        if (t) {
            init_type(&t->t, type);
            vector_init(&t->arg_type_list, sizeof(struct mix_type*));
            return &t->t;
        }
    } else if (type == MIX_VARTYPE_ALIAS) {
        struct mix_type_alias* t = (struct mix_type_alias*)malloc(sizeof(struct mix_type_alias));
        if (t) {
            init_type(&t->t, type);
            t->orig = NULL;
            return &t->t;
        }
    }

    return NULL;
}

static void delete_type_struct(struct mix_type* t) {
    struct mix_type_struct* tt = container_of(t, struct mix_type_struct, t);
    /* TODO */
    vector_destroy(&tt->field_list, NULL, NULL);
    free(tt);
}

static void delete_type_trait(struct mix_type* t) {
    struct mix_type_trait* tt = container_of(t, struct mix_type_trait, t);
    /* TODO */
    vector_destroy(&tt->func_list, NULL, NULL);
    free(tt);
}

static void delete_type_func(struct mix_type* t) {
    struct mix_type_func* tt = container_of(t, struct mix_type_func, t);
    /* TODO */
    vector_destroy(&tt->arg_type_list, NULL, NULL);
    free(tt);
}

static void delete_type_alias(struct mix_type* t) {
    struct mix_type_alias* tt = container_of(t, struct mix_type_alias, t);
    free(tt);
}

void mix_type_delete(struct mix_type* t) {
    if (t) {
        qbuf_destroy(&t->name);
        if (t->value == MIX_VARTYPE_STRUCT) {
            delete_type_struct(t);
        } else if (t->value == MIX_VARTYPE_TRAIT) {
            delete_type_trait(t);
        } else if (t->value == MIX_VARTYPE_FUNC) {
            delete_type_func(t);
        } else if (t->value == MIX_VARTYPE_ALIAS) {
            delete_type_alias(t);
        } else {
            free(t);
        }
    }
}
