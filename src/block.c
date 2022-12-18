#include "block.h"
#include "qbuf_ref_hash_utils.h"
#include "identifier.h"
#include <stdlib.h> /* free() */

/* -------------------------------------------------------------------------- */

static void destroy_variable(void* data, void* nil) {
    mix_identifier_delete((struct mix_identifier*)data);
}

static void hash_destroy_type(void* data, void* nil) {
    mix_type_release((struct mix_type*)data);
}

void mix_block_delete(struct mix_block* b) {
    if (b) {
        robin_hood_hash_destroy(&b->var_hash, NULL, destroy_variable);
        robin_hood_hash_destroy(&b->type_hash, NULL, hash_destroy_type);
        free(b);
    }
}

/* -------------------------------------------------------------------------- */

static const void* type_hash_getkey_func(const void* data) {
    const struct mix_type* t = (const struct mix_type*)data;
    return qbuf_get_ref(&t->name);
}

static const struct robin_hood_hash_operations g_type_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = type_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

static const void* var_hash_getkey_func(const void* data) {
    const struct mix_identifier* var = (const struct mix_identifier*)data;
    return qbuf_get_ref(&var->name);
}

static const struct robin_hood_hash_operations g_var_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = var_hash_getkey_func,
    .hash = qbuf_ref_hash_func,
};

/* -------------------------------------------------------------------------- */

struct mix_block* mix_block_new() {
    struct mix_block* b = (struct mix_block*)malloc(sizeof(struct mix_block));
    if (!b) {
        return NULL;
    }

    int err = robin_hood_hash_init(&b->type_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                                   &g_type_hash_ops);
    if (err) {
        goto err1;
    }

    err = robin_hood_hash_init(&b->var_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                               &g_var_hash_ops);
    if (err) {
        goto err2;
    }

    return b;

err2:
    robin_hood_hash_destroy(&b->type_hash, NULL, NULL);
err1:
    free(b);
    return NULL;
}
