#ifndef __MIX_UTILS_H__
#define __MIX_UTILS_H__

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

#endif
