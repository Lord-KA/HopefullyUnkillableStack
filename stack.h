#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


static const size_t STACK_STARTING_LEN = 10;
static const size_t WRAPPING_SIZE = 10;

static const void     *INVALID_PTR = (void*)0xBAD1;
static const void       *FREED_PTR = (void*)0xBAD2;
static const void *DEAD_STRUCT_PTR = (void*)0xBAD3;

static const int ELEM_POISON = 0x9BAD;
static const int WRAPPER_POISON = 0x8BAD;

bool pointerValid(void* ptr)         //TODO add some additional checks
{
    if (ptr == INVALID_PTR || ptr == FREED_PTR || ptr == DEAD_STRUCT_PTR)
        return false;
    return true;
}

enum stack_status {
    OK = 0,
    BAD_SELF_PTR,
    BAD_MEM_ALLOC,
    INTEGRITY_VIOLATED
};

struct stack 
{
    int   *dataWrapper;
    int   *data;
    size_t capacity;
    size_t elemSize;
    size_t len;

    stack_status status;

} typedef stack;



stack_status stack_healthCheck(stack *this_)    //TODO
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    
    if (this_->status)
        return this_->status;

   
    if (this_->capacity == 0 &&                 // check if properly empty
        this_->len      == 0 && 
        this_->dataWrapper == (int*)FREED_PTR &&
        this_->data == (int*)INVALID_PTR )          //TODO think if invalid is good here
    {
        this_->status = OK;
        return OK;
    }

    return OK;
}

void stack_dump(stack *this_)
{
    printf("================================\n");
    printf("    current status = %d\n", this_->status);
    printf("HealthCheck status = %d\n", stack_healthCheck(this_));
    printf(" Capacity = %zu\n", this_->capacity);
    printf("      Len = %zu\n", this_->len);
    printf("Elem size = %zu\n", this_->elemSize);
    printf(" {\n");
    for (size_t i = 0; i < this_->capacity; ++i) {
        if (i < this_->len) {
            printf(" * %d\n", this_->data[i]);
        }
        else {
            printf("   %d\n", this_->data[i]);
        }
    }
    printf(" }\n");
    printf("================================\n");
}


stack_status stack_ctor(stack *this_, size_t elemSize)
{
    this_->elemSize = elemSize;
    this_->capacity = -1;
    this_->len = -1;

    this_->dataWrapper = (int*)calloc(STACK_STARTING_LEN + 2 * WRAPPING_SIZE, this_->elemSize);

    if (!this_->dataWrapper) {
        this_->dataWrapper = (int*)DEAD_STRUCT_PTR;
        this_->data = (int*)DEAD_STRUCT_PTR;
        this_->status = BAD_MEM_ALLOC;
        return this_->status;
    }
    this_->data = this_->dataWrapper + WRAPPING_SIZE;
    this_->capacity = STACK_STARTING_LEN;
    this_->len = 0;
    this_->status = OK;
    return this_->status;
}   


stack_status stack_dtor(stack *this_)
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;

    this_->capacity = -1;
    this_->len = -1;
    free(this_->dataWrapper);
    this_->dataWrapper = (int*)FREED_PTR;
    this_->data = (int*)FREED_PTR;

    return this_->status;
}

stack_status stack_push(stack *this_, int item)
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;

   
    if (this_->len >= this_->capacity) //TODO
    {
        assert(!"Resizing is not emplemented yet!");   
    }

    this_->data[this_->len] = item;
    this_->len += 1;

    return this_->status;
}

stack_status stack_pop(stack *this_, int* item)
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;
    
    *item = this_->data[this_->len - 1];
    this_->len -= 1;
    return this_->status;
}


#endif
