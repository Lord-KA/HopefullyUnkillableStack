#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// #define FULL_DEBUG //TODO

#ifdef FULL_DEBUG
    #define STACK_USE_POISON
    #define STACK_USE_PTR_POISON
    #define STACK_USE_WRAPPER
    #define STACK_VERBOSE 2
#endif


static const size_t STACK_STARTING_LEN = 2;

static const double EXPAND_FACTOR = 1.5;
static const double SHRINKAGE_FACTOR = 1.3;         //TODO shrink mem in pop

#ifdef STACK_USE_PTR_POISON
    static const void     *INVALID_PTR = (void*)0xDEADC0DE1;
    static const void       *FREED_PTR = (void*)0xDEADC0DE2;
    static const void *DEAD_STRUCT_PTR = (void*)0xDEADC0DE3;
#endif

#ifdef STACK_USE_POISON
    static const int ELEM_POISON    = 0x9BADBEEF;
#endif

#ifdef STACK_USE_WRAPPER
    static const int WRAPPER_POISON = 0x8BADBEEF;
    static const size_t WRAPPER_SIZE = 3;
#else
    static const size_t WRAPPER_SIZE = 0;
#endif

#ifndef STACK_VERBOSE
#define STACK_VERBOSE 0
#endif

static const char LOG_DELIM[] = "===========================";

bool ptrValid(void* ptr)         //TODO add some additional checks
{
    if (ptr == NULL)
        return false;

    #ifdef STACK_USE_PTR_POISON
        if (ptr == INVALID_PTR || ptr == FREED_PTR || ptr == DEAD_STRUCT_PTR)
            return false;
    #endif  

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

static size_t stack_expandFactorCalc(size_t capacity)          
{
    if (capacity <= 1)
        return 2;
    return capacity * EXPAND_FACTOR;
}

void stack_dumpToStream(const stack *this_, FILE *out);

void stack_logErrorToStream(const stack *this_, FILE *out, const char *message)
{
    fprintf(out, "%s\n| %s\n", LOG_DELIM, message);
    stack_dumpToStream(this_, out);
}

void stack_logError(const stack *this_, const char *message)
{
    stack_logErrorToStream(this_, this_->logStream, message);
}

stack_status stack_healthCheck(stack *this_)    //TODO
{
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    
    if (this_->status)
        return this_->status;

    FILE *out = this_->logStream;

    if ((this_->capacity == 0 || this_->capacity == -1) &&                     // checks if properly empty
        (this_->len      == 0 || this_->len      == -1)) 
    {
        #ifdef STACK_USE_PTR_POISON                                 //TODO think of a better macro wrap
            if (this_->dataWrapper == (int*)FREED_PTR   &&
                this_->data        == (int*)FREED_PTR)              //TODO think if invalid is good here 
            {        
                this_->status = OK;
                return OK;
            }
        #else
            this_->status = OK;
            return OK;
        #endif
    }

    #ifdef STACK_USE_WRAPPER
        for (size_t i = 0; i < WRAPPER_SIZE; ++i) {             //TODO maybe add front/back_WRAPPER_CORRUPTED
            if (this_->dataWrapper[i] != WRAPPER_POISON || this_->data[i + this_->capacity] != WRAPPER_POISON) {      
                stack_logErrorToStream(this_, out, "Wrapper is corrupt!");
                this_->status = WRAPPER_CORRUPTED;
                return this_->status;
            }
        }
    #endif

    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (this_->data[i] != ELEM_POISON) {
                stack_logErrorToStream(this_, out, "Data is corrupt!");
                this_->status = DATA_INTEGRITY_VIOLATED;
                return this_->status;
            }
        }
    #endif

    

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

    #ifdef STACK_USE_WRAPPER
        for (size_t i = 0; i < WRAPPER_SIZE; ++i) {
                fprintf(out, "|  W %d\n", this_->dataWrapper[i]);           // `W` for Wrapper
        }
    #endif

    for (size_t i = 0; i < this_->len; ++i) {
            fprintf(out, "|  * %d\n", this_->data[i]);                  // `*` for in-use cells
    }

    bool printAll = false;

    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (this_->data[i] != ELEM_POISON)
                printAll = true;
        }
    #endif  

    if (this_->capacity - this_->len > 10 && printAll && STACK_VERBOSE > 1) {                // shortens the outp of same poison
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

    #ifdef STACK_USE_WRAPPER
        for (size_t i = 0; i < WRAPPER_SIZE; ++i) {
                fprintf(out, "|  W %d\n", this_->data[i + this_->capacity]);
        }
    #endif

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
    this_->logStream = stdout;          //TODO
    
    this_->dataWrapper = (int*)calloc(STACK_STARTING_LEN + 2 * WRAPPER_SIZE, this_->elemSize);

    if (!this_->dataWrapper) {
        #ifdef STACK_USE_PTR_POISON
            this_->dataWrapper = (int*)DEAD_STRUCT_PTR;
            this_->data = (int*)DEAD_STRUCT_PTR;
        #endif

        this_->status = BAD_MEM_ALLOC;
        return this_->status;
    }

    this_->data = this_->dataWrapper + WRAPPER_SIZE;
    this_->capacity = STACK_STARTING_LEN;
    this_->len = 0;
    this_->status = OK;

    #ifdef STACK_USE_WRAPPER
       for (size_t i = 0; i < WRAPPER_SIZE; ++i) {
            this_->dataWrapper[i] = WRAPPER_POISON;
            this_->data[i + this_->capacity] = WRAPPER_POISON;
        }
    #endif  

    #ifdef STACK_USE_POISON
        for (size_t i = 0; i < this_->capacity; ++i)
            this_->data[i] = ELEM_POISON;
    #endif

    return stack_healthCheck(this_);
}   


stack_status stack_dtor(stack *this_)
{
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;

    this_->capacity = -1;
    this_->len = -1;
    free(this_->dataWrapper);
    
    #ifdef STACK_USE_PTR_POISON
        this_->dataWrapper = (int*)FREED_PTR;
        this_->data        = (int*)FREED_PTR;
    #endif

    return stack_healthCheck(this_);
}

stack_status stack_push(stack *this_, int item)
{
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;
    
    FILE *out = this_->logStream;       //TODO
   
    if (this_->len == this_->capacity) 
    {
        const size_t newCapacity = stack_expandFactorCalc(this_->capacity);
        int *newDataWrapper = (int*)realloc(this_->dataWrapper, (newCapacity + 2 * WRAPPER_SIZE) * this_->elemSize);
        if (newDataWrapper == NULL) // reallocation failed
        {
            assert(!"Not emplemented yet!");            //TODO

            newDataWrapper = (int*)calloc(newCapacity, this_->elemSize);
            if (!newDataWrapper) {
                #ifdef STACK_USE_PTR_POISON
                    newDataWrapper = (int*)INVALID_PTR;
                #endif
                return BAD_MEM_ALLOC;
            }
            memcpy(newDataWrapper, this_->dataWrapper, (WRAPPER_SIZE + this_->capacity) * this_->elemSize);
            free(this_->dataWrapper);
            this_->data = newDataWrapper + WRAPPER_SIZE;
            this_->dataWrapper = newDataWrapper;

            #ifdef STACK_USE_POISON
                for (size_t i = this_->capacity; i < newCapacity; ++i)
                    this_->data[i] = ELEM_POISON;
            #endif

            #ifdef STACK_USE_WRAPPER
                for (size_t i = 0; i < WRAPPER_SIZE; ++i)
                    this_->data[newCapacity + i] = WRAPPER_POISON;
            #endif

            this_->capacity = newCapacity;
        }
        else 
        {
            if (this_->dataWrapper != newDataWrapper) {
                this_->dataWrapper = newDataWrapper;
                this_->data = this_->dataWrapper + WRAPPER_SIZE;
            }

            #ifdef STACK_USE_POISON
                for (size_t i = this_->capacity; i < newCapacity; ++i)
                    this_->data[i] = ELEM_POISON;
            #endif

            #ifdef STACK_USE_WRAPPER
                for (size_t i = 0; i < WRAPPER_SIZE; ++i) {
                    this_->data[i + newCapacity] = WRAPPER_POISON;    
                }
            #endif

            this_->capacity = newCapacity;
        }
    }
    

    #ifdef STACK_USE_POISON  
        if (this_->data[this_->len] != ELEM_POISON) {
            stack_logErrorToStream(this_, out, "Stack structure corrupt, element was modified!");
            return stack_healthCheck(this_);
        }
    #endif
    
    #ifdef STACK_USE_WRAPPER
        if (this_->data[this_->len] == WRAPPER_POISON) {
            stack_logErrorToStream(this_, out, "Requested elem in wrapper, stack didn't reallocate?");
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
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;
    
    if (this_->len == 0) {
        stack_logError(this_, "WARNING: trying to pop from empty stack!");
    }

    this_->len -= 1;
    *item = this_->data[this_->len];
    
    #ifdef STACK_USE_POISON             //TODO do smth else?
        if (*item == ELEM_POISON)
            assert(!"Accessed uninitilized element");
        this_->data[this_->len] = ELEM_POISON;
    #endif

    #ifdef STACK_USE_WRAPPER
    if (*item == WRAPPER_POISON)
        assert(!"Accessed cannary wrapper element");
    #endif

    return stack_healthCheck(this_);
}


#endif
