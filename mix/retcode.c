#include "retcode.h"
#include <stddef.h> /* NULL */

static const char* g_retcode_str[] = {
    "success",
    "invalid",
    "no mem",
};

const char* mix_get_retcode_str(mix_retcode_t rc) {
    return (rc < MIX_RC_MAX) ? g_retcode_str[rc] : NULL;
}
