#include "lex/mix_lex.h"
#include "misc/debug_utils.h"
#include <stdio.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

static void do_test_int(const char* str, int expected_value) {
    struct mix_lex lex;
    mix_lex_init(&lex, str, strlen(str));

    union mix_token_info token;
    mix_token_type_t ret = mix_lex_get_next_token(&lex, &token);
    assert(ret == MIX_TT_INTEGER);
    assert(token.l == expected_value);

    mix_lex_destroy(&lex);
}

static void test_int() {
    do_test_int("123", 123);
    do_test_int("123e+1", 1230);
    do_test_int("123000", 123000);
    do_test_int("000012300", 12300);
}

static inline int float_equal(double a, double b) {
#define delta 1e-9
    return (a > b) ? (a - b <= delta) : (b - a <= delta);
}

static void do_test_float(const char* str, double expected_value) {
    struct mix_lex lex;
    mix_lex_init(&lex, str, strlen(str));

    union mix_token_info token;
    mix_token_type_t ret = mix_lex_get_next_token(&lex, &token);
    assert(ret == MIX_TT_FLOAT);
    assert(float_equal(token.d, expected_value) == 1);

    mix_lex_destroy(&lex);
}

static void test_float() {
    do_test_float("123.456", 123.456);
    do_test_float("4576.1", 4576.1);
    do_test_float("1.23e-2", 0.0123);
    do_test_float("1.34e+5", 134000);
    do_test_float("3e-3", 0.003);
}

static void do_test_string(const char* str, const char* expected) {
    struct mix_lex lex;
    mix_lex_init(&lex, str, strlen(str));

    union mix_token_info token;
    mix_token_type_t ret = mix_lex_get_next_token(&lex, &token);
    printf("get string -> [%s]\n", make_tmp_str_s(&token.s));
    assert(ret == MIX_TT_LITERAL_STRING);
    assert(strcmp(expected, make_tmp_str_s(&token.s)) == 0);
}

static void test_literal_string() {
    do_test_string("\"{} + {} = {}\"", "{} + {} = {}");
}

#define TEST_ITEM(func) {#func, func}

struct {
    const char* name;
    void (*func)();
} g_test_suite[] = {
    TEST_ITEM(test_int),
    TEST_ITEM(test_float),
    TEST_ITEM(test_literal_string),
    TEST_ITEM(NULL),
};

int main(void) {
    for (int i = 0; g_test_suite[i].func; ++i) {
        printf("----- %s -----\n", g_test_suite[i].name);
        g_test_suite[i].func();
    }
    return 0;
}
