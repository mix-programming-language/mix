#include "mix_block.h"
#include "runtime/mix_identifier.h"
#include "misc/qbuf_ref_hash_utils.h"
#include <stdlib.h> /* free() */

/* -------------------------------------------------------------------------- */

static void __destroy_identifier(void* data, void* nil) {
    mix_identifier_delete((struct mix_identifier*)data);
}

static void __destroy_type(void* data, void* nil) {
    mix_type_release((struct mix_type*)data);
}

void mix_block_delete(struct mix_block* b) {
    if (b) {
        robin_hood_hash_destroy(&b->id_hash, NULL, __destroy_identifier);
        robin_hood_hash_destroy(&b->type_hash, NULL, __destroy_type);
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

static const void* id_hash_getkey_func(const void* data) {
    const struct mix_identifier* var = (const struct mix_identifier*)data;
    return qbuf_get_ref(&var->name);
}

static const struct robin_hood_hash_operations g_id_hash_ops = {
    .equal = qbuf_ref_equal_func,
    .getkey = id_hash_getkey_func,
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

    err = robin_hood_hash_init(&b->id_hash, 20, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR,
                               &g_id_hash_ops);
    if (err) {
        goto err2;
    }

    list_init(&b->node);
    return b;

err2:
    robin_hood_hash_destroy(&b->type_hash, NULL, NULL);
err1:
    free(b);
    return NULL;
}
