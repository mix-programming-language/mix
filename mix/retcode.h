#ifndef __MIX_RETCODE_H__
#define __MIX_RETCODE_H__

enum {
    MIX_RC_OK = 0,
    MIX_RC_INVALID,
    MIX_RC_NOMEM,
    MIX_RC_MAX,
};

typedef unsigned int mix_retcode_t;

const char* mix_get_retcode_str(mix_retcode_t);

#endif
