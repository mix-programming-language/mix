%{
#include "mix/mix.h"
#include "src/lex.h"
#include "src/identifier.h"
#include "src/parser.h"
#include "src/debug_utils.h"
#include "src/type_or_value.h"
#include "cutils/utils.h" /* container_of() */
#include <stdlib.h> /* exit() */
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

static int yylex(YYSTYPE* lvalp, struct mix_lex* lex, const char* buf, uint32_t sz) {
    mix_token_type_t type = mix_lex_get_next_token(lex, &lvalp->token);
    if (type == MIX_TT_EOF) {
        return YYEOF;
    }

    if (type == MIX_TT_CHAR) {
        return lvalp->token.c;
    }

    return g_m2b_type[type];
}

static void yyerror(struct mix_context* ctx, struct mix_lex* lex, const char* buf, uint32_t sz,
                    const char* prefix, const char *msg) {
    printf("error: %s\n", msg);
    exit(-1);
}

static struct mix_type* bison_lookup_type(struct mix_context* ctx, const char* s, unsigned int sz) {
    struct qbuf_ref tname = {.base = s, .size = sz};
    struct mix_type* t = mix_parser_lookup_type(ctx, &tname);
    if (!t) {
        logger_error(ctx->logger, "impossible! builtin type [%s] not found.", s);
        exit(-1);
    }
    return t;
}

/*
  ___res, ___a and ___b are atomic values
  TODO handle other atomic types(i8, i16, i64, f64)
*/
#define BISON_BIN_OP(___res, ___a, ___op, ___b)                         \
    do {                                                                \
        if (MIX_TYPE_IS_FLOAT((___a).t->value)) {                       \
            mix_type_acquire((___a).t);                                 \
            (___res).t = (___a).t;                                      \
            if (MIX_TYPE_IS_INT((___b).t->value)) {                     \
                (___res).d = (___a).d ___op (___b).l;                   \
            } else if (MIX_TYPE_IS_FLOAT((___b).t->value)) {            \
                (___res).d = (___a).d ___op (___b).d;                   \
            } else {                                                    \
                logger_error(ctx->logger, "invalid type of operand 2 [%s].", mix_get_type_name((___b).t->value)); \
                exit(-1);                                               \
            }                                                           \
        } else if (MIX_TYPE_IS_INT((___a).t->value)) {                  \
            if (MIX_TYPE_IS_INT((___b).t->value)) {                     \
                (___res).l = (___a).l ___op (___b).l;                   \
                mix_type_acquire((___a).t);                             \
                (___res).t = (___a).t;                                  \
            } else if (MIX_TYPE_IS_FLOAT((___b).t->value)) {            \
                (___res).d = (___a).l ___op (___b).d;                   \
                mix_type_acquire((___b).t);                             \
                (___res).t = (___b).t;                                  \
            } else {                                                    \
                logger_error(ctx->logger, "invalid type of operand 2 [%s].", mix_get_type_name((___b).t->value)); \
                exit(-1);                                               \
            }                                                           \
        } else {                                                        \
            logger_error(ctx->logger, "invalid type of operand 1 [%s].", mix_get_type_name((___a).t->value)); \
            exit(-1);                                                   \
        }                                                               \
    } while (0)

static void bison_init_tov_for_atomic_value(struct mix_context* ctx, const char* type_name, int64_t v,
                                            struct mix_type_or_value* tov) {
    struct mix_type* type = bison_lookup_type(ctx, type_name, strlen(type_name));
    if (!type) {
        logger_error(ctx->logger, "cannot find data type [%s].", type_name);
        exit(-1);
    }

    tov->type = MIX_TOV_ATOMIC_VALUE;
    mix_type_acquire(type);
    tov->t = type;
    tov->l = v;
}

/*
  XXX this should be done in lexer.
  bison may copy mix_token_info in the union struct, but the struct qbuf can not be copied directly.
*/
static void bison_escape_string(const struct qbuf_ref* src, struct qbuf* dst) {
    const char* cursor = src->base;
    const char* end = (const char*)src->base + src->size;

    qbuf_init(dst);
    while (1) {
        if (cursor >= end) {
            return;
        }

        if (*cursor == '\\') {
            ++cursor; /* skip '\\' */
            switch (*cursor) {
                case 'a': qbuf_append_c(dst, '\a'); break;
                case 'b': qbuf_append_c(dst, '\b'); break;
                case 'e': qbuf_append_c(dst, '\e'); break;
                case 'f': qbuf_append_c(dst, '\f'); break;
                case 'n': qbuf_append_c(dst, '\n'); break;
                case 'r': qbuf_append_c(dst, '\r'); break;
                case 't': qbuf_append_c(dst, '\t'); break;
                case 'v': qbuf_append_c(dst, '\v'); break;
                case '\\': qbuf_append_c(dst, '\\'); break;
                case '\'': qbuf_append_c(dst, '\''); break;
                case '?': qbuf_append_c(dst, '\?'); break;
                default: /* unknown escaped char, saved for further use */
                    qbuf_append_c(dst, '\\');
                    qbuf_append_c(dst, *cursor);
                    break;
            }
            ++cursor;
            continue;
        }

        qbuf_append_c(dst, *cursor);
        ++cursor;
    }
}

static void bison_init_tov_for_str(struct mix_context* ctx, struct qbuf_ref* str,
                                   struct mix_type_or_value* tov) {
    struct mix_shared_value* v = mix_shared_value_create();
    if (!v) {
        logger_error(ctx->logger, "malloc mix_shared_value failed.");
        exit(-1);
    }

    v->type = bison_lookup_type(ctx, "str", 3);
    mix_type_acquire(v->type);
    bison_escape_string(str, &v->s);

    tov->type = MIX_TOV_SHARED_VALUE;
    mix_shared_value_acquire(v);
    tov->v = v;
}
%}

%union {
    struct vector* value_list;
    struct qbuf* buf;
    union mix_token_info token;
    struct mix_identifier* var;
    struct mix_type* type;
    struct mix_type_or_value value;
}

// reentrant yylex()
%define api.pure full

// params of yylex()
%lex-param {struct mix_lex* lex} {const char* buf} {uint32_t buf_sz}

// debug settings
%define parse.lac full
%define parse.error detailed
%define parse.trace

// params of yyparse(), should include those passed to yylex()
%parse-param {struct mix_context* ctx} {struct mix_lex* lex} {const char* buf} {uint32_t buf_sz} {const char* prefix}

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
%type   <token> BISON_KEYWORD_i16
%type   <token> BISON_KEYWORD_i32
%type   <token> BISON_KEYWORD_i64
%type   <token> BISON_KEYWORD_i8
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

%type   <var> variable
%type   <var> identifier_and_type

%type   <type> type_specifier
%type   <type> builtin_type_specifier
%type   <type> user_type_specifier

%type   <value> constant
%type   <value> postfix_expression
%type   <value> unary_expression
%type   <value> multiplicative_expression
%type   <value> additive_expression
%type   <value> shift_expression
%type   <value> relational_expression
%type   <value> equality_expression
%type   <value> and_expression
%type   <value> exclusive_or_expression
%type   <value> inclusive_or_expression
%type   <value> logical_and_expression
%type   <value> logical_or_expression
%type   <value> conditional_expression
%type   <value> expression
%type   <value> lambda

%type   <token> multiplicative_operator
%type   <token> additive_operator
%type   <token> unary_operator
%type   <token> arithmetic_unary_operator
%type   <token> logical_unary_operator

%type   <buf> optional_generic_type_specifier
%type   <buf> generic_type_specifier
%type   <buf> nested_name_specifier
%type   <buf> nested_import_scope
%type   <buf> import_item

%type   <value_list> expression_list
%%

block : optional_statement_list
      ;

optional_statement_list : %empty
                        | optional_statement_list statement
                        ;

/* -------------------------------------------------------------------------- */

statement :
';'
| expression ';' {
    mix_type_or_value_destroy(&$1);
}
| assignment_statement
| variable_declaration_clause ';'
| selection_statement
| iteration_statement
| jump_statement
| import_statement
| export_statement
| compound_statement
| typedef_statement
| enum_declaration
| function_definition
| struct_declaration
| struct_impl_definition
| trait_definition
| trait_impl_definition
;

assignment_statement :
variable '=' expression ';' {
    mix_type_or_value_destroy(&$1->tov);
    mix_type_or_value_move_construct(&$3, &$1->tov);
}
| variable '=' braced_initializer ';' {
/* TODO */
}
| variable assignment_operator expression ';'
;

variable :
BISON_SYM_IDENTIFIER {
    $$ = mix_parser_lookup_identifier(ctx, &$1.s);
    if (!$$) {
        logger_error(ctx->logger, "var [%s] not found.", make_tmp_str_s(&$1.s));
        exit(-1);
    }
}
| postfix_expression '[' expression ']' { /* TODO */ }
| postfix_expression '.' BISON_SYM_IDENTIFIER { /* TODO */ }
;

assignment_operator : BISON_OP_ADD_ASSIGN | BISON_OP_SUB_ASSIGN | BISON_OP_MUL_ASSIGN | BISON_OP_DIV_ASSIGN | BISON_OP_MOD_ASSIGN | BISON_OP_AND_ASSIGN | BISON_OP_OR_ASSIGN | BISON_OP_XOR_ASSIGN | BISON_OP_RSHIFT_ASSIGN | BISON_OP_LSHIFT_ASSIGN
                    ;

variable_declaration_clause :
BISON_KEYWORD_var variable_declaration_with_optional_assignment_list { /* do nothing */ }
;

variable_declaration_with_optional_assignment_list
: variable_declaration_with_optional_assignment { /* do nothing */ }
| variable_declaration_with_optional_assignment_list ',' variable_declaration_with_optional_assignment { /* do nothing */ }
;

variable_declaration_with_optional_assignment :
identifier_and_type { /* do nothing */ }
| identifier_and_type '=' braced_initializer {
/* TODO */
}
| identifier_and_type '=' expression {
/* TODO */
}
| BISON_SYM_IDENTIFIER '=' expression {
    struct mix_identifier* var = mix_parser_new_identifier(ctx, &$1.s);
    if (!var) {
        logger_error(ctx->logger, "add variable [%s] failed.", make_tmp_str_s(&$1.s));
        exit(-1);
    }

    mix_type_or_value_move_construct(&$3, &var->tov);
}
;

identifier_and_type :
BISON_SYM_IDENTIFIER ':' type_specifier {
    struct mix_identifier* var = mix_parser_new_identifier(ctx, &$1.s);
    if (!var) {
        logger_error(ctx->logger, "add variable [%s] failed.", make_tmp_str_s(&$1.s));
        exit(-1);
    }

    if (MIX_TYPE_IS_ATOMIC($3->value)) {
        var->tov.type = MIX_TOV_ATOMIC_VALUE;
        var->tov.t = $3; /* no need to acquire */
    } else {
        struct mix_shared_value* v = mix_shared_value_create();
        if (!v) {
            logger_error(ctx->logger, "malloc new value failed.");
            exit(-1);
        }
        var->tov.type = MIX_TOV_SHARED_VALUE;
        mix_shared_value_acquire(v);
        var->tov.v = v;
        v->type = $3; /* no need to acquire */
    }

    $$ = var;
}
;

braced_initializer : '{' '}'
                   | '{' initializer_clause_list '}'
                   | '{' initializer_clause_list ',' '}'
                   | '{' expression_list '}'
                   | '{' expression_list ',' '}'
                   ;

initializer_clause_list : initializer_clause
                        | initializer_clause_list ',' initializer_clause
                        ;

initializer_clause : BISON_SYM_IDENTIFIER '=' expression
                   | BISON_SYM_IDENTIFIER '=' braced_initializer
                   | braced_initializer
                   ;

selection_statement : BISON_KEYWORD_if expression compound_statement
                    | BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else selection_statement
                    | BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else compound_statement
                    ;

iteration_statement : BISON_KEYWORD_while expression compound_statement
                    | BISON_KEYWORD_do compound_statement BISON_KEYWORD_while expression ';'
                    | BISON_KEYWORD_for BISON_SYM_IDENTIFIER BISON_KEYWORD_in expression compound_statement
                    ;

compound_statement : '{' optional_statement_list '}'
                   ;

jump_statement : BISON_KEYWORD_continue ';'
               | BISON_KEYWORD_break ';'
               | BISON_KEYWORD_return ';'
               | BISON_KEYWORD_return expression ';'
               ;

typedef_statement : BISON_KEYWORD_typedef type_specifier BISON_SYM_IDENTIFIER ';'
                  ;

/* -------------------------------------------------------------------------- */

export_statement : BISON_KEYWORD_export identifier_list ';'
                 ;

identifier_list : BISON_SYM_IDENTIFIER
                | identifier_list ',' BISON_SYM_IDENTIFIER
                ;

import_statement :
BISON_KEYWORD_import import_item_list ';' { /* do nothing */ }
| BISON_KEYWORD_import import_item BISON_KEYWORD_as BISON_SYM_IDENTIFIER ';' {
    struct qbuf prefix;
    qbuf_init(&prefix);
    qbuf_assign(&prefix, $4.s.base, $4.s.size);
    qbuf_append_c(&prefix, '/');
    mix_parser_load_lib(ctx, qbuf_get_ref($2), qbuf_get_ref(&prefix));
    qbuf_destroy($2);
    free($2);
}
;

import_item_list :
import_item {
    mix_parser_load_lib(ctx, qbuf_get_ref($1), NULL);
    qbuf_destroy($1);
    free($1);
}
| import_item_list ',' import_item {
    mix_parser_load_lib(ctx, qbuf_get_ref($3), NULL);
    qbuf_destroy($3);
    free($3);
}
;

import_item :
BISON_SYM_IDENTIFIER {
    $$ = malloc(sizeof(struct qbuf));
    qbuf_init($$);
    qbuf_assign($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
}
| nested_import_scope BISON_SYM_IDENTIFIER {
    qbuf_append($1, $2.s.base, $2.s.size);
    qbuf_append_c($1, '/');
    $$ = $1;
}
;

nested_import_scope :
BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    qbuf_init($$);
    qbuf_append($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
}
| nested_import_scope BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    qbuf_move_construct($1, $$);
    qbuf_append($$, $2.s.base, $2.s.size);
    qbuf_append_c($$, '/');
    free($1);
}
;

/* -------------------------------------------------------------------------- */

expression :
conditional_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
;

conditional_expression :
logical_or_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| logical_or_expression '?' conditional_expression ':' conditional_expression
;

logical_or_expression :
logical_and_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| logical_or_expression BISON_OP_LOGICAL_OR logical_and_expression
;

logical_and_expression :
inclusive_or_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| logical_and_expression BISON_OP_LOGICAL_AND inclusive_or_expression
;

inclusive_or_expression :
exclusive_or_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| inclusive_or_expression '|' exclusive_or_expression
;

exclusive_or_expression :
and_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| exclusive_or_expression '^' and_expression
;

and_expression :
equality_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| and_expression '&' equality_expression
;

equality_expression :
relational_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| equality_expression equality_operator relational_expression
;

equality_operator : BISON_OP_EQUAL | BISON_OP_NOT_EQUAL
                  ;

relational_expression :
shift_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| relational_expression relational_operator shift_expression
;

relational_operator : '<' | '>' | BISON_OP_LESS_EQUAL | BISON_OP_GREATER_EQUAL
                    ;

shift_expression :
additive_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| shift_expression shift_operator additive_expression
;

shift_operator : BISON_OP_LSHIFT | BISON_OP_RSHIFT
               ;

additive_expression :
multiplicative_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| additive_expression additive_operator multiplicative_expression {
    if ($2.c == '+') {
        BISON_BIN_OP($$, $1, +, $3);
    } else {
        BISON_BIN_OP($$, $1, -, $3);
    }

    mix_type_or_value_destroy(&$1);
    mix_type_or_value_destroy(&$3);
}
;

additive_operator : '+' { $$.c = '+'; }
                  | '-' { $$.c = '-'; }
                  ;

multiplicative_expression :
unary_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| multiplicative_expression multiplicative_operator unary_expression {
    if ($2.c == '*') {
        BISON_BIN_OP($$, $1, *, $3);
    } else if ($2.c == '/') {
        BISON_BIN_OP($$, $1, /, $3);
    } else if ($2.c == '%') {
        if (!MIX_TYPE_IS_INT($1.t->value)) {
            logger_error(ctx->logger, "invalid type [%s] of operand 1.", mix_get_type_name($1.t->value));
            exit(-1);
        } else if (!MIX_TYPE_IS_INT($3.t->value)) {
            logger_error(ctx->logger, "invalid type [%s] of operand 2.", mix_get_type_name($3.t->value));
            exit(-1);
        } else {
            $$.l = $1.l % $3.l;
            mix_type_acquire($1.t);
            $$.t = $1.t;
        }
    }

    mix_type_or_value_destroy(&$1);
    mix_type_or_value_destroy(&$3);
}
;

multiplicative_operator : '*' { $$.c = '*'; }
                        | '/' { $$.c = '/'; }
                        | '%' { $$.c = '%'; }
                        ;

unary_expression :
postfix_expression {
    mix_type_or_value_move_construct(&$1, &$$);
}
| constant { mix_type_or_value_move_construct(&$1, &$$); }
| unary_operator unary_expression { /* TODO */ }
;

unary_operator : arithmetic_unary_operator
               | logical_unary_operator
               ;

arithmetic_unary_operator :
'+' { $$.c = '+'; }
| '-' { $$.c = '-'; }
| '~' { $$.c = '~'; }
;

logical_unary_operator :
'!' { $$.c = '!'; }
;

postfix_expression :
variable {
    mix_type_or_value_copy_construct(&$1->tov, &$$);
}
| '(' expression ')' {
    mix_type_or_value_copy_construct(&$2, &$$);
}
| lambda {
    mix_type_or_value_copy_construct(&$1, &$$);
}
| BISON_SYM_IDENTIFIER generic_type_specifier { /* TODO handle generics */ }
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier {
    struct qbuf var_name;
    qbuf_move_construct($1, &var_name);
    qbuf_append(&var_name, $2.s.base, $2.s.size);
    struct mix_identifier* var = mix_parser_lookup_identifier(ctx, qbuf_get_ref(&var_name));
    if (!var) {
        logger_fatal(ctx->logger, "var [%s] not found.", make_tmp_str_s(qbuf_get_ref(&var_name)));
        exit(-1);
    }
    mix_type_or_value_copy_construct(&var->tov, &$$);
    qbuf_destroy(&var_name);
    free($1);
}
| postfix_expression '(' expression_list ')' {
    struct mix_type* type = mix_type_or_value_get_type(&$1);

    mix_parser_push_tov(ctx, &$1); /* function */
    /* push args */
    unsigned int vec_sz = vector_size($3);
    for (unsigned int i = 0; i < vec_sz; ++i) {
        struct mix_type_or_value* tov = vector_at($3, i);
        mix_parser_push_tov(ctx, tov);
    }

    mix_call(ctx, vec_sz);

    mix_type_or_value_destroy(&$1); /* destroy function */
    /* destroy args */
    for (unsigned int i = 0; i < vec_sz; ++i) {
        struct mix_type_or_value* tov = vector_at($3, i);
        mix_type_or_value_destroy(tov);
    }
    vector_destroy($3, NULL, NULL);
    free($3);

    struct mix_func_type* ftype = container_of(type, struct mix_func_type, t);
    if (ftype->ret_type) {
        struct mix_type_or_value* res = __get_top(ctx);
        mix_type_or_value_move_construct(res, &$$);
    } else {
        $$.type = MIX_TOV_UNKNOWN; /* without return value */
    }
}
| postfix_expression '(' braced_initializer ')' { /* TODO */ }
| postfix_expression '(' ')' { /* TODO */ }
| BISON_KEYWORD_cast BISON_SYM_GENERICS_LEFT_MARK type_specifier BISON_SYM_GENERICS_RIGHT_MARK '(' expression ')' { /* TODO */ }
;

lambda : function_type compound_statement { /* TODO */ }
       ;

constant :
BISON_INTEGER {
    /* TODO i8/i16/i64 */
    bison_init_tov_for_atomic_value(ctx, "i32", $1.l, &$$);
}
| BISON_FLOAT {
    /* TODO f64 */
    bison_init_tov_for_atomic_value(ctx, "f32", $1.l, &$$);
}
| BISON_LITERAL_STRING {
    bison_init_tov_for_str(ctx, &$1.s, &$$);
}
;

expression_list :
expression {
    $$ = malloc(sizeof(struct vector));
    vector_init($$, sizeof(struct mix_type_or_value));
    vector_resize($$, 1, NULL, NULL);
    mix_type_or_value_move_construct(&$1, vector_back($$));
}
| expression_list ',' expression {
    vector_resize($1, vector_size($1) + 1, NULL, NULL);
    mix_type_or_value_move_construct(&$3, vector_back($1));
    $$ = $1;
}
;

/* -------------------------------------------------------------------------- */

enum_declaration : BISON_KEYWORD_enum '{' optional_enum_member_list '}'
                 ;

optional_enum_member_list : %empty
                          | enum_member_list
                          | enum_member_list ','
                          ;

enum_member_list : BISON_SYM_IDENTIFIER
                 | BISON_SYM_IDENTIFIER '=' BISON_INTEGER
                 | enum_member_list ',' BISON_SYM_IDENTIFIER
                 | enum_member_list ',' BISON_SYM_IDENTIFIER '=' BISON_INTEGER
                 ;

/* -------------------------------------------------------------------------- */

function_definition : function_declaration compound_statement
                    ;

function_declaration : no_returned_value_function_declaration
                     | no_returned_value_function_declaration BISON_SYM_RIGHT_ARROW type_specifier
                     ;

no_returned_value_function_declaration : BISON_KEYWORD_func BISON_SYM_IDENTIFIER optional_generic_type_specifier '(' function_declaration_parameter_list ')'
                                       ;

function_declaration_parameter_list : %empty
                                    | BISON_SYM_VARIADIC_ARG
                                    | parameter_and_type_list
                                    | parameter_and_type_list ',' BISON_SYM_VARIADIC_ARG
                                    ;

parameter_and_type_list : parameter_and_type
                        | parameter_and_type_list ',' parameter_and_type
                        ;

parameter_and_type : type_specifier
                   | identifier_and_type
                   ;

function_type : no_returned_value_function_type
              | no_returned_value_function_type BISON_SYM_RIGHT_ARROW type_specifier
              ;

no_returned_value_function_type : BISON_KEYWORD_func optional_generic_type_specifier '(' function_declaration_parameter_list ')'
                                ;

/* -------------------------------------------------------------------------- */

struct_declaration : BISON_KEYWORD_struct BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_struct_member_list '}'
                   ;

optional_generic_type_specifier :
%empty {
    $$ = NULL;
}
| generic_type_specifier {
    logger_fatal(ctx->logger, "generics not implemented.");
    exit(-1);
}
;

generic_type_specifier :
BISON_SYM_GENERICS_LEFT_MARK type_specifier_list BISON_SYM_GENERICS_RIGHT_MARK {
    logger_fatal(ctx->logger, "generics not implemented.");
    exit(-1);
}
;

type_specifier_list : type_specifier
                    | type_specifier_list ',' type_specifier
                    ;

optional_struct_member_list : %empty
                            | optional_struct_member_list struct_member
                            ;

struct_member : variable_declaration_clause ';'
              ;

struct_impl_definition : BISON_KEYWORD_impl BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_member_function_list '}'
                       ;

optional_member_function_list : %empty
                              | optional_member_function_list member_function_definition
                              | optional_member_function_list member_function_declaration ';'
                              ;

member_function_definition : member_function_declaration compound_statement
                           ;

member_function_declaration : no_returned_value_member_function_declaration
                            | no_returned_value_member_function_declaration BISON_SYM_RIGHT_ARROW member_function_return_type
                            ;

member_function_return_type : type_specifier
                            | BISON_KEYWORD_self
                            ;

no_returned_value_member_function_declaration : BISON_KEYWORD_func BISON_SYM_IDENTIFIER optional_generic_type_specifier '(' member_function_declaration_parameter_list ')'
                                              ;

member_function_declaration_parameter_list : member_function_first_self_param
                                           | member_function_first_self_param ',' BISON_SYM_VARIADIC_ARG
                                           | member_function_first_self_param ',' parameter_and_type_list
                                           | member_function_first_self_param ',' parameter_and_type_list ',' BISON_SYM_VARIADIC_ARG
                                           | function_declaration_parameter_list
                                           ;

member_function_first_self_param : BISON_KEYWORD_self
                                 | BISON_SYM_IDENTIFIER ':' BISON_KEYWORD_self
                                 ;

/* -------------------------------------------------------------------------- */

trait_definition : BISON_KEYWORD_trait BISON_SYM_IDENTIFIER optional_generic_type_specifier optional_constraint_trait_specifier '{' optional_trait_member_list '}'
                 ;

optional_constraint_trait_specifier : %empty
                                    | ':' constraint_trait_list
                                    ;

constraint_trait_list : user_type_specifier
                      | constraint_trait_list ',' user_type_specifier
                      ;

optional_trait_member_list : %empty
                           | optional_trait_member_list trait_member
                           ;

trait_member : member_function_declaration ';'
             | member_function_definition
             ;

trait_impl_definition : BISON_KEYWORD_impl BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_KEYWORD_for BISON_SYM_IDENTIFIER optional_generic_type_specifier '{' optional_member_function_list '}'
                      ;

/* -------------------------------------------------------------------------- */

type_specifier : builtin_type_specifier { $$ = $1; }
               | user_type_specifier { $$ = $1; }
               ;

builtin_type_specifier :
BISON_KEYWORD_f32 {
    struct qbuf_ref tname = { .base = "f32", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_f64 {
    struct qbuf_ref tname = { .base = "f64", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_i8 {
    struct qbuf_ref tname = { .base = "i8", .size = 2 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_i16 {
    struct qbuf_ref tname = { .base = "i16", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_i32 {
    struct qbuf_ref tname = { .base = "i32", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_i64 {
    struct qbuf_ref tname = { .base = "i64", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
| BISON_KEYWORD_str {
    struct qbuf_ref tname = { .base = "str", .size = 3 };
    $$ = mix_parser_lookup_type(ctx, &tname);
    mix_type_acquire($$);
}
;

user_type_specifier : BISON_SYM_IDENTIFIER optional_generic_type_specifier { /* TODO */ }
                    | nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier { /* TODO */ }
                    | function_type { /* TODO */ }
                    ;

nested_name_specifier :
BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    qbuf_init($$);
    qbuf_assign($$, $1.s.base, $1.s.size);
    qbuf_append_c($$, '/');
}
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER {
    $$ = malloc(sizeof(struct qbuf));
    qbuf_move_construct($1, $$);
    qbuf_append($$, $2.s.base, $2.s.size);
    qbuf_append_c($$, '/');
    free($1);
}
;

/* -------------------------------------------------------------------------- */

/*
identifier : character_of_identifier character_or_digit_sequence
           ;

character_or_digit_sequence : %empty
                            | character_or_digit_sequence character_of_identifier
                            | character_or_digit_sequence digit
                            ;

character_of_identifier : '_'
                        | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j' | 'k' | 'l' | 'm'
                        | 'n' | 'o' | 'p' | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'
                        | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I' | 'J' | 'K' | 'L' | 'M'
                        | 'N' | 'O' | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z'
                        ;
*/

/* -------------------------------------------------------------------------- */

/*
integer : digit_sequence
        | digit_sequence exponent_specifier positive_plain_integer
        ;

positive_plain_integer : digit_sequence
                       | '+' digit_sequence
                       ;

negative_plain_integer : '-' digit_sequence
                       ;

floating_point : fractional
               | fractional exponent_specifier digit_sequence
               | digit_sequence exponent_specifier negative_plain_integer
               ;

fractional : digit_sequence '.' digit_sequence
           ;

exponent_specifier : 'e' | 'E'
                   ;

digit_sequence : digit
               | digit_sequence digit
               ;

digit : '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
      ;
*/

%%
