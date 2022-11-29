#include "lex.h"
#include "cutils/str_utils.h"
#include <math.h> // pow()
#include <stdio.h> // FILE and EOF
#include <stdlib.h> /* malloc() */
#include <ctype.h> /* isspace() */

#include "xxhash.h"

#ifndef NDEBUG
#include <string.h>
static inline const char* make_tmp_str(const char* base, unsigned int size) {
    static char buf[1024];
    memcpy(buf, base, size);
    buf[size] = '\0';
    return buf;
}
#endif

/* -------------------------------------------------------------------------- */

struct keyword_info {
    const char* word;
    unsigned int word_len;
    mix_token_type_t type;
};

static const struct keyword_info g_keyword[] = {
    {"as", 2, MIX_TT_KEYWORD_as},
    {"async", 5, MIX_TT_KEYWORD_async},
    {"await", 5, MIX_TT_KEYWORD_await},
    {"break", 5, MIX_TT_KEYWORD_break},
    {"cast", 4, MIX_TT_KEYWORD_cast},
    {"const", 5, MIX_TT_KEYWORD_const},
    {"continue", 8, MIX_TT_KEYWORD_continue},
    {"do", 2, MIX_TT_KEYWORD_do},
    {"else", 4, MIX_TT_KEYWORD_else},
    {"enum", 4, MIX_TT_KEYWORD_enum},
    {"export", 6, MIX_TT_KEYWORD_export},
    {"extern", 6, MIX_TT_KEYWORD_extern},
    {"f32", 3, MIX_TT_KEYWORD_f32},
    {"f64", 3, MIX_TT_KEYWORD_f64},
    {"final", 5, MIX_TT_KEYWORD_final},
    {"for", 3, MIX_TT_KEYWORD_for},
    {"func", 4, MIX_TT_KEYWORD_func},
    {"i16", 3, MIX_TT_KEYWORD_i16},
    {"i32", 3, MIX_TT_KEYWORD_i32},
    {"i64", 3, MIX_TT_KEYWORD_i64},
    {"i8", 2, MIX_TT_KEYWORD_i8},
    {"if", 2, MIX_TT_KEYWORD_if},
    {"impl", 4, MIX_TT_KEYWORD_impl},
    {"import", 6, MIX_TT_KEYWORD_import},
    {"in", 2, MIX_TT_KEYWORD_in},
    {"marco", 5, MIX_TT_KEYWORD_marco},
    {"override", 8, MIX_TT_KEYWORD_override},
    {"return", 6, MIX_TT_KEYWORD_return},
    {"self", 4, MIX_TT_KEYWORD_self},
    {"struct", 6, MIX_TT_KEYWORD_struct},
    {"trait", 5, MIX_TT_KEYWORD_trait},
    {"typeof", 6, MIX_TT_KEYWORD_typeof},
    {"var", 3, MIX_TT_KEYWORD_var},
    {"virtual", 7, MIX_TT_KEYWORD_virtual},
    {"while", 5, MIX_TT_KEYWORD_while},
    {"yield", 5, MIX_TT_KEYWORD_yield},
    {NULL, 0, MIX_TT_INVALID},
};

static const void* default_getkey(const void* data) {
    return data;
}

static int default_equal(const void* a, const void* b) {
    const struct keyword_info* ka = (const struct keyword_info*)a;
    const struct keyword_info* kb = (const struct keyword_info*)b;

    if (ka->word_len != kb->word_len) {
        return 0;
    }

    return (memcmp(ka->word, kb->word, ka->word_len) == 0);
}

static unsigned long default_hash(const void* key) {
    const struct keyword_info* k = (const struct keyword_info*)key;
    return XXH64(k->word, k->word_len, 5);
}

static const struct robin_hood_hash_operations g_hash_ops = {
    .getkey = default_getkey,
    .equal = default_equal,
    .hash = default_hash,
};

static mix_retcode_t init_keyword_hash(struct robin_hood_hash* h) {
    int ret = robin_hood_hash_init(h, 50, ROBIN_HOOD_HASH_DEFAULT_MAX_LOAD_FACTOR, &g_hash_ops);
    if (ret != 0) {
        return MIX_RC_NOMEM;
    }

    for (int i = 0; g_keyword[i].word; ++i) {
        robin_hood_hash_insert(h, (void*)&g_keyword[i]);
    }

    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

static unsigned long get_file_size(FILE* fp) {
    unsigned long pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    unsigned long bytes = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return bytes;
}

static char* read_file_content(const char* fpath, unsigned long* len, struct logger* l) {
    FILE* fp = fopen(fpath, "r");
    if (!fp) {
        logger_error(l, "open file [%s] failed.", fpath);
        return NULL;
    }

    unsigned long sz = get_file_size(fp);
    char* mem = malloc(sz);
    if (!mem) {
        logger_error(l, "alloc [%lu] bytes failed.", sz);
        goto end;
    }

    fread(mem, 1, sz, fp);
    *len = sz;

end:
    fclose(fp);
    return mem;
}

mix_retcode_t mix_lex_init(struct mix_lex* lex, const char* fpath, struct logger* l) {
    unsigned long file_sz = 0;
    lex->buf_begin = read_file_content(fpath, &file_sz, l);
    if (!lex->buf_begin) {
        logger_error(l, "read content of file [%s] failed.", fpath);
        return MIX_RC_INVALID;
    }

    lex->cursor = lex->buf_begin;
    lex->buf_end = lex->buf_begin + file_sz;
    lex->linenum = 1;
    lex->lineoff = 1;
    lex->logger = l;

    return init_keyword_hash(&lex->keyword_hash);
}

static inline char current(struct mix_lex* lex) {
    return (lex->cursor < lex->buf_end) ? *lex->cursor : EOF;
}

static inline void forward(struct mix_lex* lex) {
    if (lex->cursor >= lex->buf_end) {
        return;
    }

    if (*lex->cursor == '\n') {
        ++lex->linenum;
        lex->lineoff = 1;
    } else {
        ++lex->lineoff;
    }

    ++lex->cursor;
}

static inline void forward2(struct mix_lex* lex) {
    lex->cursor += 2;
    lex->lineoff += 2;
}

static inline void backward(struct mix_lex* lex) {
    --lex->cursor;
    --lex->lineoff;
}

static mix_token_type_t parse_literal_string(struct mix_lex* lex, union mix_token_info* token) {
    forward(lex); /* skip starting '"' */
    token->s.base = lex->cursor;

    while (1) {
        char c = current(lex);
        if (c == EOF) {
            return MIX_TT_INVALID;
        }
        /* TODO parse escaped char */
        if (c == '\\') {
            forward2(lex); /* skip '\\' and the escaped char */
            c = current(lex);
        }
        if (c == '"') {
            token->s.size = lex->cursor - (const char*)(token->s.base);
            forward(lex);
            return MIX_TT_LITERAL_STRING;
        }
        forward(lex);
    }

    return MIX_TT_INVALID;
}

static void ignore_comment_line(struct mix_lex* lex) {
    char c;
    do {
        forward(lex);
        c = current(lex);
    } while (c != EOF && c != '\n');
}

static mix_token_type_t parse_lshift(struct mix_lex* lex, union mix_token_info* token) {
    forward(lex); /* skip second '<' */

    if (current(lex) == '=') {
        token->s.size = 3;
        forward(lex);
        return MIX_TT_OP_LSHIFT_ASSIGN;
    }

    token->s.size = 2;
    return MIX_TT_OP_LSHIFT;
}

static mix_token_type_t parse_rshift(struct mix_lex* lex, union mix_token_info* token) {
    forward(lex); /* skip second '>' */

    if (current(lex) == '=') {
        token->s.size = 3;
        forward(lex);
        return MIX_TT_OP_RSHIFT_ASSIGN;
    }

    token->s.size = 2;
    return MIX_TT_OP_RSHIFT;
}

static char skip_digit_sequence(struct mix_lex* lex) {
    char c;
    do {
        forward(lex);
        c = current(lex);
    } while (isdigit(c));
    return c;
}

static mix_token_type_t parse_number(struct mix_lex* lex, union mix_token_info* token) {
    mix_token_type_t type;

    /* integer */
    const char* begin = lex->cursor;
    char c = skip_digit_sequence(lex);
    type = MIX_TT_INTEGER;

    long int_value = ndec2long(begin, lex->cursor - begin);
    double fract_value = 0.0;

    /* fractional */
    if (c == '.') {
        forward(lex);
        c = current(lex);
        if (!isdigit(c)) { /* expect c to be a digit after '.' */
            return MIX_TT_INVALID;
        }

        begin = lex->cursor;
        c = skip_digit_sequence(lex);

        int len = lex->cursor - begin;
        fract_value = (double)int_value + (double)ndec2long(begin, len) / pow(10, len);
        type = MIX_TT_FLOAT;
    }

    long exp_value_for_int = 1;
    double exp_value_for_float = 1.0;

    /* exponent */
    if (c == 'e' || c == 'E') {
        int exp_coeff = 1;
        forward(lex);
        c = current(lex);
        if (c == '+' || c == '-') {
            if (c == '-') {
                exp_coeff = -1;
                if (type == MIX_TT_INTEGER) {
                    fract_value = (double)int_value;
                }
                type = MIX_TT_FLOAT;
            }
            forward(lex);
            c = current(lex);
        }
        if (!isdigit(c)) {
            return MIX_TT_INVALID;
        }

        begin = lex->cursor;
        skip_digit_sequence(lex);

        int len = lex->cursor - begin;
        if (exp_coeff == 1) {
            exp_value_for_int = pow(10, ndec2long(begin, lex->cursor - begin));
        } else {
            exp_value_for_float = (double)1 / pow(10, ndec2long(begin, lex->cursor - begin));
        }
    }

    if (type == MIX_TT_INTEGER) {
        token->l = int_value * exp_value_for_int;
    } else if (type == MIX_TT_FLOAT) {
        token->d = fract_value * exp_value_for_float;
    }

    return type;
}

static mix_token_type_t parse_identifier(struct mix_lex* lex, union mix_token_info* token) {
    token->s.base = lex->cursor;

    char c;
    do {
        forward(lex);
        c = current(lex);
    } while (isalpha(c) || isdigit(c) || c == '_');

    token->s.size = lex->cursor - (char*)token->s.base;

    struct keyword_info word_wanted = {
        .word = token->s.base,
        .word_len = token->s.size,
        .type = MIX_TT_INVALID,
    };
    struct keyword_info* ret = (struct keyword_info*)robin_hood_hash_lookup(&lex->keyword_hash, &word_wanted);
    if (ret) {
        return ret->type;
    }

    return MIX_TT_SYM_IDENTIFIER;
}

mix_token_type_t mix_lex_get_next_token(struct mix_lex* lex, union mix_token_info* token) {
    while (1) {
        char c = current(lex);
        if (isspace(c)) {
            forward(lex);
            continue;
        }
        if (c == '#') {
            ignore_comment_line(lex);
            continue;
        }
        if (c == '"') {
            return parse_literal_string(lex, token);
        }
        if (isdigit(c)) {
            return parse_number(lex, token);
        }

        switch (c) {
            case '<': {
                token->s.base = lex->cursor;
                forward(lex); /* skip first '<' */
                char c2 = current(lex);
                if (c2 == '<') {
                    return parse_lshift(lex, token);
                } else if (c2 == '=') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_LESS_EQUAL;
                } else if (c2 == '|') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_SYM_GENERICS_LEFT_MARK;
                }
                break;
            }
            case '>': {
                token->s.base = lex->cursor;
                forward(lex); /* skip first '>' */
                char c2 = current(lex);
                if (c2 == '>') {
                    return parse_rshift(lex, token);
                } else if (c2 == '=') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_GREATER_EQUAL;
                }
                break;
            }
            case '+': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_ADD_ASSIGN;
                }
                break;
            }
            case '-': {
                forward(lex);
                char c2 = current(lex);
                if (c2 == '=') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_SUB_ASSIGN;
                } if (c2 == '>') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_SYM_RIGHT_ARROW;
                }
                break;
            }
            case '*': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_MUL_ASSIGN;
                }
                break;
            }
            case '/': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_DIV_ASSIGN;
                }
                break;
            }
            case '%': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_MOD_ASSIGN;
                }
                break;
            }
            case '&': {
                forward(lex);
                char c2 = current(lex);
                if (c2 == '=') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_AND_ASSIGN;
                } else if (c2 == '&') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_LOGICAL_AND;
                }
                break;
            }
            case '|': {
                forward(lex);
                char c2 = current(lex);
                if (c2 == '=') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_OR_ASSIGN;
                } else if (c2 == '|') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_LOGICAL_OR;
                } else if (c2 == '>') {
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_SYM_GENERICS_RIGHT_MARK;
                }
                break;
            }
            case '^': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_XOR_ASSIGN;
                }
                break;
            }
            case '=': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_EQUAL;
                }
                break;
            }
            case '!': {
                forward(lex);
                if (current(lex) == '=') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_OP_NOT_EQUAL;
                }
                break;
            }
            case ':': {
                forward(lex);
                if (current(lex) == ':') {
                    token->s.base = lex->cursor - 2;
                    token->s.size = 2;
                    forward(lex);
                    return MIX_TT_SYM_SCOPE_SPECIFIER;
                }
                break;
            }
            case '.': {
                forward(lex);
                if (current(lex) == '.') {
                    forward(lex);
                    if (current(lex) == '.') {
                        token->s.base = lex->cursor - 3;
                        token->s.size = 3;
                        forward(lex);
                        return MIX_TT_SYM_VARIADIC_ARG;
                    }
                    backward(lex);
                }
                break;
            }
            default: {
                if (isalpha(c) || c == '_') {
                    return parse_identifier(lex, token);
                }
                forward(lex);
            }
        }

        /* returns current char and move to the next */
        token->c = c;
        return MIX_TT_CHAR;
    }

    return MIX_TT_INVALID;
}

void mix_lex_destroy(struct mix_lex* lex) {
    if (lex->buf_begin) {
        free(lex->buf_begin);
    }
    robin_hood_hash_destroy(&lex->keyword_hash);
}
