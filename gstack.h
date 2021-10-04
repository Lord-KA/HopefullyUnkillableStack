#ifndef GSTACK_H
#define GSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <nmmintrin.h>
#include <inttypes.h>
#define __STDC_FORMAT_MACROS

#include "stack-config.h"


//===========================================
// Stack options configuration

struct stack;

#ifndef ULL
#define ULL unsigned long long
#endif

static const char LOG_DELIM[] = "===========================";

static const size_t STACK_STARTING_CAPACITY = 2;

static const double EXPAND_FACTOR = 1.5;
static const double SHRINKAGE_FACTOR = 3;         


//===========================================
// Debug options configuration


#ifdef FULL_DEBUG
    #define STACK_USE_POISON
    #define STACK_USE_PTR_POISON
    #define STACK_USE_CANARY
    #define STACK_USE_STRUCT_HASH
    #define STACK_USE_DATA_HASH
    #define STACK_VERBOSE 2
#endif


#ifdef STACK_USE_PTR_POISON
    static const void     *INVALID_PTR = (void*)0xDEADC0DE1;
    static const void       *FREED_PTR = (void*)0xDEADC0DE2;
    static const void *DEAD_STRUCT_PTR = (void*)0xDEADC0DE3;
    static const void    *BAD_PTR_MASK = (void*)0xDEADC0DE;
#endif

static const size_t SIZE_T_POISON = -13;

#ifdef STACK_USE_POISON
    static const char ELEM_POISON    = 0xFA;           // one-byte poison?
    static const char FREED_POISON   = 0xFC;
#endif

#ifdef STACK_USE_CANARY   
    static const ULL  LEFT_CANARY_POISON = 0xFEEDFACECAFEBEE9;
    static const ULL RIGHT_CANARY_POISON = 0xFEEDFACECAFEBEE8;
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
      RIGHT_DATA_CANARY_CORRUPTED = 0b1000000000,

    BAD_STRUCT_HASH = 0b010000000000,
    BAD_DATA_HASH   = 0b100000000000
};



//===========================================
// Advanced debug functions

#ifdef STACK_USE_POISON
    bool isPoisoned(STACK_TYPE *elem)                       //TODO move to memcmp
    {
        for (size_t i = 0; i < sizeof(STACK_TYPE); ++i)     
            if (((char*)elem)[i] != ELEM_POISON)
                return false;
        return true;
    }
#endif

bool ptrValid(const void* ptr)         //TODO add some additional checks?
{
    if (ptr == NULL)
        return false;

    #ifdef STACK_USE_PTR_POISON
        if ((size_t)ptr>>4 == (size_t)BAD_PTR_MASK)           //TODO add system-check option
            return false;
    #endif  

    return true;
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

#ifdef STACK_USE_STRUCT_HASH
uint64_t stack_calculateStructHash(stack *this_);
#endif


#ifdef STACK_USE_DATA_HASH
stack_status stack_calculateDataHash(stack *this_);
#endif


#define  LEFT_CANARY_WRAPPER (this_->dataWrapper)
#define RIGHT_CANARY_WRAPPER ((CANARY_TYPE*)((char*)this_->dataWrapper + CANARY_WRAPPER_LEN * sizeof(CANARY_TYPE) + this_->capacity * sizeof(STACK_TYPE)))


#define STACK_LOG_TO_STREAM(this_, out, message)                                              \
{                                                                                              \
    fprintf(out, "%s\n| %s\n", LOG_DELIM, message);                                             \
    fprintf(out, "| called from func %s on line %d of file %s\n", __func__, __LINE__, __FILE__); \
    stack_dumpToStream(this_, out);                                                               \
}

#ifndef NDEBUG
#define STACK_HEALTH_CHECK(this_) ({                                                        \
    if (stack_healthCheck(this_)) {                                                          \
        fprintf(this_->logStream, "Probles found in healthcheck run from %s\n", __func__);    \
    }                                                                                          \
    this_->status;                                                                              \
})
#else
#define STACK_HEALTH_CHECK(this_) {}
#endif

#ifndef NDEBUG
#define STACK_PTR_VALIDATE(this__) {                                  \
    if (!ptrValid((void*)this__))  {                                   \
        STACK_LOG_TO_STREAM(this__, stderr, "Recieved bad ptr");        \
        return BAD_SELF_PTR;                                             \
    }                                                                     \
}
#else
#define STACK_PTR_VALIDATE(this__) {}
#endif

#ifdef STACK_USE_DATA_HASH
#define STACK_RECALCULATE_DATA_HASH(this_) {         \
    this_->dataHash = stack_calculateDataHash(this_); \
}
#else
#define STACK_RECALCULATE_DATA_HASH(this_) {}
#endif

//===========================================
// Auxiliary stack functions

static size_t stack_expandFactorCalc(size_t capacity)          
{
    #ifdef STACK_USE_CANARY
        if (capacity <= 1)
            return 2 * sizeof(CANARY_TYPE);
        return (((size_t)(capacity * EXPAND_FACTOR) - 1) / sizeof(CANARY_TYPE) + 1) * sizeof(CANARY_TYPE);        // formula for least denominator of sizeof(CANARY_TYPE), that is > new capacity
    #else
        if (capacity <= 1)
            return 2;
        return capacity * EXPAND_FACTOR;
    #endif 
}

static size_t stack_shrinkageFactorCalc(size_t capacity)
{
    #ifdef STACK_USE_CANARY
        if (capacity <= 1)          //TODO think if it is a good practice
            return 2 * sizeof(CANARY_TYPE);
        return (size_t)(capacity * SHRINKAGE_FACTOR) / sizeof(CANARY_TYPE) * sizeof(CANARY_TYPE);               // formula for greatest denominator of sizeof(CANARY_TYPE), that is < new capacity
    #else
        if (capacity <= 1)
            return 2;
        return capacity * SHRINKAGE_FACTOR;
    #endif
}

static size_t stack_allocated_size(size_t capacity) 
{
    return (capacity * sizeof(STACK_TYPE) + 2 * CANARY_WRAPPER_LEN * sizeof(CANARY_TYPE));
}

//===========================================
// Stack structure

struct stack                
{
    #ifdef STACK_USE_CANARY
        CANARY_TYPE leftCanary[CANARY_WRAPPER_LEN];
    #endif
    
    CANARY_TYPE *dataWrapper;
     STACK_TYPE *data;
    size_t capacity;
    size_t len;

    stack_status status;

    FILE *logStream;
    
    #ifdef STACK_USE_STRUCT_HASH
        uint64_t structHash;
    #endif

    #ifdef STACK_USE_DATA_HASH
        uint64_t dataHash;
    #endif

    #ifdef STACK_USE_CANARY
        CANARY_TYPE rightCanary[CANARY_WRAPPER_LEN];
    #endif

} typedef stack;


stack_status stack_ctor(stack *this_);

stack_status stack_dtor(stack *this_);

stack_status stack_push(stack *this_, int item);

stack_status stack_pop(stack *this_, int* item);

stack_status stack_healthCheck(stack *this_);  

uint64_t stack_calculateHash(stack *this_);

stack_status stack_dump(const stack *this_);

stack_status stack_dumpToStream(const stack *this_, FILE *out);

stack_status stack_reallocate(stack *this_, const size_t newCapacity);


//===========================================
// Stack implementation

stack_status stack_ctor(stack *this_)
{
    this_->capacity = SIZE_T_POISON;
    this_->len      = SIZE_T_POISON;
    this_->logStream = stdout;          //TODO
    
    this_->dataWrapper = (CANARY_TYPE*)calloc(stack_allocated_size(STACK_STARTING_CAPACITY), sizeof(char));

    if (!this_->dataWrapper) {
        #ifdef STACK_USE_PTR_POISON
            this_->dataWrapper = (CANARY_TYPE*)DEAD_STRUCT_PTR;
            this_->data        =  (STACK_TYPE*)DEAD_STRUCT_PTR;
        #endif

        this_->status = BAD_MEM_ALLOC;
        return this_->status;
    }

    this_->data = (STACK_TYPE*)(this_->dataWrapper + CANARY_WRAPPER_LEN);
    this_->capacity = STACK_STARTING_CAPACITY;               //TODO rename to CAPACITY
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
    #endif

    #ifdef STACK_USE_DATA_HASH
        this_->dataHash = stack_calculateDataHash(this_);
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        this_->structHash = stack_calculateStructHash(this_);
    #endif

    return STACK_HEALTH_CHECK(this_);
}   


stack_status stack_dtor(stack *this_)           
{
    STACK_PTR_VALIDATE(this_);          

    if (STACK_HEALTH_CHECK(this_))
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

    return STACK_HEALTH_CHECK(this_);
}

stack_status stack_push(stack *this_, int item)
{
    STACK_PTR_VALIDATE(this_);

    if (STACK_HEALTH_CHECK(this_))
        return this_->status;
    
    FILE *out = this_->logStream;       //TODO
   
    if (this_->len == this_->capacity) 
    {
        const size_t newCapacity = stack_expandFactorCalc(this_->capacity);
        this_->status |= stack_reallocate(this_, newCapacity);
    }
    

    #ifdef STACK_USE_POISON  
        if (!isPoisoned(&this_->data[this_->len])) {
            STACK_LOG_TO_STREAM(this_, out, "Stack structure corrupt, element was modified!");
            this_->status |= DATA_INTEGRITY_VIOLATED;
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

    #ifdef STACK_USE_DATA_HASH
        this_->dataHash = stack_calculateDataHash(this_);
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        this_->structHash = stack_calculateStructHash(this_);
    #endif


    return STACK_HEALTH_CHECK(this_);
}


stack_status stack_pop(stack *this_, int* item)
{
    STACK_PTR_VALIDATE(this_);

    if (STACK_HEALTH_CHECK(this_))
        return this_->status;
    
    if (this_->len == 0) {
        STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: trying to pop from empty stack!");
    }

    this_->len -= 1;
    *item = this_->data[this_->len];
    
    #ifdef STACK_USE_POISON             //TODO do smth else?
        if (isPoisoned(item)) {                               
            STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: accessed uninitilized element!");
        }
        memset((char*)(&this_->data[this_->len]), ELEM_POISON, sizeof(STACK_TYPE));
    #endif

    #ifdef STACK_USE_CANARY
        // if (isCanaryVal(item))                                                                                //TODO think if it is possible to do this check properly
        //     STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING accessed cannary wrapper element!");
    #endif
    
    #ifdef AUTO_SHRINK
        size_t newCapacity = stack_shrinkageFactorCalc(this_->capacity);

        if (this_->len < newCapacity)
        {
            stack_reallocate(this_, newCapacity);
        }
    #endif

    #ifdef STACK_USE_DATA_HASH
        this_->dataHash = stack_calculateDataHash(this_);
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        this_->structHash = stack_calculateStructHash(this_);
    #endif


    return STACK_HEALTH_CHECK(this_);
}


stack_status stack_reallocate(stack *this_, const size_t newCapacity)
{
    STACK_HEALTH_CHECK(this_);
    #ifdef STACK_USE_POISON
        if (newCapacity < this_->capacity) 
        {
            memset((char*)(this_->data + newCapacity), FREED_POISON, (this_->capacity - newCapacity) * sizeof(STACK_TYPE));
        }
    #endif

    CANARY_TYPE *newDataWrapper = (CANARY_TYPE*)realloc(this_->dataWrapper, stack_allocated_size(newCapacity));
    if (newDataWrapper == NULL)             // reallocation failed
    {
        #ifdef STACK_USE_PTR_POISON
            newDataWrapper = (CANARY_TYPE*)INVALID_PTR;
        #endif
        return BAD_MEM_ALLOC;
    }

    if (this_->dataWrapper != newDataWrapper) { 
        this_->dataWrapper = newDataWrapper;
        this_->data = (STACK_TYPE*)(this_->dataWrapper + CANARY_WRAPPER_LEN);
    }

    #ifdef STACK_USE_POISON
        memset((char*)(this_->data + this_->capacity), ELEM_POISON, (newCapacity - this_->capacity) * sizeof(STACK_TYPE));
    #endif

    this_->capacity = newCapacity;

    #ifdef STACK_USE_CANARY
        for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {
            RIGHT_CANARY_WRAPPER[i] = RIGHT_CANARY_POISON;
        }
    #endif

    #ifdef STACK_USE_DATA_HASH
        this_->dataHash = stack_calculateDataHash(this_);
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        this_->structHash = stack_calculateStructHash(this_);
    #endif


    return STACK_HEALTH_CHECK(this_);
}


stack_status stack_dumpToStream(const stack *this_, FILE *out)
{
    STACK_PTR_VALIDATE(this_);

    fprintf(out, "%s\n", LOG_DELIM);
    fprintf(out, "| Stack [%p] :\n", this_);
    fprintf(out, "|----------------\n");

    fprintf(out, "| Current status = %d\n", this_->status); 
    if (this_->status & BAD_SELF_PTR)
        fprintf(out, "| Bad self ptr \n");
    if (this_->status & BAD_MEM_ALLOC)
        fprintf(out, "| Bad memory allocation \n");
    if (this_->status & INTEGRITY_VIOLATED)
        fprintf(out, "| Stack integrity violated \n");
    if (this_->status & DATA_INTEGRITY_VIOLATED)
        fprintf(out, "| Data integrity violated \n");
    if (this_->status & LEFT_STRUCT_CANARY_CORRUPTED)
        fprintf(out, "| Left structure canary corrupted \n");
    if (this_->status & RIGHT_STRUCT_CANARY_CORRUPTED)
        fprintf(out, "| Right structure canary corrupted \n");
    if (this_->status & LEFT_DATA_CANARY_CORRUPTED)
        fprintf(out, "| Left data canary corrupted \n");
    if (this_->status & RIGHT_DATA_CANARY_CORRUPTED)
        fprintf(out, "| Right data canary corrupted \n");
    if (this_->status & BAD_STRUCT_HASH)
        fprintf(out, "| Bad structure hash, stack may be corrupted \n");
    if (this_->status & BAD_DATA_HASH)
        fprintf(out, "| Bad data hash, stack data may be corrupted \n");

    if (STACK_VERBOSE >= 1) {
        fprintf(out, "|----------------\n");
        fprintf(out, "| Capacity       = %zu\n", this_->capacity);
        fprintf(out, "| Len            = %zu\n", this_->len);
        fprintf(out, "| Elem size      = %zu\n", sizeof(STACK_TYPE));
        fprintf(out, "| Struct hash    = %zu\n", this_->structHash);
        fprintf(out, "| Data hash      = %zu\n", this_->dataHash);
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
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     ...\n");
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
        }
        else {
            for (size_t i = this_->len; i < this_->capacity; ++i) {
                fprintf(out, "|     %x\n", this_->data[i]);
            }
        }

        #ifdef STACK_USE_CANARY
            for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {
                    fprintf(out, "| w   %llx\n", RIGHT_CANARY_WRAPPER[i]);
            }
        #endif

        fprintf(out, "|  }\n");
    }
    fprintf(out, "%s\n", LOG_DELIM);

    return this_->status;
}

stack_status stack_dump(const stack *this_) 
{
    return stack_dumpToStream(this_, this_->logStream);
}

stack_status stack_healthCheck(stack *this_)    //TODO
{
 
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

    if (this_->len > this_->capacity || this_->capacity > 1e20)
        this_->status |= INTEGRITY_VIOLATED;

    #ifdef STACK_USE_CANARY
    for (size_t i = 0; i < CANARY_WRAPPER_LEN; ++i) {             
        if (LEFT_CANARY_WRAPPER[i] != LEFT_CANARY_POISON) {      
            this_->status |= LEFT_DATA_CANARY_CORRUPTED;
        }
        
        if (RIGHT_CANARY_WRAPPER[i] != RIGHT_CANARY_POISON) {
            this_->status |= RIGHT_DATA_CANARY_CORRUPTED;
            }
        
        if (this_->leftCanary[i] != LEFT_CANARY_POISON) {
            this_->status |= LEFT_STRUCT_CANARY_CORRUPTED;
        }

        if (this_->rightCanary[i] != RIGHT_CANARY_POISON) {
            this_->status |= RIGHT_STRUCT_CANARY_CORRUPTED;
        }
 
    }
    #endif

    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (!isPoisoned(&this_->data[i])) {
                this_->status |= DATA_INTEGRITY_VIOLATED;
            }
        }
    #endif
    
    uint64_t hash;

    #ifdef STACK_USE_DATA_HASH
        hash = stack_calculateDataHash(this_);
        if (this_->dataHash != hash) 
            this_->status |= BAD_DATA_HASH;
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        hash = stack_calculateStructHash(this_);
        if (this_->structHash != hash) 
            this_->status |= BAD_STRUCT_HASH;
    #endif


    if (this_->status)
        STACK_LOG_TO_STREAM(this_, out, "Problems found during healthcheck!");

    return this_->status;
}


#ifdef STACK_USE_STRUCT_HASH
uint64_t stack_calculateStructHash(stack *this_)
{
    assert(ptrValid(this_));

    uint64_t hash = 0;

    hash = _mm_crc32_u64(hash, (uint64_t)(this_->dataWrapper));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->data));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->dataWrapper));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->capacity));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->len));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->status));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->logStream));
    
    #ifdef STACK_USE_DATA_HASH
        hash = _mm_crc32_u64(hash, (uint64_t)(this_->logStream));
    #endif

    return hash;
}
#endif


#ifdef STACK_USE_DATA_HASH
stack_status stack_calculateDataHash(stack *this_)
{
    assert(ptrValid(this_));

    uint64_t hash = 0;

    for (char* iter = (char*)(this_->data); iter < (char*)(this_->data + this_->capacity); ++iter) {
        hash = _mm_crc32_u8(hash, *iter);
    }

    return hash;
}
#endif


#endif
