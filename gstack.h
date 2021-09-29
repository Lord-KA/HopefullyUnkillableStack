#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


static const size_t STACK_STARTING_LEN = 20;
static const size_t WRAPPING_SIZE = 3;

static const void     *INVALID_PTR = (void*)0xBADBEEF1;
static const void       *FREED_PTR = (void*)0xBADBEEF2;
static const void *DEAD_STRUCT_PTR = (void*)0xBADBEEF3;

static const int ELEM_POISON    = 0x9BADBEEF;
static const int WRAPPER_POISON = 0x8BADBEEF;

static const char LOG_DELIM[] = "===========================";

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
    INTEGRITY_VIOLATED,
    DATA_INTEGRITY_VIOLATED,
    WRAPPER_CORRUPTED
};

struct stack 
{
    int   *dataWrapper;
    int   *data;
    size_t capacity;
    size_t elemSize;
    size_t len;

    stack_status status;

    FILE *logStream;

} typedef stack;


void stack_dumpToStream(const stack *this_, FILE *out);

stack_status stack_healthCheck(stack *this_)    //TODO
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    
    if (this_->status)
        return this_->status;

    FILE *out = this_->logStream;

    if ( this_->capacity == 0  &&                     // checks if properly empty
         this_->len      == 0  && 
         this_->dataWrapper == (int*)FREED_PTR   &&
         this_->data        == (int*)INVALID_PTR )          //TODO think if invalid is good here
    {
        this_->status = OK;
        return OK;
    }

    for (size_t i = 0; i < WRAPPING_SIZE; ++i) {
        if (this_->dataWrapper[i] != WRAPPER_POISON) {
            fprintf(out, "%s\n| Wrapper is corrupt!\n", LOG_DELIM);
            stack_dumpToStream(this_, out);
            this_->status = WRAPPER_CORRUPTED;
            return this_->status;
        }
    }

    for (size_t i = this_->len; i < this_->capacity; ++i) {
        if (this_->data[i] != ELEM_POISON) {
            fprintf(out, "%s\n| Data is corrupt\n", LOG_DELIM);
            stack_dumpToStream(this_, out);
            this_->status = DATA_INTEGRITY_VIOLATED;
            return this_->status;
        }
    }

    for (size_t i = 0; i < WRAPPING_SIZE; ++i) {
        if (this_->data[i + this_->capacity] != WRAPPER_POISON) {
            fprintf(out, "%s\n| Wrapper is corrupt\n", LOG_DELIM);
            stack_dumpToStream(this_, out);
            this_->status = WRAPPER_CORRUPTED;
            return this_->status;
        }
    }

    return OK;
}



void stack_dumpToStream(const stack *this_, FILE *out)
{
    fprintf(out, "%s\n", LOG_DELIM);
    fprintf(out, "| Current status = %d\n", this_->status);
    fprintf(out, "| Capacity       = %zu\n", this_->capacity);
    fprintf(out, "| Len            = %zu\n", this_->len);
    fprintf(out, "| Elem size      = %zu\n", this_->elemSize);
    fprintf(out, "|  {\n");

    for (size_t i = 0; i < WRAPPING_SIZE; ++i) {
            fprintf(out, "|  W %d\n", this_->dataWrapper[i]);           // `W` for Wrapper
    }
    for (size_t i = 0; i < this_->len; ++i) {
            fprintf(out, "|  * %d\n", this_->data[i]);                  // `*` for in-use cells
    }

    bool printAll = false;
    for (size_t i = this_->len; i < this_->capacity; ++i) {
        if (this_->data[i] != ELEM_POISON)
            printAll = true;
    }

    if (this_->capacity - this_->len > 10 && printAll) {                // shortens the outp of same poison
        fprintf(out, "|    %d\n", this_->data[this_->len]);
        fprintf(out, "|    %d\n", this_->data[this_->len]);
        fprintf(out, "|    %d\n", this_->data[this_->len]);
        fprintf(out, "|    ...\n");
        fprintf(out, "|    %d\n", this_->data[this_->len]);
    }
    else {
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            fprintf(out, "|    %d\n", this_->data[i]);
        }
    }

    for (size_t i = 0; i < WRAPPING_SIZE; ++i) {
            fprintf(out, "|  W %d\n", this_->data[i + this_->capacity]);
    }
    fprintf(out, "|  }\n");
    fprintf(out, "%s\n", LOG_DELIM);
}

void stack_dump(const stack *this_) 
{
    stack_dumpToStream(this_, this_->logStream);
}

stack_status stack_ctor(stack *this_)
{
    this_->elemSize = sizeof(int);      //TODO
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

    #ifndef NDEBUG

    for (size_t i = 0; i < WRAPPING_SIZE; ++i)
        this_->dataWrapper[i] = WRAPPER_POISON;

    for (size_t i = 0; i < this_->capacity; ++i)
        this_->data[i] = ELEM_POISON;

    for (size_t i = 0; i < WRAPPING_SIZE; ++i)
        this_->data[i + this_->capacity] = WRAPPER_POISON;
    

    #endif

    return stack_healthCheck(this_);
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

    return stack_healthCheck(this_);
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
    
    #ifndef NDEBUG  //TODO do smth else?
    
    if (this_->data[this_->len] != ELEM_POISON) {
        // fprintf()
        assert(!"Stack structure corrupt, element was modified");
    }

    if (this_->data[this_->len] == WRAPPER_POISON) {
        assert(!"Requested elem in wrapper, stack didn't reallocate?");
        this_->status = BAD_MEM_ALLOC;
        return this_->status;
    }

    #endif  


    this_->data[this_->len] = item;
    this_->len += 1;

    return stack_healthCheck(this_);
}

stack_status stack_pop(stack *this_, int* item)
{
    if (!pointerValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;
    
    if (this_->len == 0) {
        assert(!"Trying to pop from empty stack");
    }

    this_->len -= 1;
    *item = this_->data[this_->len];
    
    #ifndef NDEBUG              //TODO do smth else?
    
    if (*item == ELEM_POISON)
        assert(!"Accessed uninitilized element");
    
    if (*item == WRAPPER_POISON)
        assert(!"Accessed cannary wrapper element");

    #endif

    this_->data[this_->len] = ELEM_POISON;
    return stack_healthCheck(this_);
}


#endif
