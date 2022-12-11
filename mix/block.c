#include "block.h"
#include "variable.h"
#include "cutils/qbuf_ref.h"
#include "cutils/hash_func.h"
#include <string.h> /* memcmp() */
#include <stdlib.h> /* free() */

static void destroy_variable(void* data, void* nil) {
    mix_variable_delete((struct mix_variable*)data);
}

static void hash_destroy_type(void* data, void* nil) {
    mix_type_delete((struct mix_type*)data);
}

static void vector_destroy_type(void* item, void* nil) {
    struct mix_type* t = (struct mix_type*)(*(void**)item);
    mix_type_delete(t);
}

void mix_block_delete(struct mix_block* b) {
    if (b) {
        robin_hood_hash_foreach(&b->var_hash, NULL, destroy_variable);
        robin_hood_hash_destroy(&b->var_hash);
        robin_hood_hash_foreach(&b->type_hash, NULL, hash_destroy_type);
        robin_hood_hash_destroy(&b->type_hash);
        vector_destroy(&b->anomymous_type, NULL, hash_destroy_type);
        free(b);
    }
}

static int qbuf_ref_equal_func(const void* a, const void* b) {
    return qbuf_ref_equal((const struct qbuf_ref*)a, (const struct qbuf_ref*)b);
}

static const void* type_hash_getkey_func(const void* data) {
    const struct mix_type* t = (const struct mix_type*)data;
    return qbuf_get_ref(&t->name);
}

static unsigned long qbuf_ref_hash_func(const void* key) {
    const struct qbuf_ref* k = (const struct qbuf_ref*)key;
    return bkd_hash(k->base, k->size);
}

static const struct robin_hood_hash_operations g_type_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = type_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

static const void* var_hash_getkey_func(const void* data) {
    const struct mix_variable* var = (const struct mix_variable*)data;
    return &var->name;
}

static const struct robin_hood_hash_operations g_var_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = var_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

struct mix_block* mix_block_new() {
    struct mix_block* b = (struct mix_block*)malloc(sizeof(struct mix_block));
    if (!b) {
        return NULL;
    }

    int err = robin_hood_hash_init(&b->var_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                                   &g_var_hash_ops);
    if (err) {
        goto err1;
    }

    err = robin_hood_hash_init(&b->type_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                               &g_type_hash_ops);
    if (err) {
        goto err2;
    }

    vector_init(&b->anomymous_type, sizeof(struct mix_type*));
    return b;

err2:
    robin_hood_hash_destroy(&b->var_hash);
err1:
    free(b);
    return NULL;
}
