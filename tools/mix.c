#include "mix/mix.h"
#include "logger/stdio_logger.h"
#include <stddef.h>

int main(int argc, char* argv[]) {
    struct logger stdio_logger;
    stdio_logger_init(&stdio_logger);

    if (argc != 2) {
        logger_error(&stdio_logger, "usage: %s file", argv[0]);
        return -1;
    }

    struct mix_context* ctx = mix_context_new(&stdio_logger);
    if (!ctx) {
        logger_error(&stdio_logger, "init context failed.");
        return -1;
    }

    mix_register_stdlib(ctx);
    mix_retcode_t ret = mix_eval_file(ctx, argv[1], NULL);
    if (ret != MIX_RC_OK) {
        logger_error(&stdio_logger, "exec file [%s] failed.", argv[1]);
    }

    mix_context_delete(ctx);

    return 0;
}
