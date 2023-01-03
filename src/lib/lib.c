#include "mix/mix.h"
#include "cutils/qbuf.h"
#include "cutils/utils.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */

static void std_io_print(struct mix_context* ctx) {
    int32_t len = 0;
    const char* str = mix_to_str(ctx, -1, &len);
    fwrite(str, len, 1, stdout);
}

static mix_retcode_t __register_std_io_print(struct mix_context* ctx) {
    mix_new_func_type_begin(ctx);
    mix_new_func_type_add_arg_str(ctx);
    mix_new_func_type_end(ctx);
    mix_new_func(ctx, std_io_print);
    mix_register(ctx, "std/io/", "print");
    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

/* TODO copy optimize */
static void std_string_format(struct mix_context* ctx) {
    int32_t len = 0;
    const char* fmt = mix_to_str(ctx, 0, &len);
    int argc = mix_get_stack_size(ctx);
    int arg_idx = 1;

    const char* cursor = fmt;
    const char* end = fmt + len;

    struct qbuf result_str;
    qbuf_init(&result_str);

    while (1) {
        if (cursor >= end) {
            break;
        }

        if (*cursor == '{') {
            ++cursor;
            if (cursor == end) {
                qbuf_append_c(&result_str, '{');
                break;
            }
            if (*cursor == '}') {
                if (arg_idx == argc) {
                    logger_error(mix_get_logger(ctx), "arg mismatch.");
                    goto err;
                }
                mix_type_t type = mix_get_type(ctx, arg_idx);
                if (MIX_TYPE_IS_INT(type)) {
                    char tmp[32];
                    int ll = sprintf(tmp, "%lld", mix_to_i64(ctx, arg_idx));
                    qbuf_append(&result_str, tmp, ll);
                } else if (MIX_TYPE_IS_FLOAT(type)) {
                    char tmp[64];
                    int ll = sprintf(tmp, "%.6lf", mix_to_f64(ctx, arg_idx));
                    qbuf_append(&result_str, tmp, ll);
                } else if (type == MIX_TYPE_STR) {
                    int ll = 0;
                    const char* ss = mix_to_str(ctx, arg_idx, &ll);
                    qbuf_append(&result_str, ss, ll);
                } else {
                    char tmp[32];
                    int ll = sprintf(tmp, "<unsupported[%s]>", mix_get_type_name(type));
                    qbuf_append(&result_str, tmp, ll);
                }

                ++arg_idx; /* move to next arg */
                ++cursor;
                if (cursor == end) {
                    break;
                }
            } else {
                qbuf_append_c(&result_str, '{');
            }
        }

        if (*cursor == '\\') {
            ++cursor; /* skip '\\' */
            qbuf_append_c(&result_str, *cursor);
            ++cursor;
            continue;
        }

        qbuf_append_c(&result_str, *cursor);
        ++cursor;
    }

    mix_push_str(ctx, qbuf_data(&result_str), qbuf_size(&result_str));
err:
    qbuf_destroy(&result_str);
}

static mix_retcode_t __register_std_string_format(struct mix_context* ctx) {
    mix_new_func_type_begin(ctx);
    mix_new_func_type_set_ret_str(ctx);
    mix_new_func_type_add_arg_str(ctx);
    mix_new_func_type_add_arg_variadic(ctx);
    mix_new_func_type_end(ctx);
    mix_new_func(ctx, std_string_format);
    mix_register(ctx, "std/string/", "format");
    return MIX_RC_OK;
}

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_register_stdlib(struct mix_context* ctx) {
    __register_std_io_print(ctx);
    __register_std_string_format(ctx);
    return MIX_RC_OK;
}
