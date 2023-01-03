#ifndef __MIX_PARSER_MIX_BLOCK_H__
#define __MIX_PARSER_MIX_BLOCK_H__

#include "cutils/list.h"
#include "cutils/robin_hood_hash.h"

struct mix_block {
    struct list_node node;
    struct robin_hood_hash id_hash; /* struct qbuf_ref* => struct mix_identifier* */
    struct robin_hood_hash type_hash; /* struct qbuf_ref* => struct mix_type* */
};

struct mix_block* mix_block_new();
void mix_block_delete(struct mix_block*);

#endif
