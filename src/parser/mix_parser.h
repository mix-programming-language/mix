#ifndef __MIX_PARSER_MIX_PARSER_H__
#define __MIX_PARSER_MIX_PARSER_H__

#include "mix_ast_node.h"
#include "mix_block.h"
#include "mix/retcode.h"
#include "runtime/mix_context.h"
#include "cutils/list.h"

struct mix_parser {
    struct mix_context* ctx;
    struct mix_ast_node* ast_root;
    struct list_node block_stack;
};

mix_retcode_t mix_parser_init(struct mix_parser*, struct mix_context*);
void mix_parser_destroy(struct mix_parser*);
mix_retcode_t mix_parser_parse(struct mix_parser*, const char* buf, uint32_t sz, struct logger*);

/* -------------------------------------------------------------------------- */

/* internal functions for yyparse() */

struct mix_type* mix_parser_lookup_type(struct mix_parser*, const struct qbuf_ref* tname);
struct mix_identifier* mix_parser_lookup_identifier(struct mix_parser*, const struct qbuf_ref* name);

struct mix_identifier* mix_parser_decl_var(struct mix_parser*, struct mix_type* type,
                                           const struct qbuf_ref* vname);

mix_retcode_t mix_parser_import_lib(struct mix_parser*, const struct qbuf_ref* lib_name,
                                    const struct qbuf_ref* alias);

static inline struct mix_block* mix_parser_get_root_block(struct mix_parser* p) {
    return list_entry(list_prev(&p->block_stack), struct mix_block, node);
}

static inline struct mix_block* mix_parser_get_current_block(struct mix_parser* p) {
    return list_entry(list_next(&p->block_stack), struct mix_block, node);
}

#endif
