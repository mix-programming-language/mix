#ifndef __MIX_LIB_INFO_H__
#define __MIX_LIB_INFO_H__

#include "cutils/qbuf.h"
#include "cutils/vector.h"

struct mix_lib_info {
    struct qbuf prefix;
    struct vector var_list;
};

struct mix_lib_info* mix_lib_info_new(const struct qbuf_ref* qname);
void mix_lib_info_delete(struct mix_lib_info*);


#endif
