#include "gtest/gtest.h"

#include "gstack.h"

#include <random>
#include <time.h>
#include <stack>

// std::mt19937 rnd(time(NULL));
std::mt19937 rnd(179);

#define STACK_USE_WRAPPER
#define AUTO_TEST

TEST(General, Modes)
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
    #ifdef FULL_DEBUG
        printf("\tFULL_DEBUG\n");
    #endif
    #ifdef NDEBUG
        printf("\tNDEBUG\n");
    #endif
    #ifdef STACK_VERBOSE
        printf("\tSTACK_VERBOSE = %d\n", STACK_VERBOSE);
    #endif
    printf(" }\n");
}

TEST(PushPop, Manual)
{
    FILE *log = fopen("log.txt", "a");
    stack S = {};

    stack_ctor(&S);
    S.logStream = log;
    stack_push(&S, 12);
    stack_push(&S, 13);
    stack_push(&S, 14);
    stack_dump(&S);
    
    int res = 0;
    stack_pop(&S, &res);
    EXPECT_EQ(res, 14);

    stack_dump(&S);

    stack_dtor(&S);
}

TEST(PushPop, Random)
{
    stack S = {};
    std::stack<int> STD = {};

    stack_ctor(&S);
    
    size_t iterations = rnd() % 30000 + 100;
    // iterations = 1;
    for (size_t i = 0; i < iterations; ++i) {
        EXPECT_EQ(S.len, STD.size());
        if (rnd() % 10 < 7) {
            int item = rnd();
            stack_push(&S, item);
            STD.push(item);
        }
        else if (!(S.len || STD.size())) {
            int item = 0;
            stack_pop(&S, &item);
            EXPECT_EQ(item, STD.top());
            STD.pop();
        }
    }
    EXPECT_EQ(S.len, STD.size());
    while (S.len)
    {
        int item = 0;
        stack_pop(&S, &item);
        EXPECT_EQ(item, STD.top());
        STD.pop();
     
    }
    stack_dtor(&S);
}
