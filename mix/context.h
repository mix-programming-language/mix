#ifndef __MIX_CONTEXT_H__
#define __MIX_CONTEXT_H__

#include "retcode.h"
#include "cutils/robin_hood_hash.h"

/* -------------------------------------------------------------------------- */

#include "cutils/vector.h"

/* TODO hide these definitions */

struct mix_context {
    int32_t runtime_stack_base;
    struct vector runtime_stack;
    struct vector block_stack;
    struct robin_hood_hash prefix_var_hash;
};

/* -------------------------------------------------------------------------- */

typedef int (*mix_func_t)(struct mix_context*);

struct mix_context* mix_context_new();
void mix_context_delete(struct mix_context*);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_push_i8(struct mix_context*, int8_t);
mix_retcode_t mix_context_push_i16(struct mix_context*, int16_t);
mix_retcode_t mix_context_push_i32(struct mix_context*, int32_t);
mix_retcode_t mix_context_push_i64(struct mix_context*, int64_t);
mix_retcode_t mix_context_push_f32(struct mix_context*, float);
mix_retcode_t mix_context_push_f64(struct mix_context*, double);
mix_retcode_t mix_context_push_str(struct mix_context*, const char*, uint32_t);

int8_t mix_context_to_i8(struct mix_context*);
int16_t mix_context_to_i16(struct mix_context*);
int32_t mix_context_to_i32(struct mix_context*);
int64_t mix_context_to_i64(struct mix_context*);
float mix_context_to_f32(struct mix_context*);
double mix_context_to_f64(struct mix_context*);
const char* mix_context_to_str(struct mix_context*, uint32_t* len);

void mix_context_push(struct mix_context*, int32_t idx);
void mix_context_pop(struct mix_context*, uint32_t);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_new_struct_type_begin(struct mix_context*);
mix_retcode_t mix_context_new_struct_type_add_i8(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_i16(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_i32(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_i64(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_f32(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_f64(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_str(struct mix_context*, const char* field_name);
mix_retcode_t mix_context_new_struct_type_add_struct(struct mix_context*, const char* struct_type_name,
                                                 const char* field_name);
mix_retcode_t mix_context_new_struct_type_end(struct mix_context*, const char* name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_new_trait_type_begin(struct mix_context*);
mix_retcode_t mix_context_new_trait_type_add_func(struct mix_context*, const char* func_type_name,
                                              const char* field_name);
mix_retcode_t mix_context_new_trait_type_end(struct mix_context*, const char* name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_new_func_begin(struct mix_context*);
mix_retcode_t mix_context_new_func_set_returned_type(struct mix_context*, const char* type_name);
mix_retcode_t mix_context_new_func_add_arg_i8(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_i16(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_i32(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_i64(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_f32(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_f64(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_str(struct mix_context*);
mix_retcode_t mix_context_new_func_add_arg_struct(struct mix_context*, const char* struct_type_name);
mix_retcode_t mix_context_new_func_add_arg_trait(struct mix_context*, const char* trait_type_name);
mix_retcode_t mix_context_new_func_add_arg_func(struct mix_context*, const char* func_type_name);
mix_retcode_t mix_context_new_func_end(struct mix_context*, mix_func_t func);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_get_struct(struct mix_context*, const char* var_name);
mix_retcode_t mix_context_new_struct(struct mix_context*, const char* struct_type_name, const char* var_name);

mix_retcode_t mix_context_get_trait(struct mix_context*, const char* struct_type_name, const char* trait_type_name);
mix_retcode_t mix_context_new_trait(struct mix_context*, const char* struct_type_name, const char* trait_type_name);

mix_retcode_t mix_context_set_field(struct mix_context*, const char* field_name);

/* -------------------------------------------------------------------------- */

int32_t mix_context_get_top_index(struct mix_context*);
void mix_context_move(struct mix_context*, int32_t from, int32_t to);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_get(struct mix_context*, const char* name);
mix_retcode_t mix_context_set(struct mix_context*, const char* name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_context_eval(struct mix_context*, const char* buf, uint32_t sz);

#endif
