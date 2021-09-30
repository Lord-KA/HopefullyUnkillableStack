#include "gtest/gtest.h"

#include "gstack.h"

#include <random>
#include <time.h>
#include <stack>

// std::mt19937 rnd(time(NULL));
std::mt19937 rnd(179);


TEST(PushPop, Manual)
{
    stack S = {};

    stack_ctor(&S);
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
    
    size_t iterations = rnd() % 10000 + 1000;
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
