#include "stack.h"

int main()
{
    stack S = {};

    stack_ctor(&S, sizeof(int));
    stack_push(&S, 12);
    stack_push(&S, 13);
    stack_push(&S, 14);
    stack_dump(&S);
    
    int res = 0;
    stack_pop(&S, &res);
    printf("res = %d\n", res);

    stack_dump(&S);

    stack_dtor(&S);
}
