%{
#include "mix/lex.h"
#include <stdio.h>
#include <stdlib.h> // exit()

#define YYSTYPE union mix_token_info
#include "mix.tab.h"

static struct mix_lex g_lex;

/* mix token type => bison token type */
static const int g_m2b_type[] = {
    YYUNDEF,
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
    BISON_KEYWORD_const,
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
    BISON_KEYWORD_macro,
    BISON_KEYWORD_override,
    BISON_KEYWORD_return,
    BISON_KEYWORD_self,
    BISON_KEYWORD_struct,
    BISON_KEYWORD_trait,
    BISON_KEYWORD_typeof,
    BISON_KEYWORD_var,
    BISON_KEYWORD_virtual,
    BISON_KEYWORD_while,
    BISON_KEYWORD_yield,
};

static int yylex() {
    mix_token_type_t type = mix_lex_get_next_token(&g_lex, &yylval);
    if (type == MIX_TT_CHAR) {
        if (yylval.c == EOF) {
            return YYEOF;
        }
        return yylval.c;
    }
    return g_m2b_type[type];
}

static void yyerror(const char *msg) {
    printf("error: %s\n", msg);
    exit(-1);
}
%}

%define parse.lac full
%define parse.error detailed
%define parse.trace

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
%token BISON_KEYWORD_const
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
%token BISON_KEYWORD_macro
%token BISON_KEYWORD_override
%token BISON_KEYWORD_return
%token BISON_KEYWORD_self
%token BISON_KEYWORD_struct
%token BISON_KEYWORD_trait
%token BISON_KEYWORD_typeof
%token BISON_KEYWORD_var
%token BISON_KEYWORD_virtual
%token BISON_KEYWORD_while
%token BISON_KEYWORD_yield

%%

block : optional_statement_list
      ;

optional_statement_list : %empty
                        | optional_statement_list statement
                        ;

/* -------------------------------------------------------------------------- */

statement : ';'
          | expression ';'
          | assignment_statement
          | variable_declaration_clause ';'
          | selection_statement
          | iteration_statement
          | jump_statement
          | import_statement
          | export_statement
          | compound_statement
          | enum_declaration
          | function_definition
          | struct_declaration
          | struct_impl_definition
          | trait_definition
          | trait_impl_definition
          ;

assignment_statement : variable '=' initializer ';'
                     | variable assignment_operator expression ';'
                     ;

variable : BISON_SYM_IDENTIFIER
         | postfix_expression '[' expression ']'
         | postfix_expression '.' BISON_SYM_IDENTIFIER
         ;

assignment_operator : BISON_OP_ADD_ASSIGN | BISON_OP_SUB_ASSIGN | BISON_OP_MUL_ASSIGN | BISON_OP_DIV_ASSIGN | BISON_OP_MOD_ASSIGN | BISON_OP_AND_ASSIGN | BISON_OP_OR_ASSIGN | BISON_OP_XOR_ASSIGN | BISON_OP_RSHIFT_ASSIGN | BISON_OP_LSHIFT_ASSIGN
                    ;

variable_declaration_clause : BISON_KEYWORD_var variable_declaration_with_optional_assignment_list
                            | BISON_KEYWORD_const variable_declaration_with_optional_assignment_list
                            ;

variable_declaration_with_optional_assignment_list : variable_declaration_with_optional_assignment
                                                   | variable_declaration_with_optional_assignment_list ',' variable_declaration_with_optional_assignment
                                                   ;

variable_declaration_with_optional_assignment : identifier_and_type
                                              | identifier_and_type '=' initializer
                                              | BISON_SYM_IDENTIFIER '=' initializer
                                              ;

identifier_and_type : BISON_SYM_IDENTIFIER ':' type_specifier
                    ;

initializer : expression
            | braced_initializer
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

initializer_clause : BISON_SYM_IDENTIFIER '=' initializer
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

/* -------------------------------------------------------------------------- */

export_statement : BISON_KEYWORD_export identifier_list ';'
                 ;

identifier_list : BISON_SYM_IDENTIFIER
                | identifier_list ',' BISON_SYM_IDENTIFIER
                ;

import_statement : BISON_KEYWORD_import identifier_list ';'
                 | BISON_KEYWORD_import import_source BISON_KEYWORD_as BISON_SYM_IDENTIFIER ';'
                 ;

import_source : BISON_SYM_IDENTIFIER
              | BISON_LITERAL_STRING
              ;

/* -------------------------------------------------------------------------- */

expression : conditional_expression
           ;

conditional_expression : logical_or_expression
                       | logical_or_expression '?' conditional_expression ':' conditional_expression
                       ;

logical_or_expression : logical_and_expression
                      | logical_or_expression BISON_OP_LOGICAL_OR logical_and_expression
                      ;

logical_and_expression : inclusive_or_expression
                       | logical_and_expression BISON_OP_LOGICAL_AND inclusive_or_expression
                       ;

inclusive_or_expression : exclusive_or_expression
                        | inclusive_or_expression '|' exclusive_or_expression
                        ;

exclusive_or_expression : and_expression
                        | exclusive_or_expression '^' and_expression
                        ;

and_expression : equality_expression
               | and_expression '&' equality_expression
               ;

equality_expression : relational_expression
                    | equality_expression equality_operator relational_expression
                    ;

equality_operator : BISON_OP_EQUAL | BISON_OP_NOT_EQUAL
                  ;

relational_expression : shift_expression
                      | relational_expression relational_operator shift_expression
                      ;

relational_operator : '<' | '>' | BISON_OP_LESS_EQUAL | BISON_OP_GREATER_EQUAL
                    ;

shift_expression : additive_expression
                 | shift_expression shift_operator additive_expression
                 ;

shift_operator : BISON_OP_LSHIFT | BISON_OP_RSHIFT
               ;

additive_expression : multiplicative_expression
                    | additive_expression additive_operator multiplicative_expression
                    ;

additive_operator : '+' | '-'
                  ;

multiplicative_expression : unary_expression
                          | multiplicative_expression multiplicative_operator unary_expression
                          ;

multiplicative_operator : '*' | '/' | '%'
                        ;

unary_expression : postfix_expression
                 | constant
                 | unary_operator unary_expression
                 ;

unary_operator : arithmetic_unary_operator
               | logical_unary_operator
               ;

arithmetic_unary_operator : '+' | '-' | '~'
                          ;

logical_unary_operator : '!'
                       ;

postfix_expression : variable
                   | '(' expression ')'
                   | lambda
                   | BISON_SYM_IDENTIFIER generic_type_specifier
                   | nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier
                   | postfix_expression '(' expression_list ')'
                   | postfix_expression '(' braced_initializer ')'
                   | postfix_expression '(' ')'
                   | BISON_KEYWORD_cast '<' type_specifier '>' '(' expression ')'
                   ;

lambda : function_type compound_statement
       ;

constant : BISON_INTEGER
         | BISON_FLOAT
         | BISON_LITERAL_STRING
         ;

expression_list : expression
                | expression_list ',' expression
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

optional_generic_type_specifier : %empty
                                | generic_type_specifier
                                ;

generic_type_specifier : BISON_SYM_GENERICS_LEFT_MARK type_specifier_list BISON_SYM_GENERICS_RIGHT_MARK
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

constraint_trait_list : non_function_user_type_specifier
                      | constraint_trait_list ',' non_function_user_type_specifier
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

atomic_type_specifier : BISON_KEYWORD_f32 | BISON_KEYWORD_f64 | BISON_KEYWORD_i8 | BISON_KEYWORD_i16 | BISON_KEYWORD_i32 | BISON_KEYWORD_i64
                      ;

non_function_user_type_specifier : BISON_SYM_IDENTIFIER optional_generic_type_specifier
                                 | nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier
                                 ;

user_type_specifier : non_function_user_type_specifier
                    | function_type
                    ;

type_specifier : atomic_type_specifier
               | user_type_specifier
               ;

nested_name_specifier : BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER
                      | nested_name_specifier BISON_SYM_IDENTIFIER optional_generic_type_specifier BISON_SYM_SCOPE_SPECIFIER
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

#include "logger/stdio_logger.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return -1;
    }

    struct logger logger;
    stdio_logger_init(&logger);
    mix_lex_init(&g_lex, argv[1], &logger);
    yyparse();
    mix_lex_destroy(&g_lex);
    return 0;
}
