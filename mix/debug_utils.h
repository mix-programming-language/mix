#ifndef __MIX_UTILS_H__
#define __MIX_UTILS_H__

#include "cutils/qbuf.h"
#include "cutils/qbuf_ref.h"
#include <stdio.h> /* FILE related */
#include <string.h>

static inline const char* make_tmp_str_s(const struct qbuf_ref* s) {
    static char buf[1024];
    memcpy(buf, s->base, s->size);
    buf[s->size] = '\0';
    return buf;
}

static inline const char* make_tmp_str(const struct qbuf_ref* s, char buf[]) {
    memcpy(buf, s->base, s->size);
    buf[s->size] = '\0';
    return buf;
}

static inline uint64_t get_file_size(FILE* fp) {
    uint64_t pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    uint64_t bytes = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return bytes;
}

static inline int read_file_content(const char* fpath, struct qbuf* res) {
    FILE* fp = fopen(fpath, "r");
    if (!fp) {
        return -1;
    }

    uint64_t sz = get_file_size(fp);
    qbuf_resize(res, sz);
    if (!qbuf_data(res)) {
        fclose(fp);
        return -1;
    }

    fread(qbuf_data(res), 1, sz, fp);

end:
    fclose(fp);
    return 0;
}

#endif
