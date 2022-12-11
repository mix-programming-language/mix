#ifndef __MIX_BLOCK_H__
#define __MIX_BLOCK_H__

#include "cutils/vector.h"
#include "cutils/robin_hood_hash.h"

struct mix_block {
    struct robin_hood_hash var_hash;
    struct robin_hood_hash type_hash;
    struct vector anomymous_type; /* for example, type of a function or lambda */
};

struct mix_block* mix_block_new();
void mix_block_delete(struct mix_block*);

#endif
