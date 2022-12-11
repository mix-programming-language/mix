#ifndef __MIX_MEMBER_H__
#define __MIX_MEMBER_H__

#include "type.h"

struct mix_member {
    struct qbuf name;
    struct mix_type* type;
};

struct mix_member* mix_member_new();
void mix_member_delete(struct mix_member*);

#endif
