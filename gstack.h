#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef ULL
#define ULL unsigned long long
#endif


#define FULL_DEBUG //TODO
// #define AUTO_SHRINK 

#ifdef FULL_DEBUG
    #define STACK_USE_POISON
    #define STACK_USE_PTR_POISON
    // #define STACK_USE_CANARY
    #define STACK_VERBOSE 2
#endif


static const size_t STACK_STARTING_LEN = 2;

static const double EXPAND_FACTOR = 1.5;
static const double SHRINKAGE_FACTOR = 3;         

#ifdef STACK_USE_PTR_POISON
    static const void     *INVALID_PTR = (void*)0xDEADC0DE1;
    static const void       *FREED_PTR = (void*)0xDEADC0DE2;
    static const void *DEAD_STRUCT_PTR = (void*)0xDEADC0DE3;
    static const void    *BAD_PTR_MASK = (void*)0xDEADC0DE;
#endif

static const size_t SIZE_T_POISON = -13;

#ifdef STACK_USE_POISON
    static const char ELEM_POISON    = 0xFA;           // add one-byte poison?
    static const char FREED_POISON   = 0xFB;
#endif

#ifdef STACK_USE_CANARY   
    static const ULL  LEFT_CANARY_POISON = 0xBADBEEF1;
    static const ULL RIGHT_CANARY_POISON = 0xBADBEEF2;
    static const size_t CANARY_WRAPPER_LEN  = 3;
    
    static const ULL BAD_CANARY_MASK = 0b1111000000;
#else
    static const size_t CANARY_WRAPPER_LEN = 0;
#endif

#ifndef STACK_VERBOSE
    #define STACK_VERBOSE 0
#endif

#ifndef CANARY_TYPE
    typedef ULL CANARY_TYPE;
#endif
 


typedef int STACK_TYPE;

static const char LOG_DELIM[] = "===========================";

bool ptrValid(const void* ptr)         //TODO add some additional checks?
{
    if (ptr == NULL)
        return false;

    #ifdef STACK_USE_PTR_POISON
        if ((size_t)ptr>>4 == (size_t)BAD_PTR_MASK)           //TODO read about NAN tagging / bit masks
            return false;
    #endif  

    return true;
}

typedef int stack_status;

enum stack_status_enum {
    OK                      = 0,

    BAD_SELF_PTR            = 0b0001,
    BAD_MEM_ALLOC           = 0b0010,
    INTEGRITY_VIOLATED      = 0b0100,
    DATA_INTEGRITY_VIOLATED = 0b1000,

     LEFT_STRUCT_CANARY_CORRUPTED = 0b0001000000,
    RIGHT_STRUCT_CANARY_CORRUPTED = 0b0010000000,
       LEFT_DATA_CANARY_CORRUPTED = 0b0100000000,
      RIGHT_DATA_CANARY_CORRUPTED = 0b1000000000
};

struct stack                
{
    #ifdef STACK_USE_CANARY
        ULL leftCanary[CANARY_WRAPPER_LEN];
    #endif
    
    CANARY_TYPE *dataWrapper;
     STACK_TYPE *data;
    size_t capacity;
    size_t len;

    stack_status status;

    FILE *logStream;

    #ifdef STACK_USE_CANARY
        ULL rightCanary[CANARY_WRAPPER_LEN];
    #endif

} typedef stack;


#ifdef STACK_USE_POISON
    bool isPoisoned(STACK_TYPE *elem)                       //TODO add POISONED_ELEM
    {
        for (size_t i = 0; i < sizeof(STACK_TYPE); ++i)
            if (((char*)elem)[i] != ELEM_POISON)
                return false;
        return true;
    }
#endif

void stack_dumpToStream(const stack *this_, FILE *out);

stack_status stack_healthCheck(stack *this_);


static size_t stack_expandFactorCalc(size_t capacity)          
{
    if (capacity <= 1)
        return 2;
    return capacity * EXPAND_FACTOR;
}

static size_t stack_shrinkageFactorCalc(size_t capacity)
{
    if (capacity <= 1)          //TODO think if it is a good practice
        return 2;
    return capacity * SHRINKAGE_FACTOR;
}

#ifdef STACK_USE_CANARY
    static bool isCanaryVal(void* ptr) 
    {
        if (*(CANARY_TYPE*)ptr == LEFT_CANARY_POISON)
            return true;
        if (*(CANARY_TYPE*)ptr == RIGHT_CANARY_POISON)
            return true;
        return false;
    }
#endif

#define  LEFT_CANARY_WRAPPER (this_->dataWrapper)
#define RIGHT_CANARY_WRAPPER ((CANARY_TYPE*)(this_->data + this_->capacity * sizeof(STACK_TYPE)))


#define STACK_LOG_TO_STREAM(this_, out, message)                                              \
{                                                                                              \
    fprintf(out, "%s\n| %s\n", LOG_DELIM, message);                                             \
    fprintf(out, "called from func %s on line %d of file %s", __func__, __LINE__, __FILE__);     \
    stack_dumpToStream(this_, out);                                                               \
}                                                                                                  \

static size_t stack_allocated_size(size_t capacity) 
{
    return (capacity * sizeof(STACK_TYPE) + 2 * CANARY_WRAPPER_LEN * sizeof(CANARY_TYPE));
}

stack_status stack_reallocate(stack *this_, const size_t newCapacity)
{
    #ifdef STACK_USE_POISON
        if (newCapacity < this_->capacity) 
        {
            memset(this_->data + newCapacity, FREED_POISON, this_->capacity - newCapacity);
            // for (size_t i = newCapacity; i < this_->capacity; ++i)
            //     this_->data[i] = FREED_POISON;
        }
    #endif

    CANARY_TYPE *newDataWrapper = (CANARY_TYPE*)realloc(this_->dataWrapper,  newCapacity * sizeof(STACK_TYPE) + 2 * CANARY_WRAPPER_LEN * sizeof(CANARY_TYPE));
    if (newDataWrapper == NULL)             // reallocation failed
    {
        #ifdef STACK_USE_PTR_POISON
            newDataWrapper = (CANARY_TYPE*)INVALID_PTR;
        #endif
        return BAD_MEM_ALLOC;
    }

    if (this_->dataWrapper != newDataWrapper) { 
        this_->dataWrapper = newDataWrapper;
        this_->data = (int*)(this_->dataWrapper + CANARY_WRAPPER_LEN);
    }

    #ifdef STACK_USE_POISON
        memset(this_->data + this_->capacity, ELEM_POISON, newCapacity - this_->capacity);

        // for (size_t i = this_->capacity; i < newCapacity; ++i)
        //     this_->data[i] = ELEM_POISON;
    #endif

    #ifdef STACK_USE_CANARY
        for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {
            RIGHT_CANARY_WRAPPER[i] = RIGHT_CANARY_POISON;
        }
    #endif

    this_->capacity = newCapacity;
    return stack_healthCheck(this_);
}

stack_status stack_healthCheck(stack *this_)    //TODO
{
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }

    // if (this_->status)
    //     return this_->status;

    FILE *out = this_->logStream;

    if ((this_->capacity == 0 || this_->capacity == SIZE_T_POISON) &&                     // checks if properly empty
        (this_->len      == 0 || this_->len      == SIZE_T_POISON)) 
    {
    #ifdef STACK_USE_PTR_POISON                                             //TODO think of a better macro wrap
        if (this_->dataWrapper == (CANARY_TYPE*)FREED_PTR   &&
            this_->data        ==  (STACK_TYPE*)FREED_PTR)                  //TODO think if invalid would fit here 
        {        
            this_->status = OK;
            return OK;
        }
    #else
        this_->status = OK;
        return OK;
    #endif
    }

    #ifdef STACK_USE_CANARY
    for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {             
        if (LEFT_CANARY_WRAPPER[i] != LEFT_CANARY_POISON) {      
            STACK_LOG_TO_STREAM(this_, out, "Left data canary is corrupt!");
            this_->status |= LEFT_DATA_CANARY_CORRUPTED;
        }
        
        if (RIGHT_CANARY_WRAPPER[i] != RIGHT_CANARY_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "Right data canary is corrupt!");
            this_->status |= RIGHT_DATA_CANARY_CORRUPTED;
            }
        
        if (this_->leftCanary[i] != LEFT_CANARY_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "Left struct canary is corrupt!");
            this_->status |= LEFT_STRUCT_CANARY_CORRUPTED;
        }

        if (this_->rightCanary[i] != RIGHT_CANARY_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "Right struct canary is corrupt!");
            this_->status |= RIGHT_STRUCT_CANARY_CORRUPTED;
        }
 
    }

        
    #endif

    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (!isPoisoned(&this_->data[i])) {
                STACK_LOG_TO_STREAM(this_, out, "Data is corrupt!");
                this_->status |= DATA_INTEGRITY_VIOLATED;
            }
        }
    #endif

    

    return OK;
}



void stack_dumpToStream(const stack *this_, FILE *out)
{
    fprintf(out, "%s\n", LOG_DELIM);
    fprintf(out, "| Stack [%p] :\n", this_);
    fprintf(out, "|----------------\n");
    fprintf(out, "| Current status = %d\n", this_->status); //TODO print active status flags
    
    fprintf(out, "|----------------\n");
    fprintf(out, "| Capacity       = %zu\n", this_->capacity);
    fprintf(out, "| Len            = %zu\n", this_->len);
    fprintf(out, "| Elem size      = %zu\n", sizeof(STACK_TYPE));
    fprintf(out, "|   {\n");

    #ifdef STACK_USE_CANARY                    //TODO read about graphviz 
        for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {                 
                fprintf(out, "| w   %llx\n", LEFT_CANARY_WRAPPER[i]);           // `w` for Wrappera    
        }
    #endif

    for (size_t i = 0; i < this_->len; ++i) {
            fprintf(out, "| *   %d\n", this_->data[i]);      //TODO add generalized print // `*` for in-use cells
    }

    bool printAll = false;

    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (!isPoisoned(&this_->data[i]))
                printAll = true;
        }
    #endif  

    if (this_->capacity - this_->len > 10 && !printAll && STACK_VERBOSE > 1) {                // shortens outp of the same poison
        fprintf(out, "|     %d\n", this_->data[this_->len]);
        fprintf(out, "|     %d\n", this_->data[this_->len]);
        fprintf(out, "|     %d\n", this_->data[this_->len]);
        fprintf(out, "|     ...\n");
        fprintf(out, "|     %d\n", this_->data[this_->len]);
        fprintf(out, "|     %d\n", this_->data[this_->len]);
    }
    else {
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            fprintf(out, "|     %d\n", this_->data[i]);
        }
    }

    #ifdef STACK_USE_CANARY
        for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {
                fprintf(out, "| w   %llx\n", RIGHT_CANARY_WRAPPER[i]);
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
    this_->capacity = SIZE_T_POISON;
    this_->len      = SIZE_T_POISON;
    this_->logStream = stdout;          //TODO
    
    this_->dataWrapper = (CANARY_TYPE*)calloc(stack_allocated_size(STACK_STARTING_LEN), sizeof(char));

    if (!this_->dataWrapper) {
        #ifdef STACK_USE_PTR_POISON
            this_->dataWrapper = (CANARY_TYPE*)DEAD_STRUCT_PTR;
            this_->data        =  (STACK_TYPE*)DEAD_STRUCT_PTR;
        #endif

        this_->status = BAD_MEM_ALLOC;
        return this_->status;
    }

    this_->data = (STACK_TYPE*)(this_->dataWrapper + CANARY_WRAPPER_LEN);
    this_->capacity = STACK_STARTING_LEN;
    this_->len = 0;
    this_->status = OK;

    #ifdef STACK_USE_CANARY
       for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {
             LEFT_CANARY_WRAPPER[i] =  LEFT_CANARY_POISON;
            RIGHT_CANARY_WRAPPER[i] = RIGHT_CANARY_POISON;
            this_-> leftCanary[i]   =  LEFT_CANARY_POISON;
            this_->rightCanary[i]   = RIGHT_CANARY_POISON;
        }
    #endif  

    #ifdef STACK_USE_POISON
        memset((char*)this_->data, ELEM_POISON, this_->capacity * sizeof(STACK_TYPE));
        // for (size_t i = 0; i < this_->capacity; ++i)
        //     this_->data[i] = ELEM_POISON;
    #endif

    return stack_healthCheck(this_);
}   


stack_status stack_dtor(stack *this_)           //TODO think about dtor warnings being silent
{
    if (!ptrValid((void*)this_)) {
        return BAD_SELF_PTR;
    }
    if (stack_healthCheck(this_))
        return this_->status;

    #ifdef STACK_USE_POISON
        memset((char*)this_->dataWrapper, FREED_POISON, stack_allocated_size(this_->capacity));
        // for (size_t i = 0; i < this_->capacity + 2 * CANARY_WRAPPER_LEN; ++i)
        //     this_->dataWrapper[i] = FREED_POISON;
    #endif


    this_->capacity = SIZE_T_POISON;
    this_->len      = SIZE_T_POISON;
    free(this_->dataWrapper);
    
    #ifdef STACK_USE_PTR_POISON
        this_->dataWrapper = (CANARY_TYPE*)FREED_PTR;
        this_->data        =  (STACK_TYPE*)FREED_PTR;
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
        this_->status |= stack_reallocate(this_, newCapacity);
    }
    

    #ifdef STACK_USE_POISON  
        if (this_->data[this_->len] != ELEM_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "Stack structure corrupt, element was modified!");
            return stack_healthCheck(this_);
        }
    #endif
    
    #if defined(STACK_USE_CANARY) && defined(STACK_USE_POISON)
        if ((CANARY_TYPE)this_->data[this_->len] == RIGHT_CANARY_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "Requested elem in wrapper, stack didn't reallocate?");
            this_->status |= BAD_MEM_ALLOC;
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
        STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: trying to pop from empty stack!");
    }

    this_->len -= 1;
    *item = this_->data[this_->len];
    
    #ifdef STACK_USE_POISON             //TODO do smth else?
        if (*item == ELEM_POISON) {                               
            STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: accessed uninitilized element!");
        }
        memset((char*)(&this_->data[this_->len]), ELEM_POISON, sizeof(STACK_TYPE));
    #endif

    #ifdef STACK_USE_CANARY
       if (isCanaryVal(item))
            STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING accessed cannary wrapper element!");
    #endif
    
    #ifdef AUTO_SHRINK
        size_t newCapacity = stack_shrinkageFactorCalc(this_->capacity);

        if (this_->len < newCapacity)
        {
            stack_reallocate(this_, newCapacity);
        }
    #endif

    return stack_healthCheck(this_);
}


#endif
