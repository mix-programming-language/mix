#ifndef __MIX_RETCODE_H__
#define __MIX_RETCODE_H__

#include <stdint.h>

enum {
    MIX_RC_OK = 0,
    MIX_RC_INVALID,
    MIX_RC_INTERNAL_ERROR,
    MIX_RC_NOMEM,
    MIX_RC_EXISTS,
    MIX_RC_NOT_FOUND,
    MIX_RC_NOT_IMPLEMENTED,
    MIX_RC_PERMISSION_DENIED,
    MIX_RC_MAX,
};

typedef uint32_t mix_retcode_t;

const char* mix_get_retcode_str(mix_retcode_t);

#endif
