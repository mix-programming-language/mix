#ifndef __MIX_PARSER_H__
#define __MIX_PARSER_H__

#include "mix/retcode.h"
#include "context_internal.h"
#include "cutils/qbuf_ref.h"

struct mix_type* mix_parser_lookup_type(struct mix_context*, const struct qbuf_ref* tname);

struct mix_identifier* mix_parser_new_identifier(struct mix_context*, const struct qbuf_ref* id_name);
struct mix_identifier* mix_parser_lookup_identifier(struct mix_context*, const struct qbuf_ref* id_name);

mix_retcode_t mix_parser_push_tov(struct mix_context*, struct mix_type_or_value*);

/* load specified lib to current block */
mix_retcode_t mix_parser_load_lib(struct mix_context*, const struct qbuf_ref* prefix,
                                  const struct qbuf_ref* alias);

#endif
