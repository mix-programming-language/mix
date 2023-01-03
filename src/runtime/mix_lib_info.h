#ifndef __MIX_RUNTIME_MIX_LIB_INFO_H__
#define __MIX_RUNTIME_MIX_LIB_INFO_H__

#include "cutils/qbuf.h"
#include "cutils/vector.h"

struct mix_lib_info {
    struct qbuf name;
    struct vector identifier_list; /* struct mix_identifier* */
};

struct mix_lib_info* mix_lib_info_new(const struct qbuf_ref* qname);
void mix_lib_info_delete(struct mix_lib_info*);


#endif
