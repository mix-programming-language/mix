#ifndef __MIX_LEX_H__
#define __MIX_LEX_H__

#include "retcode.h"
#include "token_type.h"
#include "cutils/qbuf_ref.h"
#include "cutils/robin_hood_hash.h"
#include "logger/logger.h"

struct mix_lex {
    unsigned int linenum;
    unsigned int lineoff;
    const char* cursor;
    const char* buf_end;
    char* buf_begin;
    struct robin_hood_hash keyword_hash;
    struct logger* logger;
};

union mix_token_info {
    char c;
    long l;
    double d;
    struct qbuf_ref s;
};

mix_retcode_t mix_lex_init(struct mix_lex*, const char* fpath, struct logger*);
mix_token_type_t mix_lex_get_next_token(struct mix_lex*, union mix_token_info*);
void mix_lex_destroy(struct mix_lex*);

#endif
