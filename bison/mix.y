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
%}

%union {
    char c;
    struct qbuf* buf;
    struct mix_type* type;
    union mix_token_info token;
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
%%

block
: optional_statement_list
;

optional_statement_list
: %empty
| optional_statement_list statement
;

/* -------------------------------------------------------------------------- */

statement
: ';'
| expression ';'
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
| struct_definition
| struct_impl_definition
| trait_definition
| trait_impl_definition
;

assignment_statement
: variable '=' expression ';'
| variable '=' braced_initializer ';'
| variable assignment_operator expression ';'
;

variable
: BISON_SYM_IDENTIFIER
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
: BISON_KEYWORD_var variable_declaration_with_optional_assignment_list
;

variable_declaration_with_optional_assignment_list
: variable_declaration_with_optional_assignment
| variable_declaration_with_optional_assignment_list ',' variable_declaration_with_optional_assignment
;

variable_declaration_with_optional_assignment
: BISON_SYM_IDENTIFIER ':' type_specifier
| BISON_SYM_IDENTIFIER ':' type_specifier '=' braced_initializer
| BISON_SYM_IDENTIFIER ':' type_specifier '=' expression
| BISON_SYM_IDENTIFIER '=' expression
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
: BISON_KEYWORD_if expression compound_statement
| BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else selection_statement
| BISON_KEYWORD_if expression compound_statement BISON_KEYWORD_else compound_statement
;

iteration_statement
: BISON_KEYWORD_while expression compound_statement
| BISON_KEYWORD_do compound_statement BISON_KEYWORD_while expression ';'
| BISON_KEYWORD_for BISON_SYM_IDENTIFIER BISON_KEYWORD_in expression compound_statement
;

compound_statement
: '{' optional_statement_list '}'
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
: BISON_KEYWORD_import import_item_list ';'
| BISON_KEYWORD_import import_item BISON_KEYWORD_as BISON_SYM_IDENTIFIER ';'
;

import_item_list
: import_item
| import_item_list ',' import_item
;

import_item
: BISON_SYM_IDENTIFIER
| nested_import_scope BISON_SYM_IDENTIFIER
;

nested_import_scope
: BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER
| nested_import_scope BISON_SYM_IDENTIFIER BISON_SYM_SCOPE_SPECIFIER
;

/* -------------------------------------------------------------------------- */

expression
: conditional_expression
;

conditional_expression
: logical_or_expression
| logical_or_expression '?' conditional_expression ':' conditional_expression
;

logical_or_expression
: logical_and_expression
| logical_or_expression BISON_OP_LOGICAL_OR logical_and_expression
;

logical_and_expression
: inclusive_or_expression
| logical_and_expression BISON_OP_LOGICAL_AND inclusive_or_expression
;

inclusive_or_expression
: exclusive_or_expression
| inclusive_or_expression '|' exclusive_or_expression
;

exclusive_or_expression
: and_expression
| exclusive_or_expression '^' and_expression
;

and_expression
: equality_expression
| and_expression '&' equality_expression
;

equality_expression
: relational_expression
| equality_expression equality_operator relational_expression
;

equality_operator
: BISON_OP_EQUAL | BISON_OP_NOT_EQUAL
;

relational_expression
: shift_expression
| relational_expression relational_operator shift_expression
;

relational_operator
: '<' | '>' | BISON_OP_LESS_EQUAL | BISON_OP_GREATER_EQUAL
;

shift_expression
: additive_expression
| shift_expression shift_operator additive_expression
;

shift_operator
: BISON_OP_LSHIFT | BISON_OP_RSHIFT
;

additive_expression
: multiplicative_expression
| additive_expression additive_operator multiplicative_expression
;

additive_operator
: '+'
| '-'
;

multiplicative_expression
: unary_expression
| multiplicative_expression multiplicative_operator unary_expression
;

multiplicative_operator
: '*'
| '/'
| '%'
;

unary_expression
: postfix_expression
| constant
| unary_operator unary_expression
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
: variable
| '(' expression ')'
| lambda
| BISON_SYM_IDENTIFIER generic_type_specifier
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier
| postfix_expression '(' expression_or_braced_initializer_list ')'
| postfix_expression '(' ')'
| BISON_KEYWORD_cast BISON_SYM_GENERICS_LEFT_MARK type_specifier BISON_SYM_GENERICS_RIGHT_MARK '(' expression ')'
;

lambda
: function_type compound_statement
;

expression_or_braced_initializer_list
: expression
| braced_initializer
| expression_or_braced_initializer_list ',' expression
| expression_or_braced_initializer_list ',' braced_initializer
;

constant
: BISON_INTEGER
| BISON_FLOAT
| BISON_LITERAL_STRING
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
: BISON_SYM_GENERICS_LEFT_MARK type_specifier_list BISON_SYM_GENERICS_RIGHT_MARK
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
: builtin_type_specifier
| user_type_specifier
;

builtin_type_specifier
: BISON_KEYWORD_f32
| BISON_KEYWORD_f64
| BISON_KEYWORD_i8
| BISON_KEYWORD_i16
| BISON_KEYWORD_i32
| BISON_KEYWORD_i64
| BISON_KEYWORD_str
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
: BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER
| nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER
;
%%
