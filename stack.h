#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>

static const size_t STACK_STARTING_LEN = 2;



struct stack 
{
    void** data;
    size_t capacity;
    size_t elemSize;
    size_t len;

} typedef stack;

void stack_ctor(stack *this_, size_t elemSize)
{
    this_->data = (void**)calloc(STACK_STARTING_LEN, elemSize);
}

#endif
