#ifndef __MIX_LEX_H__
#define __MIX_LEX_H__

#include "typedef.h"
#include "retcode.h"
#include "cutils/qbuf.h"
#include "cutils/qbuf_ref.h"
#include "cutils/robin_hood_hash.h"

enum {
    MIX_TT_INVALID = 0,

    MIX_TT_CHAR,
    MIX_TT_LITERAL_STRING,
    MIX_TT_INTEGER,
    MIX_TT_FLOAT,

    MIX_TT_OP_LSHIFT,               /* << */
    MIX_TT_OP_RSHIFT,               /* >> */
    MIX_TT_OP_LSHIFT_ASSIGN,        /* <<= */
    MIX_TT_OP_RSHIFT_ASSIGN,        /* >>= */
    MIX_TT_OP_ADD_ASSIGN,           /* += */
    MIX_TT_OP_SUB_ASSIGN,           /* -= */
    MIX_TT_OP_MUL_ASSIGN,           /* *= */
    MIX_TT_OP_DIV_ASSIGN,           /* /= */
    MIX_TT_OP_MOD_ASSIGN,           /* %= */
    MIX_TT_OP_AND_ASSIGN,           /* &= */
    MIX_TT_OP_OR_ASSIGN,            /* |= */
    MIX_TT_OP_XOR_ASSIGN,           /* ^= */
    MIX_TT_OP_LOGICAL_OR,           /* || */
    MIX_TT_OP_LOGICAL_AND,          /* && */
    MIX_TT_OP_EQUAL,                /* == */
    MIX_TT_OP_NOT_EQUAL,            /* != */
    MIX_TT_OP_GREATER_EQUAL,        /* >= */
    MIX_TT_OP_LESS_EQUAL,           /* <= */

    MIX_TT_SYM_IDENTIFIER,          /* */
    MIX_TT_SYM_SCOPE_SPECIFIER,     /* :: */
    MIX_TT_SYM_RIGHT_ARROW,         /* -> */
    MIX_TT_SYM_GENERICS_LEFT_MARK,  /* <| */
    MIX_TT_SYM_GENERICS_RIGHT_MARK, /* |> */
    MIX_TT_SYM_VARIADIC_ARG,        /* ... */

    MIX_TT_KEYWORD_alias,
    MIX_TT_KEYWORD_as,
    MIX_TT_KEYWORD_async, /* reserved */
    MIX_TT_KEYWORD_await, /* reserved */
    MIX_TT_KEYWORD_break,
    MIX_TT_KEYWORD_cast,
    MIX_TT_KEYWORD_continue,
    MIX_TT_KEYWORD_do,
    MIX_TT_KEYWORD_else,
    MIX_TT_KEYWORD_enum,
    MIX_TT_KEYWORD_export,
    MIX_TT_KEYWORD_extern, /* reserved */
    MIX_TT_KEYWORD_f32,
    MIX_TT_KEYWORD_f64,
    MIX_TT_KEYWORD_final, /* reserved */
    MIX_TT_KEYWORD_for,
    MIX_TT_KEYWORD_func,
    MIX_TT_KEYWORD_i16,
    MIX_TT_KEYWORD_i32,
    MIX_TT_KEYWORD_i64,
    MIX_TT_KEYWORD_i8,
    MIX_TT_KEYWORD_if,
    MIX_TT_KEYWORD_impl,
    MIX_TT_KEYWORD_import,
    MIX_TT_KEYWORD_in,
    MIX_TT_KEYWORD_let, /* reserved */
    MIX_TT_KEYWORD_marco, /* reserved */
    MIX_TT_KEYWORD_override, /* reserved */
    MIX_TT_KEYWORD_return,
    MIX_TT_KEYWORD_self,
    MIX_TT_KEYWORD_str,
    MIX_TT_KEYWORD_struct,
    MIX_TT_KEYWORD_trait,
    MIX_TT_KEYWORD_typeof, /* reserved */
    MIX_TT_KEYWORD_var,
    MIX_TT_KEYWORD_virtual, /* reserved */
    MIX_TT_KEYWORD_while,
    MIX_TT_KEYWORD_yield, /* reserved */
};

typedef uint32_t mix_token_type_t;

struct mix_lex {
    uint32_t linenum;
    uint32_t lineoff;
    const char* cursor;
    const char* buf_end;
    struct robin_hood_hash keyword_hash; /* TODO optimize: parse directly */
};

union mix_token_info {
    char c;
    int64_t l;
    double d;
    struct qbuf_ref s;
    struct qbuf str; /* for MIX_TT_LITERAL_STRING only */
};

mix_retcode_t mix_lex_init(struct mix_lex*, const char* buf, uint32_t len);
mix_token_type_t mix_lex_get_next_token(struct mix_lex*, union mix_token_info*);
void mix_lex_destroy(struct mix_lex*);

#endif
