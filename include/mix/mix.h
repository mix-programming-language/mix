#ifndef __MIX_MIX_H__
#define __MIX_MIX_H__

#include "retcode.h"
#include "mix_func_t.h"
#include "typedef.h"
#include "logger/logger.h"

struct mix_context;

/* -------------------------------------------------------------------------- */

struct mix_context* mix_context_new(struct logger*);
void mix_context_delete(struct mix_context*);

void mix_set_logger(struct mix_context*, struct logger*);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_register_stdlib(struct mix_context*);
struct logger* mix_get_logger(struct mix_context*);

/* `prefix` cannot be NULL. `name` is either function, struct or trait. */
mix_retcode_t mix_register(struct mix_context*, const char* prefix, const char* name);

/* -------------------------------------------------------------------------- */

mix_type_t mix_get_type(struct mix_context*, int32_t idx);

mix_retcode_t mix_push_i8(struct mix_context*, int8_t);
mix_retcode_t mix_push_i16(struct mix_context*, int16_t);
mix_retcode_t mix_push_i32(struct mix_context*, int32_t);
mix_retcode_t mix_push_i64(struct mix_context*, int64_t);
mix_retcode_t mix_push_f32(struct mix_context*, float);
mix_retcode_t mix_push_f64(struct mix_context*, double);
mix_retcode_t mix_push_str(struct mix_context*, const char*, int32_t);

int8_t mix_to_i8(struct mix_context*, int32_t idx);
int16_t mix_to_i16(struct mix_context*, int32_t idx);
int32_t mix_to_i32(struct mix_context*, int32_t idx);
int64_t mix_to_i64(struct mix_context*, int32_t idx);
float mix_to_f32(struct mix_context*, int32_t idx);
double mix_to_f64(struct mix_context*, int32_t idx);
const char* mix_to_str(struct mix_context*, int32_t idx, int32_t* len);

mix_retcode_t mix_push(struct mix_context*, int32_t idx);
void mix_pop(struct mix_context*, int32_t nr_item);
mix_retcode_t mix_replace(struct mix_context*, int32_t idx);
mix_retcode_t mix_remove(struct mix_context*, int32_t idx);

int32_t mix_get_stack_size(struct mix_context*);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_new_struct_type_begin(struct mix_context*);
mix_retcode_t mix_new_struct_type_add_i8(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_i16(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_i32(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_i64(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_f32(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_f64(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_str(struct mix_context*, const char* field_name);
mix_retcode_t mix_new_struct_type_add_struct(struct mix_context*, const char* struct_type_name,
                                             const char* field_name);
mix_retcode_t mix_new_struct_type_end(struct mix_context*, const char* name);

mix_retcode_t mix_new_struct(struct mix_context*, const char* struct_type_name,
                             const char* struct_name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_new_trait_type_begin(struct mix_context*);
mix_retcode_t mix_new_trait_type_add_func(struct mix_context*, const char* func_type_name,
                                          const char* field_name);
mix_retcode_t mix_new_trait_type_end(struct mix_context*, const char* name);

mix_retcode_t mix_new_trait(struct mix_context*, const char* struct_type_name,
                            const char* trait_type_name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_new_func_type_begin(struct mix_context*);

mix_retcode_t mix_new_func_type_set_ret_i8(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_i16(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_i32(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_i64(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_f32(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_f64(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_str(struct mix_context*);
mix_retcode_t mix_new_func_type_set_ret_struct(struct mix_context*, const char* struct_type_name);
mix_retcode_t mix_new_func_type_set_ret_trait(struct mix_context*, const char* trait_type_name);

/*
  the following two functions use the -1st object as the source type, and the -2nd
  object as the function object. for example, a function can take a function as a parameter
  and returns another function. both functions are associated with anomymous function types.

  source objects were popped up if success.
*/
mix_retcode_t mix_new_func_type_set_ret_func(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_func(struct mix_context*);

mix_retcode_t mix_new_func_type_add_arg_i8(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_i16(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_i32(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_i64(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_f32(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_f64(struct mix_context*);
mix_retcode_t mix_new_func_type_add_arg_str(struct mix_context*);

/* the '...' MUST be the last component */
mix_retcode_t mix_new_func_type_add_arg_variadic(struct mix_context*);

/* struct and trait must have a name */
mix_retcode_t mix_new_func_type_add_arg_struct(struct mix_context*, const char* struct_type_name);
mix_retcode_t mix_new_func_type_add_arg_trait(struct mix_context*, const char* trait_type_name);

/* function type is left on the top of the stack */
mix_retcode_t mix_new_func_type_end(struct mix_context*);

/* pushes the function type object onto the stack */
mix_retcode_t mix_get_func_type(struct mix_context*);

/*
  function type MUST be on the top of the stack and will be popped if success.
  new function object is left on the top of the stack.
*/
mix_retcode_t mix_new_func(struct mix_context*, mix_func_t);

mix_retcode_t mix_call(struct mix_context*, int32_t argc);

/* -------------------------------------------------------------------------- */

/*
  gets the variable named `name` and pushes it onto the stack.
  nothing is changed if returned value != MIX_RC_OK.
*/
mix_retcode_t mix_get(struct mix_context*, const char* name);

/* sets the top value's name to `name` and pops it from the stack */
mix_retcode_t mix_set(struct mix_context*, const char* name);

mix_retcode_t mix_set_field(struct mix_context*, const char* field_name);

/* -------------------------------------------------------------------------- */

mix_retcode_t mix_eval_file(struct mix_context*, const char* fpath,
                            const char* prefix);
mix_retcode_t mix_eval_buffer(struct mix_context*, const char* buf, int32_t sz,
                              const char* prefix);

#endif
