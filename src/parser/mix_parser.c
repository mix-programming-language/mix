#include "mix_parser.h"
#include "runtime/mix_identifier.h"
#include "runtime/mix_lib_info.h"
#include "common/mix_type.h"
#include "cutils/robin_hood_hash.h"
#include <stdlib.h> /* malloc() */

#include "lex/mix_lex.h"
#include "mix.tab.h"

#ifndef NDEBUG
#include "misc/debug_utils.h"
#endif

mix_retcode_t mix_parser_init(struct mix_parser* p, struct mix_context* ctx) {
    p->ast_root = NULL;
    p->ctx = ctx;
    list_init(&p->block_stack);

    /* creates the root block */
    struct mix_block* root_block = mix_block_new();
    if (!root_block) {
        logger_error(ctx->logger, "allocate root block for parsing failed.");
        return MIX_RC_INTERNAL_ERROR;
    }
    list_add_next(&root_block->node, &p->block_stack);

    return MIX_RC_OK;
}

void mix_parser_destroy(struct mix_parser* p) {
    struct list_node *curr, *next;
    list_for_each_safe(curr, next, &p->block_stack) {
        struct mix_block* blk = list_entry(curr, struct mix_block, node);
        list_del(&blk->node);
        mix_block_delete(blk);
    }

    mix_ast_node_release(p->ast_root);
}

mix_retcode_t mix_parser_parse(struct mix_parser* p, const char* buf, uint32_t sz,
                               struct logger* logger) {
    struct mix_lex lex;
    mix_lex_init(&lex, buf, sz);
    int ret = yyparse(p, &lex, buf, sz, logger);
    mix_lex_destroy(&lex);

    if (ret == YYerror) {
        logger_error(logger, "parse failed.");
        return MIX_RC_INTERNAL_ERROR;
    }

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

struct mix_type* mix_parser_lookup_type(struct mix_parser* p, const struct qbuf_ref* tname) {
    struct list_node* curr;
    list_for_each(curr, &p->block_stack) {
        struct mix_block* blk = list_entry(curr, struct mix_block, node);
        struct mix_type* type = (struct mix_type*)robin_hood_hash_lookup(&blk->type_hash, tname);
        if (type) {
            return type;
        }
    }

    return (struct mix_type*)robin_hood_hash_lookup(&p->ctx->type_hash, tname);
}

struct mix_identifier* mix_parser_lookup_identifier(struct mix_parser* p, const struct qbuf_ref* name) {
    struct list_node* curr;
    list_for_each(curr, &p->block_stack) {
        struct mix_block* blk = list_entry(curr, struct mix_block, node);
        struct mix_identifier* id = (struct mix_identifier*)robin_hood_hash_lookup(&blk->id_hash, name);
        if (id) {
            return id;
        }
    }
    return NULL;
}

struct mix_identifier* mix_parser_decl_var(struct mix_parser* p, struct mix_type* type,
                                           const struct qbuf_ref* vname) {
    struct mix_block* curr_blk = mix_parser_get_current_block(p);

    struct mix_identifier* var = mix_identifier_new();
    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(&curr_blk->id_hash, vname, var);
    if (!res.inserted) {
        logger_error(p->ctx->logger, "declare new variable [%s] failed: exists.", make_tmp_str_s(vname));
        mix_identifier_delete(var);
        return NULL;
    }

    var->tov.type = MIX_TOV_TYPE;
    mix_type_acquire(type);
    var->tov.t = type;
    qbuf_assign(&var->name, vname->base, vname->size);
    return var;
}

/* -------------------------------------------------------------------------- */

struct import_lib_arg {
    const struct qbuf_ref* lib_name;
    struct mix_block* block;
    struct logger* logger;
};

static int __import_lib_func(void* item, void* arg) {
    struct import_lib_arg* iarg = (struct import_lib_arg*)arg;
    struct mix_identifier* id = (struct mix_identifier*)(*(void**)item);

    struct mix_identifier* new_id = mix_identifier_new();
    if (!new_id) {
        logger_error(iarg->logger, "allocate new identifier failed: out of memory.");
        return -1;
    }

    qbuf_assign(&new_id->name, iarg->lib_name->base, iarg->lib_name->size);
    qbuf_append(&new_id->name, qbuf_data(&id->name), qbuf_size(&id->name));

    struct robin_hood_hash_insertion_res res = robin_hood_hash_insert(
        &iarg->block->id_hash, qbuf_get_ref(&new_id->name), NULL);
    if (!res.pvalue) {
        logger_error(iarg->logger, "insert new variable failed: out of memory.");
        return -1;
    }
    if (!res.inserted) {
        logger_error(iarg->logger, "variable [%s] exists.", make_tmp_str_s(qbuf_get_ref(&new_id->name)));
        mix_identifier_delete(new_id);
        return -1;
    }

    mix_type_or_value_copy_construct(&id->tov, &new_id->tov);
    *res.pvalue = new_id;
    return 0;
}

mix_retcode_t mix_parser_import_lib(struct mix_parser* p, const struct qbuf_ref* lib_name,
                                    const struct qbuf_ref* alias) {
    struct mix_lib_info* info = robin_hood_hash_lookup(&p->ctx->libs, lib_name);
    if (!info) {
        logger_error(p->ctx->logger, "lib [%s] is not registered.", lib_name);
        return MIX_RC_NOT_FOUND;
    }

    if (alias) {
        lib_name = alias;
    }

    struct import_lib_arg iarg = {
        .lib_name = lib_name,
        .block = mix_parser_get_current_block(p),
        .logger = p->ctx->logger,
    };

    vector_foreach(&info->identifier_list, &iarg, __import_lib_func);

    return MIX_RC_OK;
}
