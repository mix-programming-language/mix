#include "typedef_internal.h"

static const char* type_name[] = {
    "nil", "i8", "i16", "i32", "i64", "f32", "f64", "func", "str", "struct", "trait",
    "<typedef>", "<...>",
};

const char* mix_get_type_name(mix_type_t t) {
    return type_name[t];
}
