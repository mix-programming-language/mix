#ifndef __MIX_PARSER_MIX_AST_NODE_H__
#define __MIX_PARSER_MIX_AST_NODE_H__

#include "runtime/mix_context.h"
#include "mix_ast_node_type.h"
#include "common/typedef_internal.h"
#include "mix/retcode.h"

struct mix_ast_node {
    mix_ast_node_type_t type;
};

/* -------------------------------------------------------------------------- */

#include "common/mix_type.h"

struct mix_ast_var_decl_node {
    struct mix_ast_node n;
    struct qbuf name;
    struct mix_type* type;
    struct mix_ast_node* rhs; /* decl with initialization */
};

/* -------------------------------------------------------------------------- */

#include "runtime/mix_type_or_value.h"

struct mix_ast_exp_node {
    struct mix_ast_node n;
    struct mix_type_or_value tov;
#ifndef NDEBUG
    struct qbuf name;
#endif
};

/* -------------------------------------------------------------------------- */

#include "cutils/vector.h"

struct mix_ast_cond_stat {
    struct mix_ast_node* cond;
    struct mix_ast_node* stat;
};

struct mix_ast_branch_node {
    struct mix_ast_node n;
    struct vector cond_stat_list;
};

/* -------------------------------------------------------------------------- */

struct mix_ast_binop_node {
#define MIX_BINOP_UNKNOWN 0
    char op;
    struct mix_ast_exp_node* lhs;
    struct mix_ast_exp_node* rhs;
    struct mix_ast_exp_node en;
};

/* -------------------------------------------------------------------------- */

struct mix_ast_node_list_node {
    struct mix_ast_node n;
    struct vector node_list;
};

/* -------------------------------------------------------------------------- */

struct mix_ast_import_node {
    struct mix_ast_node n;
    struct vector name_list; /* struct qbuf* */
};

/* -------------------------------------------------------------------------- */

struct mix_ast_import_alias_node {
    struct mix_ast_node n;
    struct qbuf orig;
    struct qbuf alias;
};

/* -------------------------------------------------------------------------- */

struct mix_ast_funcall_node {
    struct mix_ast_node n;
    struct mix_ast_exp_node* func;
    struct vector argv; /* struct mix_ast_node* */
};

/* -------------------------------------------------------------------------- */

struct mix_ast_node* mix_ast_node_create(mix_ast_node_type_t);
void mix_ast_node_release(struct mix_ast_node*);

void mix_ast_node_do_print(const char* file, int line, struct mix_ast_node* node);
#define mix_ast_node_print(node) mix_ast_node_do_print(__FILE__, __LINE__, node)

#endif
