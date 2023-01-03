#ifndef __MIX_RUNTIME_MIX_IDENTIFIER_H__
#define __MIX_RUNTIME_MIX_IDENTIFIER_H__

#include "mix_type_or_value.h"
#include "cutils/qbuf.h"

struct mix_identifier {
    struct qbuf name;
    struct mix_type_or_value tov;
};

struct mix_identifier* mix_identifier_new();
void mix_identifier_delete(struct mix_identifier*);

#endif
