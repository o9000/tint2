#ifndef TEST_H
#define TEST_H

#include "bool.h"
#include "print.h"

typedef void Test(Status *test_result_);

void register_test_(Test *test, const char *name);

#define TEST(name)                                           \
    void test_##name(Status *test_result_);                  \
    __attribute__((constructor)) void test_register_##name() \
    {                                                        \
        register_test_(test_##name, #name);                  \
    }                                                        \
    void test_##name(Status *test_result_)

void run_all_tests(bool verbose);

#define FAIL_TEST_           \
    *test_result_ = FAILURE; \
    return;

#define ASSERT(value) \
    if (!(value)) {   \
        FAIL_TEST_    \
    }

#define ASSERT_EQUAL(a, b)                              \
    if (!(a == b)) {                                    \
        printf("%s:%d: Assertion failed: %s == %s: ", __FILE__, __LINE__, #a, #b); \
        print(a);                                       \
        printf(" != ");                                 \
        print(b);                                       \
        FAIL_TEST_                                      \
    }

#define ASSERT_DIFFERENT(a, b) \
    if (a == b) {                                    \
        printf("%s:%d: Assertion failed: %s != %s: ", __FILE__, __LINE__, #a, #b); \
        print(a);                                       \
        printf(" == ");                                 \
        print(b);                                       \
        FAIL_TEST_                                      \
    }


#define ASSERT_STR_EQUAL(a, b) \
    if (strcmp(a, b) != 0) {                                    \
        printf("%s:%d: Assertion failed: %s == %s: ", __FILE__, __LINE__, #a, #b); \
        print(a);                                       \
        printf(" != ");                                 \
        print(b);                                       \
        FAIL_TEST_                                      \
    }

#define ASSERT_STR_DIFFERENT(a, b) \
    if (strcmp(a, b) == 0) {                                    \
        printf("%s:%d: Assertion failed: %s != %s: ", __FILE__, __LINE__, #a, #b); \
        print(a);                                       \
        printf(" == ");                                 \
        print(b);                                       \
        FAIL_TEST_                                      \
    }

#define ASSERT_TRUE(value) ASSERT_EQUAL(value, 1)
#define ASSERT_FALSE(value) ASSERT_EQUAL(value, 0)
#define ASSERT_NULL(value) ASSERT_EQUAL(value, NULL)
#define ASSERT_NON_NULL(value) ASSERT_DIFFERENT(value, NULL)

#endif
