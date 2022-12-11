#ifndef __MIX_PARSER_H__
#define __MIX_PARSER_H__

#include "context.h"
#include "cutils/qbuf_ref.h"

struct mix_type* mix_parser_lookup_type(struct mix_context*, const struct qbuf_ref* tname);

struct mix_variable* mix_parser_new_variable(struct mix_context*, const struct qbuf_ref* var_name,
                                             struct mix_type* type);
struct mix_variable* mix_parser_lookup_variable(struct mix_context*, const struct qbuf_ref* var_name);

struct mix_block* mix_parser_get_root_block(struct mix_context*);

#endif
