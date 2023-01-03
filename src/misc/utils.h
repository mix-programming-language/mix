#ifndef __MIX_MISC_UTILS_H__
#define __MIX_MISC_UTILS_H__

#include "mix/retcode.h"
#include "cutils/qbuf.h"
#include <stdint.h>
#include <errno.h>
#include <stdio.h> /* FILE related */

static inline uint64_t get_file_size(FILE* fp) {
    uint64_t pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    uint64_t bytes = ftell(fp);
    fseek(fp, pos, SEEK_SET);
    return bytes;
}

static inline mix_retcode_t read_file_content(const char* fpath, struct qbuf* res) {
    FILE* fp = fopen(fpath, "r");
    if (!fp) {
        if (errno == ENOENT) {
            return MIX_RC_NOT_FOUND;
        }
        if (errno == EACCES) {
            return MIX_RC_PERMISSION_DENIED;
        }
        return MIX_RC_INTERNAL_ERROR;
    }

    uint64_t sz = get_file_size(fp);
    qbuf_resize(res, sz);
    if (!qbuf_data(res)) {
        fclose(fp);
        return MIX_RC_NOMEM;
    }

    fread(qbuf_data(res), 1, sz, fp);

end:
    fclose(fp);
    return MIX_RC_OK;
}

static inline const char* make_tmp_str_s(const struct qbuf_ref* s) {
    static char buf[1024];
    int sz = (s->size > 1023) ? 1023 : s->size;
    memcpy(buf, s->base, sz);
    buf[s->size] = '\0';
    return buf;
}

static inline const char* make_tmp_str_sl(const char* s, int l) {
    static char buf[1024];
    if (l > 1023) {
        l = 1023;
    }
    memcpy(buf, s, l);
    buf[l] = '\0';
    return buf;
}

static inline const char* make_tmp_str(const struct qbuf_ref* s, char buf[]) {
    memcpy(buf, s->base, s->size);
    buf[s->size] = '\0';
    return buf;
}

#endif
