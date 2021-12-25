#include "gtest/gtest.h"

#define STACK_TYPE double
#define ELEM_PRINTF_FORM "%g"

#include "gstack.h"

#include <random>
#include <time.h>
#include <stack>

// std::mt19937 rnd(time(NULL));
std::mt19937 rnd(179);

// #define AUTO_TEST

TEST(GeneralDouble, Modes)
{
    printf("Debug modes:\n {\n");
    #ifdef STACK_USE_POISON
        printf("\tSTACK_USE_POISON\n");
    #endif
    #ifdef STACK_USE_WRAPPER
        printf("\tSTACK_USE_WRAPPER\n");
    #endif
    #ifdef STACK_USE_PTR_POISON
        printf("\tSTACK_USE_PTR_POISON\n");
    #endif
    #ifdef STACK_USE_CANARY                 
        printf("\tSTACK_USE_CANARY\n");     
    #endif                                  
    #ifdef STACK_USE_STRUCT_HASH                                                                                                    
        printf("\tSTACK_USE_STRUCT_HASH\n");
    #endif                                  
    #ifdef STACK_USE_DATA_HASH                                                                                                      
        printf("\tSTACK_USE_DATA_HASH\n");  
    #endif
    #ifdef STACK_USE_CAPACITY_SYS_CHECK
        printf("\tSTACK_USE_CAPACITY_SYS_CHECK\n");
    #endif
    #ifdef FULL_DEBUG
        printf("\tFULL_DEBUG\n");
    #endif
    #ifdef NDEBUG
        printf("\tNDEBUG\n");
    #endif
    #ifdef __SANITIZE_ADDRESS__
        printf("\t__SANITIZE_ADDRESS__\n");
    #endif
    #ifdef STACK_VERBOSE
        printf("\tSTACK_VERBOSE = %d\n", STACK_VERBOSE);
    #endif
    printf(" }\n");
}

TEST(PushPopDouble, Manual)
{
    FILE *log = stdout; //fopen("log.txt", "a");
    GENERIC(stack) S = {};

    GENERIC(stack_ctor)(&S);
    S.logStream = log;
    GENERIC(stack_push)(&S, 12);
    GENERIC(stack_push)(&S, 13);
    GENERIC(stack_dump)(&S);
    GENERIC(stack_push)(&S, 14);
    GENERIC(stack_dump)(&S);
    
    STACK_TYPE res = 0;
    GENERIC(stack_pop)(&S, &res);
    EXPECT_EQ(res, 14);

    GENERIC(stack_dump)(&S);

    GENERIC(stack_dtor)(&S);
}

TEST(PushPopDouble, Random)
{
    GENERIC(stack) S = {};
    std::stack<STACK_TYPE> STD = {};

    GENERIC(stack_ctor)(&S);
    
    size_t iterations = rnd() % 10000 + 100;
    // iterations = 0;
    for (size_t i = 0; i < iterations; ++i) {
        EXPECT_EQ(S.len, STD.size());
        if (rnd() % 10 < 7) {
            STACK_TYPE item = rnd();
            GENERIC(stack_push)(&S, item);
            STD.push(item);
        }
        else if (!(S.len || STD.size())) {
            STACK_TYPE item = 0;
            GENERIC(stack_pop)(&S, &item);
            EXPECT_EQ(item, STD.top());
            STD.pop();
        }
    }
    EXPECT_EQ(S.len, STD.size());
    while (S.len)
    {
        STACK_TYPE item = 0;
        GENERIC(stack_pop)(&S, &item);
        EXPECT_EQ(item, STD.top());
        STD.pop();
     
    }
    GENERIC(stack_dtor)(&S);
}

#undef STACK_TYPE
#undef ELEM_PRINTF_FORM

#define STACK_TYPE long
#define ELEM_PRINTF_FORM "%li"

#include "gstack.h"

TEST(GeneralLong, Modes)
{
    printf("Debug modes:\n {\n");
    #ifdef STACK_USE_POISON
        printf("\tSTACK_USE_POISON\n");
    #endif
    #ifdef STACK_USE_WRAPPER
        printf("\tSTACK_USE_WRAPPER\n");
    #endif
    #ifdef STACK_USE_PTR_POISON
        printf("\tSTACK_USE_PTR_POISON\n");
    #endif
    #ifdef STACK_USE_CANARY                 
        printf("\tSTACK_USE_CANARY\n");     
    #endif                                  
    #ifdef STACK_USE_STRUCT_HASH                                                                                                    
        printf("\tSTACK_USE_STRUCT_HASH\n");
    #endif                                  
    #ifdef STACK_USE_DATA_HASH                                                                                                      
        printf("\tSTACK_USE_DATA_HASH\n");  
    #endif
    #ifdef STACK_USE_CAPACITY_SYS_CHECK
        printf("\tSTACK_USE_CAPACITY_SYS_CHECK\n");
    #endif
    #ifdef FULL_DEBUG
        printf("\tFULL_DEBUG\n");
    #endif
    #ifdef NDEBUG
        printf("\tNDEBUG\n");
    #endif
    #ifdef __SANITIZE_ADDRESS__
        printf("\t__SANITIZE_ADDRESS__\n");
    #endif
    #ifdef STACK_VERBOSE
        printf("\tSTACK_VERBOSE = %d\n", STACK_VERBOSE);
    #endif
    printf(" }\n");
}

TEST(PushPopLong, Manual)
{
    FILE *log = stdout; //fopen("log.txt", "a");
    GENERIC(stack) S = {};

    GENERIC(stack_ctor)(&S);
    S.logStream = log;
    GENERIC(stack_push)(&S, 12);
    GENERIC(stack_push)(&S, 13);
    GENERIC(stack_dump)(&S);
    GENERIC(stack_push)(&S, 14);
    GENERIC(stack_dump)(&S);
    
    STACK_TYPE res = 0;
    GENERIC(stack_pop)(&S, &res);
    EXPECT_EQ(res, 14);

    GENERIC(stack_dump)(&S);

    GENERIC(stack_dtor)(&S);
}

TEST(PushPopLong, Random)
{
    GENERIC(stack) S = {};
    std::stack<STACK_TYPE> STD = {};

    GENERIC(stack_ctor)(&S);
    
    size_t iterations = rnd() % 10000 + 100;
    for (size_t i = 0; i < iterations; ++i) {
        EXPECT_EQ(S.len, STD.size());
        if (rnd() % 10 < 7) {
            STACK_TYPE item = rnd();
            GENERIC(stack_push)(&S, item);
            STD.push(item);
        }
        else if (!(S.len || STD.size())) {
            STACK_TYPE item = 0;
            GENERIC(stack_pop)(&S, &item);
            EXPECT_EQ(item, STD.top());
            STD.pop();
        }
    }
    EXPECT_EQ(S.len, STD.size());
    while (S.len)
    {
        STACK_TYPE item = 0;
        GENERIC(stack_pop)(&S, &item);
        EXPECT_EQ(item, STD.top());
        STD.pop();
     
    }
    GENERIC(stack_dtor)(&S);
}

TEST(PushGet, Random)
{
    GENERIC(stack) S = {};
    std::vector<STACK_TYPE> STD = {};

    GENERIC(stack_ctor)(&S);
    
    size_t iterations = rnd() % 1000 + 100;
    for (size_t i = 0; i < iterations; ++i) {
        EXPECT_EQ(S.len, STD.size());
        if (rnd() % 10 < 7) {
            STACK_TYPE item = rnd();
            GENERIC(stack_push)(&S, item);
            STD.push_back(item);
        }
        else if (!(S.len || STD.size())) {
            int pos = rnd() % S.len;
            STACK_TYPE *item = NULL;
            GENERIC(stack_get)(&S, pos, &item);
            EXPECT_EQ(*item, STD[pos]);
        }
    }
    EXPECT_EQ(S.len, STD.size());
    GENERIC(stack_clear)(&S);
    EXPECT_EQ(S.len, 0);
    GENERIC(stack_dtor)(&S);

}
