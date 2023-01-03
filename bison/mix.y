%{
#include "runtime/mix_context.h"
#include "runtime/mix_identifier.h"
#include "parser/mix_parser.h"
#include "lex/mix_lex.h"
#include "cutils/utils.h" /* container_of() */
#include "misc/debug_utils.h"
#include <stdio.h> /* EOF */

#include "mix.tab.h"

/* mix token type => bison token type */
static const int g_m2b_type[] = {
    YYUNDEF,
    YYEOF,
    0,
    BISON_LITERAL_STRING,
    BISON_INTEGER,
    BISON_FLOAT,
    BISON_OP_LSHIFT,
    BISON_OP_RSHIFT,
    BISON_OP_LSHIFT_ASSIGN,
    BISON_OP_RSHIFT_ASSIGN,
    BISON_OP_ADD_ASSIGN,
    BISON_OP_SUB_ASSIGN,
    BISON_OP_MUL_ASSIGN,
    BISON_OP_DIV_ASSIGN,
    BISON_OP_MOD_ASSIGN,
    BISON_OP_AND_ASSIGN,
    BISON_OP_OR_ASSIGN,
    BISON_OP_XOR_ASSIGN,
    BISON_OP_LOGICAL_OR,
    BISON_OP_LOGICAL_AND,
    BISON_OP_EQUAL,
    BISON_OP_NOT_EQUAL,
    BISON_OP_GREATER_EQUAL,
    BISON_OP_LESS_EQUAL,
    BISON_SYM_IDENTIFIER,
    BISON_SYM_SCOPE_SPECIFIER,
    BISON_SYM_RIGHT_ARROW,
    BISON_SYM_GENERICS_LEFT_MARK,
    BISON_SYM_GENERICS_RIGHT_MARK,
    BISON_SYM_VARIADIC_ARG,
    BISON_KEYWORD_as,
    BISON_KEYWORD_async,
    BISON_KEYWORD_await,
    BISON_KEYWORD_break,
    BISON_KEYWORD_cast,
    BISON_KEYWORD_continue,
    BISON_KEYWORD_do,
    BISON_KEYWORD_else,
    BISON_KEYWORD_enum,
    BISON_KEYWORD_export,
    BISON_KEYWORD_extern,
    BISON_KEYWORD_f32,
    BISON_KEYWORD_f64,
    BISON_KEYWORD_final,
    BISON_KEYWORD_for,
    BISON_KEYWORD_func,
    BISON_KEYWORD_i16,
    BISON_KEYWORD_i32,
    BISON_KEYWORD_i64,
    BISON_KEYWORD_i8,
    BISON_KEYWORD_if,
    BISON_KEYWORD_impl,
    BISON_KEYWORD_import,
    BISON_KEYWORD_in,
    BISON_KEYWORD_let,
    BISON_KEYWORD_macro,
    BISON_KEYWORD_override,
    BISON_KEYWORD_return,
    BISON_KEYWORD_self,
    BISON_KEYWORD_str,
    BISON_KEYWORD_struct,
    BISON_KEYWORD_trait,
    BISON_KEYWORD_typedef,
    BISON_KEYWORD_typeof,
    BISON_KEYWORD_var,
    BISON_KEYWORD_virtual,
    BISON_KEYWORD_while,
    BISON_KEYWORD_yield,
};

static int yylex(YYSTYPE* lvalp, struct mix_lex* lex, const char* buf, uint32_t sz, struct logger* l) {
    mix_token_type_t type = mix_lex_get_next_token(lex, &lvalp->token);
    if (type == MIX_TT_EOF) {
        return YYEOF;
    }

    if (type == MIX_TT_CHAR) {
        return lvalp->token.c;
    }

    return g_m2b_type[type];
}

static void yyerror(struct mix_parser* parser, struct mix_lex* lex, const char* buf, uint32_t sz,
                    struct logger* l, const char *msg) {
    printf("line [%u] column [%u] error: %s\n", lex->linenum, lex->lineoff, msg);
}

static void bison_ast_push_arg_node(void* item, void* arg) {
    struct vector* argv = (struct vector*)arg;
    vector_push_back(argv, item);
}
%}

%union {
    char c;
    struct qbuf* buf;
    union mix_token_info token;
    struct mix_ast_node* node;
    struct mix_type* type;
}

// reentrant yylex()
%define api.pure full

// params of yylex()
%lex-param {struct mix_lex* lex} {const char* buf} {uint32_t buf_sz} {struct logger* logger}

// debug settings
%define parse.lac full
%define parse.error detailed
%define parse.trace

// params of yyparse(), should include those passed to yylex()
%parse-param {struct mix_parser* parser} {struct mix_lex* lex} {const char* buf} {uint32_t buf_sz} {struct logger* logger}

%token BISON_LITERAL_STRING
%token BISON_INTEGER
%token BISON_FLOAT

%token BISON_OP_LSHIFT
%token BISON_OP_RSHIFT
%token BISON_OP_LSHIFT_ASSIGN
%token BISON_OP_RSHIFT_ASSIGN
%token BISON_OP_ADD_ASSIGN
%token BISON_OP_SUB_ASSIGN
%token BISON_OP_MUL_ASSIGN
%token BISON_OP_DIV_ASSIGN
%token BISON_OP_MOD_ASSIGN
%token BISON_OP_AND_ASSIGN
%token BISON_OP_OR_ASSIGN
%token BISON_OP_XOR_ASSIGN
%token BISON_OP_LOGICAL_OR
%token BISON_OP_LOGICAL_AND
%token BISON_OP_EQUAL
%token BISON_OP_NOT_EQUAL
%token BISON_OP_GREATER_EQUAL
%token BISON_OP_LESS_EQUAL

%token BISON_SYM_IDENTIFIER
%token BISON_SYM_SCOPE_SPECIFIER
%token BISON_SYM_RIGHT_ARROW
%token BISON_SYM_GENERICS_LEFT_MARK
%token BISON_SYM_GENERICS_RIGHT_MARK
%token BISON_SYM_VARIADIC_ARG

%token BISON_KEYWORD_as
%token BISON_KEYWORD_async
%token BISON_KEYWORD_await
%token BISON_KEYWORD_break
%token BISON_KEYWORD_cast
%token BISON_KEYWORD_continue
%token BISON_KEYWORD_do
%token BISON_KEYWORD_else
%token BISON_KEYWORD_enum
%token BISON_KEYWORD_export
%token BISON_KEYWORD_extern
%token BISON_KEYWORD_f32
%token BISON_KEYWORD_f64
%token BISON_KEYWORD_final
%token BISON_KEYWORD_for
%token BISON_KEYWORD_func
%token BISON_KEYWORD_i16
%token BISON_KEYWORD_i32
%token BISON_KEYWORD_i64
%token BISON_KEYWORD_i8
%token BISON_KEYWORD_if
%token BISON_KEYWORD_impl
%token BISON_KEYWORD_import
%token BISON_KEYWORD_in
%token BISON_KEYWORD_let
%token BISON_KEYWORD_macro
%token BISON_KEYWORD_override
%token BISON_KEYWORD_return
%token BISON_KEYWORD_self
%token BISON_KEYWORD_str
%token BISON_KEYWORD_struct
%token BISON_KEYWORD_trait
%token BISON_KEYWORD_typedef
%token BISON_KEYWORD_typeof
%token BISON_KEYWORD_var
%token BISON_KEYWORD_virtual
%token BISON_KEYWORD_while
%token BISON_KEYWORD_yield

%type   <token> BISON_LITERAL_STRING
%type   <token> BISON_INTEGER
%type   <token> BISON_FLOAT

%type   <token> BISON_OP_LSHIFT
%type   <token> BISON_OP_RSHIFT
%type   <token> BISON_OP_LSHIFT_ASSIGN
%type   <token> BISON_OP_RSHIFT_ASSIGN
%type   <token> BISON_OP_ADD_ASSIGN
%type   <token> BISON_OP_SUB_ASSIGN
%type   <token> BISON_OP_MUL_ASSIGN
%type   <token> BISON_OP_DIV_ASSIGN
%type   <token> BISON_OP_MOD_ASSIGN
%type   <token> BISON_OP_AND_ASSIGN
%type   <token> BISON_OP_OR_ASSIGN
%type   <token> BISON_OP_XOR_ASSIGN
%type   <token> BISON_OP_LOGICAL_OR
%type   <token> BISON_OP_LOGICAL_AND
%type   <token> BISON_OP_EQUAL
%type   <token> BISON_OP_NOT_EQUAL
%type   <token> BISON_OP_GREATER_EQUAL
%type   <token> BISON_OP_LESS_EQUAL

%type   <token> BISON_SYM_IDENTIFIER
%type   <token> BISON_SYM_SCOPE_SPECIFIER
%type   <token> BISON_SYM_RIGHT_ARROW
%type   <token> BISON_SYM_GENERICS_LEFT_MARK
%type   <token> BISON_SYM_GENERICS_RIGHT_MARK
%type   <token> BISON_SYM_VARIADIC_ARG

%type   <token> BISON_KEYWORD_as
%type   <token> BISON_KEYWORD_async
%type   <token> BISON_KEYWORD_await
%type   <token> BISON_KEYWORD_break
%type   <token> BISON_KEYWORD_cast
%type   <token> BISON_KEYWORD_continue
%type   <token> BISON_KEYWORD_do
%type   <token> BISON_KEYWORD_else
%type   <token> BISON_KEYWORD_enum
%type   <token> BISON_KEYWORD_export
%type   <token> BISON_KEYWORD_extern
%type   <token> BISON_KEYWORD_f32
%type   <token> BISON_KEYWORD_f64
%type   <token> BISON_KEYWORD_final
%type   <token> BISON_KEYWORD_for
%type   <token> BISON_KEYWORD_func
%type   <token> BISON_KEYWORD_i8
%type   <token> BISON_KEYWORD_i16
%type   <token> BISON_KEYWORD_i32
%type   <token> BISON_KEYWORD_i64
%type   <token> BISON_KEYWORD_if
%type   <token> BISON_KEYWORD_impl
%type   <token> BISON_KEYWORD_import
%type   <token> BISON_KEYWORD_in
%type   <token> BISON_KEYWORD_let
%type   <token> BISON_KEYWORD_macro
%type   <token> BISON_KEYWORD_override
%type   <token> BISON_KEYWORD_return
%type   <token> BISON_KEYWORD_self
%type   <token> BISON_KEYWORD_str
%type   <token> BISON_KEYWORD_struct
%type   <token> BISON_KEYWORD_trait
%type   <token> BISON_KEYWORD_typedef
%type   <token> BISON_KEYWORD_typeof
%type   <token> BISON_KEYWORD_var
%type   <token> BISON_KEYWORD_virtual
%type   <token> BISON_KEYWORD_while
%type   <token> BISON_KEYWORD_yield

%type   <node> constant
%type   <node> expression
%type   <node> conditional_expression
%type   <node> logical_or_expression
%type   <node> logical_and_expression
%type   <node> inclusive_or_expression
%type   <node> exclusive_or_expression
%type   <node> and_expression
%type   <node> equality_expression
%type   <node> relational_expression
%type   <node> shift_expression
%type   <node> additive_expression
%type   <node> multiplicative_expression
%type   <node> unary_expression
%type   <node> postfix_expression

%type   <node> statement
%type   <node> variable_declaration_clause
%type   <node> selection_statement
%type   <node> compound_statement
%type   <node> optional_statement_list
%type   <node> variable_declaration_with_optional_assignment
%type   <node> variable_declaration_with_optional_assignment_list
%type   <node> import_item_list
%type   <node> import_statement
%type   <node> expression_or_braced_initializer_list
%type   <node> variable

%type   <c> additive_operator
%type   <c> multiplicative_operator

%type   <type> type_specifier
%type   <type> builtin_type_specifier
%type   <type> user_type_specifier

%type   <buf> nested_import_scope
%type   <buf> import_item
%type   <buf> nested_name_specifier
%%

block
: optional_statement_list {
    parser->ast_root = $1;
    mix_ast_node_print(parser->ast_root);
 }
;

optional_statement_list
: %empty {
    $$ = NULL;
 }
| optional_statement_list statement {
    if ($2) {
        if (!$1) {
            $1 = mix_ast_node_create(MIX_AST_NODE_TYPE_NODE_LIST);
            if (!$1) {
                logger_error(logger, "create node list node failed.");
                return YYerror;
            }
        }

        struct mix_ast_node_list_node* node = container_of($1, struct mix_ast_node_list_node, n);
        int ret = vector_push_back(&node->node_list, &$2);
        if (ret != 0) {
            logger_error(logger, "push node list node failed.");
            return YYerror;
        }
    }

    $$ = $1;
 }
;

/* -------------------------------------------------------------------------- */

statement
: ';' { $$ = NULL; }
| expression ';' { $$ = $1; }
| assignment_statement {}
| variable_declaration_clause ';' { $$ = $1; }
| selection_statement {}
| iteration_statement {}
| jump_statement {}
| import_statement { $$ = $1; }
| export_statement {}
| compound_statement {}
| typedef_statement {}
| enum_declaration {}
| function_definition {}
| struct_definition {}
| struct_impl_definition {}
| trait_definition {}
| trait_impl_definition {}
;

assignment_statement
: variable '=' expression ';'
| variable '=' braced_initializer ';'
| variable assignment_operator expression ';'
;

variable
: BISON_SYM_IDENTIFIER {
    struct mix_identifier* id = mix_parser_lookup_identifier(parser, &$1.s);
    if (!id) {
        logger_error(logger, "cannot find variable [%s].", make_tmp_str_s(&$1.s));
        return YYerror;
    }

    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_EXP);
    if (!$$) {
        logger_error(logger, "create exp node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* node = container_of($$, struct mix_ast_exp_node, n);
    mix_type_or_value_copy_construct(&id->tov, &node->tov);
 }
| postfix_expression '[' expression ']' {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
| postfix_expression '.' BISON_SYM_IDENTIFIER {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
;

assignment_operator
: BISON_OP_ADD_ASSIGN | BISON_OP_SUB_ASSIGN | BISON_OP_MUL_ASSIGN | BISON_OP_DIV_ASSIGN | BISON_OP_MOD_ASSIGN | BISON_OP_AND_ASSIGN | BISON_OP_OR_ASSIGN | BISON_OP_XOR_ASSIGN | BISON_OP_RSHIFT_ASSIGN | BISON_OP_LSHIFT_ASSIGN
;

variable_declaration_clause
: BISON_KEYWORD_var variable_declaration_with_optional_assignment_list {
    $$ = $2;
 }
;

variable_declaration_with_optional_assignment_list
: variable_declaration_with_optional_assignment {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_NODE_LIST);
    if (!$$) {
        logger_error(logger, "create node list node failed.");
        return YYerror;
    }

    struct mix_ast_node_list_node* node = container_of($$, struct mix_ast_node_list_node, n);
    int ret = vector_push_back(&node->node_list, &$1);
    if (ret != 0) {
        logger_error(logger, "push node list node failed: out of memory.");
        return YYerror;
    }
 }
| variable_declaration_with_optional_assignment_list ',' variable_declaration_with_optional_assignment {
    struct mix_ast_node_list_node* node = container_of($1, struct mix_ast_node_list_node, n);
    int ret = vector_push_back(&node->node_list, &$3);
    if (ret != 0) {
        logger_error(logger, "push node list node failed.");
        return YYerror;
    }
    $$ = $1;
 }
;

variable_declaration_with_optional_assignment
: BISON_SYM_IDENTIFIER ':' type_specifier {
    struct mix_ast_node* n = mix_ast_node_create(MIX_AST_NODE_TYPE_VAR_DECL);
    if (!n) {
        logger_error(logger, "create var decl node failed.");
        return YYerror;
    }

    struct mix_identifier* var = mix_parser_decl_var(parser, $3, &$1.s);
    if (!var) {
        mix_ast_node_release(n);
        logger_error(logger, "declare variable [%s] failed: exists.", make_tmp_str_s(&$1.s));
        return YYerror;
    }

    struct mix_ast_var_decl_node* node = container_of(n, struct mix_ast_var_decl_node, n);
    qbuf_init(&node->name);
    qbuf_assign(&node->name, $1.s.base, $1.s.size);
    node->type = $3;

    $$ = n;
 }
| BISON_SYM_IDENTIFIER ':' type_specifier '=' braced_initializer {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
| BISON_SYM_IDENTIFIER ':' type_specifier '=' expression {
    struct mix_ast_exp_node* exp_node = container_of($5, struct mix_ast_exp_node, n);
    struct mix_type* exp_type = mix_type_or_value_get_type(&exp_node->tov);
    if ($3 != exp_type) {
        char buf1[1024], buf2[1024];
        make_tmp_str(qbuf_get_ref(&$3->name), buf1);
        make_tmp_str(qbuf_get_ref(&exp_type->name), buf2);
        logger_error(logger, "type of lhs [%s] != type of rhs [%s].", buf1, buf2);
        return YYerror;
    }

    struct mix_ast_node* n = mix_ast_node_create(MIX_AST_NODE_TYPE_VAR_DECL);
    if (!n) {
        logger_error(logger, "create var decl node failed.");
        return YYerror;
    }

    struct mix_identifier* var = mix_parser_decl_var(parser, $3, &$1.s);
    if (!var) {
        mix_ast_node_release(n);
        logger_error(logger, "declare variable [%s] failed: exists.", make_tmp_str_s(&$1.s));
        return YYerror;
    }

    struct mix_ast_var_decl_node* node = container_of(n, struct mix_ast_var_decl_node, n);
    qbuf_init(&node->name);
    qbuf_assign(&node->name, $1.s.base, $1.s.size);
    node->type = $3;
    node->rhs = $5;

    $$ = n;
 }
| BISON_SYM_IDENTIFIER '=' expression {
    struct mix_ast_exp_node* exp_node = container_of($3, struct mix_ast_exp_node, n);

    struct mix_ast_node* n = mix_ast_node_create(MIX_AST_NODE_TYPE_VAR_DECL);
    if (!n) {
        logger_error(logger, "create var decl node failed.");
        return YYerror;
    }

    struct mix_type* exp_type = mix_type_or_value_get_type(&exp_node->tov);
    struct mix_identifier* var = mix_parser_decl_var(parser, exp_type, &$1.s);
    if (!var) {
        mix_ast_node_release(n);
        logger_error(logger, "declare variable [%s] failed: exists.", make_tmp_str_s(&$1.s));
        return YYerror;
    }

    struct mix_ast_var_decl_node* node = container_of(n, struct mix_ast_var_decl_node, n);
    qbuf_init(&node->name);
    qbuf_assign(&node->name, $1.s.base, $1.s.size);
    mix_type_acquire(exp_type);
    node->type = exp_type;
    node->rhs = $3;

    $$ = n;
 }
;

braced_initializer
: '{' '}'
| '{' initializer_clause_list '}'
| '{' initializer_clause_list ',' '}'
;

initializer_clause_list
: initializer_clause
| initializer_clause_list ',' initializer_clause
;

initializer_clause
: BISON_SYM_IDENTIFIER '=' expression
| BISON_SYM_IDENTIFIER '=' braced_initializer
;

selection_statement
: BISON_KEYWORD_if expression compound_statement {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_BRANCH);
    struct mix_ast_branch_node* branch_node = container_of($$, struct mix_ast_branch_node, n);

    struct mix_ast_cond_stat cs = {
        .cond = $2,
        .stat = $3,
    };
    vector_push_back(&branch_node->cond_stat_list, &cs);
 }
| BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else selection_statement {}
| BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else compound_statement {}
;

iteration_statement
: BISON_KEYWORD_while expression compound_statement
| BISON_KEYWORD_do compound_statement BISON_KEYWORD_while expression ';'
| BISON_KEYWORD_for BISON_SYM_IDENTIFIER BISON_KEYWORD_in expression compound_statement
;

compound_statement
: '{' optional_statement_list '}' {
    $$ = $2;
 }
;

jump_statement
: BISON_KEYWORD_continue ';'
| BISON_KEYWORD_break ';'
| BISON_KEYWORD_return ';'
| BISON_KEYWORD_return expression ';'
;

typedef_statement
: BISON_KEYWORD_typedef type_specifier BISON_SYM_IDENTIFIER ';'
;

/* -------------------------------------------------------------------------- */

export_statement
: BISON_KEYWORD_export identifier_list ';'
;

identifier_list
: BISON_SYM_IDENTIFIER
| identifier_list ',' BISON_SYM_IDENTIFIER
;

import_statement
: BISON_KEYWORD_import import_item_list ';' {
    $$ = $2;
 }
| BISON_KEYWORD_import import_item BISON_KEYWORD_as BISON_SYM_IDENTIFIER ';' {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
;

import_item_list
: import_item {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_IMPORT);
    if (!$$) {
        logger_error(logger, "create ast node failed.");
        return YYerror;
    }

    struct mix_ast_import_node* node = container_of($$, struct mix_ast_import_node, n);
    int ret = vector_push_back(&node->name_list, &$1);
    if (ret != 0) {
        logger_error(logger, "add import item failed: out of memory.");
        return YYerror;
    }

    mix_retcode_t rc = mix_parser_import_lib(parser, qbuf_get_ref($1), NULL);
    if (rc != MIX_RC_OK) {
        logger_error(logger, "import lib [%s] failed.", make_tmp_str_s(qbuf_get_ref($1)));
        return YYerror;
    }
 }
| import_item_list ',' import_item {
    struct mix_ast_import_node* node = container_of($1, struct mix_ast_import_node, n);
    int ret = vector_push_back(&node->name_list, &$3);
    if (ret != 0) {
        logger_error(logger, "add import item failed: out of memory.");
        return YYerror;
    }
    $$ = $1;

    mix_retcode_t rc = mix_parser_import_lib(parser, qbuf_get_ref($3), NULL);
    if (rc != MIX_RC_OK) {
        logger_error(logger, "import lib [%s] failed.", make_tmp_str_s(qbuf_get_ref($3)));
        return YYerror;
    }
 }
;

import_item
: BISON_SYM_IDENTIFIER {
    $$ = malloc(sizeof(struct qbuf));
    if (!$$) {
        logger_error(logger, "allocate qbuf failed: out of memory.");
        return YYerror;
    }
    qbuf_init($$);
    qbuf_assign($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
 }
| nested_import_scope BISON_SYM_IDENTIFIER {
    qbuf_append($1, $2.s.base, $2.s.size);
    qbuf_append_c($$, '/');
    $$ = $1;
 }
;

nested_import_scope
: BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    if (!$$) {
        logger_error(logger, "allocate qbuf failed: out of memory.");
        return YYerror;
    }
    qbuf_init($$);
    qbuf_assign($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
 }
| nested_import_scope BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER {
    qbuf_append($1, $2.s.base, $2.s.size);
    qbuf_append_c($1, '/');
    $$ = $1;
 }
;

/* -------------------------------------------------------------------------- */

expression
: conditional_expression { $$ = $1; }
;

conditional_expression
: logical_or_expression { $$ = $1; }
| logical_or_expression '?' conditional_expression ':' conditional_expression
;

logical_or_expression
: logical_and_expression { $$ = $1; }
| logical_or_expression BISON_OP_LOGICAL_OR logical_and_expression
;

logical_and_expression
: inclusive_or_expression { $$ = $1; }
| logical_and_expression BISON_OP_LOGICAL_AND inclusive_or_expression
;

inclusive_or_expression
: exclusive_or_expression { $$ = $1; }
| inclusive_or_expression '|' exclusive_or_expression
;

exclusive_or_expression
: and_expression { $$ = $1; }
| exclusive_or_expression '^' and_expression
;

and_expression
: equality_expression { $$ = $1; }
| and_expression '&' equality_expression
;

equality_expression
: relational_expression { $$ = $1; }
| equality_expression equality_operator relational_expression
;

equality_operator
: BISON_OP_EQUAL | BISON_OP_NOT_EQUAL
;

relational_expression
: shift_expression { $$ = $1; }
| relational_expression relational_operator shift_expression
;

relational_operator
: '<' | '>' | BISON_OP_LESS_EQUAL | BISON_OP_GREATER_EQUAL
;

shift_expression
: additive_expression { $$ = $1; }
| shift_expression shift_operator additive_expression
;

shift_operator
: BISON_OP_LSHIFT | BISON_OP_RSHIFT
;

additive_expression
: multiplicative_expression { $$ = $1; }
| additive_expression additive_operator multiplicative_expression {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_BINOP);
    if (!$$) {
        logger_error(logger, "create BINOP node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* en1 = container_of($1, struct mix_ast_exp_node, n);
    struct mix_ast_exp_node* en2 = container_of($3, struct mix_ast_exp_node, n);
    struct mix_ast_exp_node* en = container_of($$, struct mix_ast_exp_node, n);
    struct mix_type* exp1_type = mix_type_or_value_get_type(&en1->tov);
    struct mix_type* exp2_type = mix_type_or_value_get_type(&en2->tov);

    if (!MIX_TYPE_IS_NUM(exp1_type->value)) {
        logger_error(logger, "type of lhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp1_type->name)));
        return YYerror;
    }
    if (!MIX_TYPE_IS_NUM(exp2_type->value)) {
        logger_error(logger, "type of rhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp2_type->name)));
        return YYerror;
    }

    if (MIX_TYPE_IS_FLOAT(exp1_type->value)) {
        mix_type_acquire(exp1_type);
        en->tov.type = MIX_TOV_TYPE;
        en->tov.t = exp1_type;
    } else if (MIX_TYPE_IS_FLOAT(exp2_type->value)) {
        mix_type_acquire(exp2_type);
        en->tov.type = MIX_TOV_TYPE;
        en->tov.t = exp2_type;
    } else { /* both en1 and en2 are ints */
        mix_type_acquire(exp1_type);
        en->tov.type = MIX_TOV_TYPE;
        en->tov.t = exp1_type;
    }

    struct mix_ast_binop_node* node = container_of(en, struct mix_ast_binop_node, en);
    node->op = $2;
    node->lhs = en1;
    node->rhs = en2;
 }
;

additive_operator
: '+' { $$ = '+'; }
| '-' { $$ = '-'; }
;

multiplicative_expression
: unary_expression { $$ = $1; }
| multiplicative_expression multiplicative_operator unary_expression {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_BINOP);
    if (!$$) {
        logger_error(logger, "create BINOP node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* en1 = container_of($1, struct mix_ast_exp_node, n);
    struct mix_ast_exp_node* en2 = container_of($3, struct mix_ast_exp_node, n);
    struct mix_ast_exp_node* en = container_of($$, struct mix_ast_exp_node, n);
    struct mix_type* exp1_type = mix_type_or_value_get_type(&en1->tov);
    struct mix_type* exp2_type = mix_type_or_value_get_type(&en2->tov);

    if ($2 == '%') {
        if (!MIX_TYPE_IS_INT(exp1_type->value)) {
            logger_error(logger, "type of lhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp1_type->name)));
            return YYerror;
        }
        if (!MIX_TYPE_IS_INT(exp2_type->value)) {
            logger_error(logger, "type of rhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp1_type->name)));
            return YYerror;
        }

        mix_type_acquire(exp1_type);
        en->tov.t = exp1_type;
    } else {
        if (!MIX_TYPE_IS_NUM(exp1_type->value)) {
            logger_error(logger, "type of lhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp1_type->name)));
            return YYerror;
        }
        if (!MIX_TYPE_IS_NUM(exp2_type->value)) {
            logger_error(logger, "type of rhs [%s] is not number.", make_tmp_str_s(qbuf_get_ref(&exp2_type->name)));
            return YYerror;
        }

        if (MIX_TYPE_IS_FLOAT(exp1_type->value)) {
            mix_type_acquire(exp1_type);
            en->tov.type = MIX_TOV_TYPE;
            en->tov.t = exp1_type;
        } else if (MIX_TYPE_IS_FLOAT(exp2_type->value)) {
            mix_type_acquire(exp2_type);
            en->tov.type = MIX_TOV_TYPE;
            en->tov.t = exp2_type;
        } else { /* both en1 and en2 are ints */
            mix_type_acquire(exp1_type);
            en->tov.type = MIX_TOV_TYPE;
            en->tov.t = exp1_type;
        }
    }

    struct mix_ast_binop_node* node = container_of(en, struct mix_ast_binop_node, en);
    node->op = $2;
    node->lhs = en1;
    node->rhs = en2;
 }
;

multiplicative_operator
: '*' { $$ = '*'; }
| '/' { $$ = '/'; }
| '%' { $$ = '%'; }
;

unary_expression
: postfix_expression
| constant { $$ = $1; }
| unary_operator unary_expression {}
;

unary_operator
: arithmetic_unary_operator
| logical_unary_operator
;

arithmetic_unary_operator
: '+'
| '-'
| '~'
;

logical_unary_operator
: '!'
;

postfix_expression
: variable { $$ = $1; }
| '(' expression ')' {
    $$ = $2;
 }
| lambda {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
| BISON_SYM_IDENTIFIER generic_type_specifier {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier {
    qbuf_append($1, $2.s.base, $2.s.size);
    struct mix_identifier* id = mix_parser_lookup_identifier(parser, qbuf_get_ref($1));
    if (!id) {
        logger_error(logger, "cannot find variable [%s].", make_tmp_str_s(qbuf_get_ref($1)));
        qbuf_destroy($1);
        free($1);
        return YYerror;
    }

    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_EXP);
    if (!$$) {
        logger_error(logger, "create exp node failed.");
        qbuf_destroy($1);
        free($1);
        return YYerror;
    }

    struct mix_ast_exp_node* node = container_of($$, struct mix_ast_exp_node, n);

#ifndef NDEBUG
    qbuf_copy_construct($1, &node->name);
#endif
    qbuf_destroy($1);
    free($1);

    mix_type_or_value_copy_construct(&id->tov, &node->tov);
 }
| postfix_expression '(' expression_or_braced_initializer_list ')' {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_FUNCALL);
    if (!$$) {
        logger_error(logger, "create funcall node failed.");
        return YYerror;
    }

    struct mix_ast_node_list_node* arg_node = container_of($3, struct mix_ast_node_list_node, n);
    struct mix_ast_funcall_node* node = container_of($$, struct mix_ast_funcall_node, n);
    node->func = container_of($1, struct mix_ast_exp_node, n);
    vector_destroy(&arg_node->node_list, &node->argv, bison_ast_push_arg_node);
    mix_ast_node_release($3);
 }
| postfix_expression '(' ')'
| BISON_KEYWORD_cast BISON_SYM_GENERICS_LEFT_MARK type_specifier BISON_SYM_GENERICS_RIGHT_MARK '(' expression ')' {}
;

lambda
: function_type compound_statement
;

expression_or_braced_initializer_list
: expression {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_NODE_LIST);
    if (!$$) {
        logger_error(logger, "create node list node failed.");
        return YYerror;
    }

    struct mix_ast_node_list_node* node = container_of($$, struct mix_ast_node_list_node, n);
    int ret = vector_push_back(&node->node_list, &$1);
    if (ret != 0) {
        logger_error(logger, "push exp node failed: out of memory.");
        return YYerror;
    }
 }
| braced_initializer {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
| expression_or_braced_initializer_list ',' expression {
    struct mix_ast_node_list_node* node = container_of($1, struct mix_ast_node_list_node, n);
    int ret = vector_push_back(&node->node_list, &$3);
    if (ret != 0) {
        logger_error(logger, "push exp node failed: out of memory.");
        return YYerror;
    }

    $$ = $1;
 }
| expression_or_braced_initializer_list ',' braced_initializer {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
;

constant
: BISON_INTEGER {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_EXP);
    if (!$$) {
        logger_error(logger, "create exp node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* node = container_of($$, struct mix_ast_exp_node, n);
    node->tov.type = MIX_TOV_ATOMIC_VALUE;

    const struct qbuf_ref tname = {.base = "i32", .size = 3};
    node->tov.t = mix_parser_lookup_type(parser, &tname); /* TODO i8/i16/i64 */
    mix_type_acquire(node->tov.t);
    node->tov.l = $1.l;
 }
| BISON_FLOAT {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_EXP);
    if (!$$) {
        logger_error(logger, "create exp node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* node = container_of($$, struct mix_ast_exp_node, n);
    node->tov.type = MIX_TOV_ATOMIC_VALUE;

    const struct qbuf_ref tname = {.base = "f32", .size = 3};
    node->tov.t = mix_parser_lookup_type(parser, &tname); /* TODO f64 */
    mix_type_acquire(node->tov.t);
    node->tov.d = $1.d;
 }
| BISON_LITERAL_STRING {
    $$ = mix_ast_node_create(MIX_AST_NODE_TYPE_EXP);
    if (!$$) {
        logger_error(logger, "create exp node failed.");
        return YYerror;
    }

    struct mix_ast_exp_node* node = container_of($$, struct mix_ast_exp_node, n);
    node->tov.type = MIX_TOV_SHARED_VALUE;

    struct mix_shared_value* v = mix_shared_value_create();
    if (!v) {
        logger_error(logger, "create shared value failed: out of memory.");
        return YYerror;
    }

    mix_shared_value_acquire(v);
    node->tov.v = v;

    const struct qbuf_ref tname = {.base = "str", .size = 3};
    v->type = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire(v->type);
    qbuf_init(&v->s);
    qbuf_assign(&v->s, $1.s.base, $1.s.size);
 }
;

/* -------------------------------------------------------------------------- */

enum_declaration
: BISON_KEYWORD_enum '{' optional_enum_member_list '}'
;

optional_enum_member_list
: %empty
| enum_member_list
| enum_member_list ','
;

enum_member_list
: BISON_SYM_IDENTIFIER
| BISON_SYM_IDENTIFIER '=' BISON_INTEGER
| enum_member_list ',' BISON_SYM_IDENTIFIER
| enum_member_list ',' BISON_SYM_IDENTIFIER '=' BISON_INTEGER
;

/* -------------------------------------------------------------------------- */

function_definition
: function_declaration compound_statement
;

function_declaration
: no_returned_value_function_declaration
| no_returned_value_function_declaration BISON_SYM_RIGHT_ARROW type_specifier
;

no_returned_value_function_declaration
: BISON_KEYWORD_func BISON_SYM_IDENTIFIER optional_generic_type_specifier '(' function_declaration_parameter_list ')'
;

function_declaration_parameter_list
: %empty
| BISON_SYM_VARIADIC_ARG
| parameter_and_type_list
| parameter_and_type_list ',' BISON_SYM_VARIADIC_ARG
;

parameter_and_type_list
: parameter_and_type
| parameter_and_type_list ',' parameter_and_type
;

parameter_and_type
: type_specifier
| BISON_SYM_IDENTIFIER ':' type_specifier
;

function_type
: no_returned_value_function_type
| no_returned_value_function_type BISON_SYM_RIGHT_ARROW type_specifier
;

no_returned_value_function_type
: BISON_KEYWORD_func optional_generic_type_specifier '(' function_declaration_parameter_list ')'
;

/* -------------------------------------------------------------------------- */

struct_definition
: BISON_KEYWORD_struct BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_struct_member_list '}'
;

optional_generic_type_specifier
: %empty
| generic_type_specifier
;

generic_type_specifier
: BISON_SYM_GENERICS_LEFT_MARK type_specifier_list BISON_SYM_GENERICS_RIGHT_MARK {
    logger_error(logger, "not implemeneted.");
    return YYerror;
 }
;

type_specifier_list
: type_specifier
| type_specifier_list ',' type_specifier
;

optional_struct_member_list
: %empty
| optional_struct_member_list struct_member
;

struct_member
: variable_declaration_clause ';'
;

struct_impl_definition
: BISON_KEYWORD_impl BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_member_function_list '}'
;

optional_member_function_list
: %empty
| optional_member_function_list member_function_definition
| optional_member_function_list member_function_declaration ';'
;

member_function_definition
: member_function_declaration compound_statement
;

member_function_declaration
: no_returned_value_member_function_declaration
| no_returned_value_member_function_declaration BISON_SYM_RIGHT_ARROW member_function_return_type
;

member_function_return_type
: type_specifier
| BISON_KEYWORD_self
;

no_returned_value_member_function_declaration
: BISON_KEYWORD_func BISON_SYM_IDENTIFIER optional_generic_type_specifier '(' member_function_declaration_parameter_list ')'
;

member_function_declaration_parameter_list
: member_function_first_self_param
| member_function_first_self_param ',' BISON_SYM_VARIADIC_ARG
| member_function_first_self_param ',' parameter_and_type_list
| member_function_first_self_param ',' parameter_and_type_list ',' BISON_SYM_VARIADIC_ARG
| function_declaration_parameter_list
;

member_function_first_self_param
: BISON_KEYWORD_self
| BISON_SYM_IDENTIFIER ':' BISON_KEYWORD_self
;

/* -------------------------------------------------------------------------- */

trait_definition
: BISON_KEYWORD_trait BISON_SYM_IDENTIFIER optional_generic_type_specifier optional_constraint_trait_specifier '{' optional_trait_member_list '}'
;

optional_constraint_trait_specifier
: %empty
| ':' constraint_trait_list
;

constraint_trait_list
: user_type_specifier
| constraint_trait_list ',' user_type_specifier
;

optional_trait_member_list
: %empty
| optional_trait_member_list trait_member
;

trait_member
: member_function_declaration ';'
| member_function_definition
;

trait_impl_definition
: BISON_KEYWORD_impl BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_KEYWORD_for BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_member_function_list '}'
;

/* -------------------------------------------------------------------------- */

type_specifier
: builtin_type_specifier {
    $$ = $1;
 }
| user_type_specifier {
    $$ = $1;
 }
;

builtin_type_specifier
: BISON_KEYWORD_f32 {
    struct qbuf_ref tname = {.base = "f32", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_f64 {
    struct qbuf_ref tname = {.base = "f64", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_i8 {
    struct qbuf_ref tname = {.base = "i8", .size = 2};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_i16 {
    struct qbuf_ref tname = {.base = "i16", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_i32 {
    struct qbuf_ref tname = {.base = "i32", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_i64 {
    struct qbuf_ref tname = {.base = "i64", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
| BISON_KEYWORD_str {
    struct qbuf_ref tname = {.base = "str", .size = 3};
    $$ = mix_parser_lookup_type(parser, &tname);
    mix_type_acquire($$);
 }
;

user_type_specifier
: BISON_SYM_IDENTIFIER optional_generic_type_specifier {
    logger_error(logger, "user-defined types are not supported.");
    return YYerror;
 }
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier {
    logger_error(logger, "user-defined types are not supported.");
    return YYerror;
 }
| function_type {
    logger_error(logger, "function types are not supported.");
    return YYerror;
 }
;

nested_name_specifier
: BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    if (!$$) {
        logger_error(logger, "allocate qbuf failed: out of memory.");
        return YYerror;
    }

    qbuf_init($$);
    qbuf_append($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
 }
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER {
    qbuf_append($1, $2.s.base, $2.s.size);
    qbuf_append_c($1, '/');
    $$ = $1;
 }
;
%%
