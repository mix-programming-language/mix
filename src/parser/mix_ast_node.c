#include "mix_ast_node.h"
#include "mix/retcode.h"
#include "mix/mix.h"
#include "runtime/mix_type_or_value.h"
#include "runtime/mix_identifier.h"
#include "misc/utils.h"
#include "cutils/utils.h"
#include "logger/logger.h"
#include <stdlib.h>

static void __init_base_node(struct mix_ast_node* node, mix_ast_node_type_t type) {
    node->type = type;
}

static void __mix_ast_node_print(struct mix_ast_node* node, int level);

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_var_decl_node(mix_ast_node_type_t type) {
    struct mix_ast_var_decl_node* node = malloc(sizeof(struct mix_ast_var_decl_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    qbuf_init(&node->name);
    node->type = NULL;
    node->rhs = NULL;
    return &node->n;
}

static void __release_var_decl_node(struct mix_ast_node* n) {
    struct mix_ast_var_decl_node* node = container_of(n, struct mix_ast_var_decl_node, n);
    qbuf_destroy(&node->name);
    mix_type_release(node->type);
    mix_ast_node_release(node->rhs);
    free(node);
}

static void __print_var_decl_node(struct mix_ast_node* n, int level) {
    struct mix_ast_var_decl_node* node = container_of(n, struct mix_ast_var_decl_node, n);
    printf("%*s", level, "");
    printf("name [%s]", make_tmp_str_s(qbuf_get_ref(&node->name)));
    printf(", type [%s]", make_tmp_str_s(qbuf_get_ref(&node->type->name)));
    if (node->rhs) {
        printf(", init value\n");
        __mix_ast_node_print(node->rhs, level + 4);
    }
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_branch_node(mix_ast_node_type_t type) {
    struct mix_ast_branch_node* node = malloc(sizeof(struct mix_ast_branch_node));
    if (!node) {
        return NULL;
    }
    __init_base_node(&node->n, type);
    vector_init(&node->cond_stat_list, sizeof(struct mix_ast_cond_stat));
    return &node->n;
}

static void __release_cond_stat(void* item, void* nil) {
    struct mix_ast_cond_stat* cs = (struct mix_ast_cond_stat*)item;
    mix_ast_node_release(cs->cond);
    mix_ast_node_release(cs->stat);
}

static void __release_branch_node(struct mix_ast_node* n) {
    struct mix_ast_branch_node* node = container_of(n, struct mix_ast_branch_node, n);
    vector_destroy(&node->cond_stat_list, NULL, __release_cond_stat);
    free(node);
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_binop_node(mix_ast_node_type_t type) {
    struct mix_ast_binop_node* node = malloc(sizeof(struct mix_ast_binop_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->en.n, type);
    node->op = MIX_BINOP_UNKNOWN;
    node->lhs = NULL;
    node->rhs = NULL;
    node->en.tov.type = MIX_TOV_UNKNOWN;
    return &node->en.n;
}

static void __release_binop_node(struct mix_ast_node* n) {
    struct mix_ast_exp_node* en = container_of(n, struct mix_ast_exp_node, n);
    struct mix_ast_binop_node* node = container_of(en, struct mix_ast_binop_node, en);
    if (node->lhs) {
        mix_ast_node_release(&node->lhs->n);
    }
    if (node->rhs) {
        mix_ast_node_release(&node->rhs->n);
    }
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_node_list_node(mix_ast_node_type_t type) {
    struct mix_ast_node_list_node* node = malloc(sizeof(struct mix_ast_node_list_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    vector_init(&node->node_list, sizeof(struct mix_ast_node*));
    return &node->n;
}

static void __release_node_in_vec(void* item, void* nil) {
    struct mix_ast_node* node = (struct mix_ast_node*)(*(void**)item);
    mix_ast_node_release(node);
}

static void __release_node_list_node(struct mix_ast_node* n) {
    struct mix_ast_node_list_node* node = container_of(n, struct mix_ast_node_list_node, n);
    vector_destroy(&node->node_list, NULL, __release_node_in_vec);
    free(node);
}

static int __print_node_in_vec(void* item, void* arg) {
    int level = *(int*)arg;
    __mix_ast_node_print((struct mix_ast_node*)(*(void**)item), level + 4);
    return 0;
}

static void __print_node_list_node(struct mix_ast_node* n, int level) {
    struct mix_ast_node_list_node* node = container_of(n, struct mix_ast_node_list_node, n);
    vector_foreach(&node->node_list, &level, __print_node_in_vec);
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_exp_node(mix_ast_node_type_t type) {
    struct mix_ast_exp_node* node = malloc(sizeof(struct mix_ast_exp_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    node->tov.type = MIX_TOV_UNKNOWN;
#ifndef NDEBUG
    qbuf_init(&node->name);
#endif
    return &node->n;
}

static void __release_exp_node(struct mix_ast_node* n) {
    struct mix_ast_exp_node* node = container_of(n, struct mix_ast_exp_node, n);
    mix_type_or_value_destroy(&node->tov);
#ifndef NDEBUG
    qbuf_destroy(&node->name);
#endif
    free(node);
}

static void __print_exp_node(struct mix_ast_node* n, int level) {
    struct mix_ast_exp_node* node = container_of(n, struct mix_ast_exp_node, n);
    printf("%*s", level, "");
    struct mix_type* type = mix_type_or_value_get_type(&node->tov);
    printf("exp type [%s]\n", make_tmp_str_s(qbuf_get_ref(&type->name)));
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_import_node(mix_ast_node_type_t type) {
    struct mix_ast_import_node* node = malloc(sizeof(struct mix_ast_import_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    vector_init(&node->name_list, sizeof(struct qbuf*));
    return &node->n;
}

static void __destroy_import_name(void* item, void* nil) {
    struct qbuf* name = (struct qbuf*)(*(void**)item);
    qbuf_destroy(name);
    free(name);
}

static void __release_import_node(struct mix_ast_node* n) {
    struct mix_ast_import_node* node = container_of(n, struct mix_ast_import_node, n);
    vector_destroy(&node->name_list, NULL, __destroy_import_name);
    free(node);
}

static int __concat_lib_name(void* item, void* arg) {
    struct qbuf* lib_list = (struct qbuf*)arg;
    struct qbuf* lib = (struct qbuf*)(*(void**)item);
    if (!qbuf_empty(lib_list)) {
        qbuf_append_c(lib_list, ',');
    }
    qbuf_append(lib_list, qbuf_data(lib), qbuf_size(lib));
    return 0;
}

static void __print_import_node(struct mix_ast_node* n, int level) {
    struct mix_ast_import_node* node = container_of(n, struct mix_ast_import_node, n);

    struct qbuf lib_list;
    qbuf_init(&lib_list);
    vector_foreach(&node->name_list, &lib_list, __concat_lib_name);

    printf("%*s", level, "");
    printf("import [%s]\n", make_tmp_str_s(qbuf_get_ref(&lib_list)));
    qbuf_destroy(&lib_list);
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_import_alias_node(mix_ast_node_type_t type) {
    struct mix_ast_import_alias_node* node = malloc(sizeof(struct mix_ast_import_alias_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    qbuf_init(&node->orig);
    qbuf_init(&node->alias);
    return &node->n;
}

static void __release_import_alias_node(struct mix_ast_node* n) {
    struct mix_ast_import_alias_node* node = container_of(n, struct mix_ast_import_alias_node, n);
    qbuf_destroy(&node->orig);
    qbuf_destroy(&node->alias);
    free(node);
}

/* -------------------------------------------------------------------------- */

static struct mix_ast_node* __create_funcall_node(mix_ast_node_type_t type) {
    struct mix_ast_funcall_node* node = malloc(sizeof(struct mix_ast_funcall_node));
    if (!node) {
        return NULL;
    }

    __init_base_node(&node->n, type);
    node->func = NULL;
    vector_init(&node->argv, sizeof(struct mix_ast_node*));
    return &node->n;
}

static void __destroy_func_argv(void* item, void* nil) {
    struct mix_ast_node* node = (struct mix_ast_node*)(*(void**)item);
    mix_ast_node_release(node);
}

static void __release_funcall_node(struct mix_ast_node* n) {
    struct mix_ast_funcall_node* node = container_of(n, struct mix_ast_funcall_node, n);
    mix_ast_node_release(&node->func->n);
    vector_destroy(&node->argv, NULL, __destroy_func_argv);
}

static int __print_func_arg(void* item, void* arg) {
    int level = *(int*)arg;
    struct mix_ast_node* node = (struct mix_ast_node*)(*(void**)item);
    __mix_ast_node_print(node, level + 4);
    return 0;
}

static void __print_funcall_node(struct mix_ast_node* n, int level) {
    struct mix_ast_funcall_node* node = container_of(n, struct mix_ast_funcall_node, n);
    printf("%*s", level, "");
    printf("funcall ");
#ifndef NDEBUG
    printf("name [%s], ", make_tmp_str_s(qbuf_get_ref(&node->func->name)));
#endif
    printf("arg list (%u):\n", vector_size(&node->argv));
    vector_foreach(&node->argv, &level, __print_func_arg);
}

/* -------------------------------------------------------------------------- */

static const struct {
    struct mix_ast_node* (*create_node)(mix_ast_node_type_t);
    void (*release_node)(struct mix_ast_node*);
    void (*print_node)(struct mix_ast_node*, int level);
} g_node_handler[] = {
    {__create_var_decl_node, __release_var_decl_node, __print_var_decl_node},
    {__create_branch_node, __release_branch_node},
    {__create_binop_node, __release_binop_node},
    {__create_node_list_node, __release_node_list_node, __print_node_list_node},
    {__create_exp_node, __release_exp_node, __print_exp_node},
    {__create_import_node, __release_import_node, __print_import_node},
    {__create_import_alias_node, __release_import_alias_node},
    {__create_funcall_node, __release_funcall_node, __print_funcall_node},
    {NULL, NULL},
};

struct mix_ast_node* mix_ast_node_create(mix_ast_node_type_t type) {
    return g_node_handler[type].create_node(type);
}

void mix_ast_node_release(struct mix_ast_node* node) {
    if (node) {
        g_node_handler[node->type].release_node(node);
    }
}

static void __mix_ast_node_print(struct mix_ast_node* node, int level) {
    if (node) {
        g_node_handler[node->type].print_node(node, level);
    }
}

void mix_ast_node_do_print(const char* file, int line, struct mix_ast_node* node) {
    printf("----- ast print begin [%s:%u] -----\n", file, line);
    __mix_ast_node_print(node, 0);
    printf("----- ast print end -----\n");
}
