#include "mix/retcode.h"
#include <stddef.h> /* NULL */

static const char* g_retcode_str[] = {
    "success",
    "invalid",
    "internal error",
    "no mem",
    "exists",
    "not found",
    "not implemented",
    "permission denied",
};

const char* mix_get_retcode_str(mix_retcode_t rc) {
    return (rc < MIX_RC_MAX) ? g_retcode_str[rc] : NULL;
}
