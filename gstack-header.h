#ifndef GSTACK_HEADER_H
#define GSTACK_HEADER_H

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

static const char STACK_LOG_DELIM[] = "===========================";

static const size_t STACK_STARTING_CAPACITY = 2;

static const double STACK_EXPAND_FACTOR = 1.5;
static const double STACK_SHRINKAGE_FACTOR = 3;         

static STACK_TYPE STACK_REFERENCE_POISONED_ELEM;

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
    static const void     *STACK_INVALID_PTR = (void*)0xDEADC0DE1;
    static const void       *STACK_FREED_PTR = (void*)0xDEADC0DE2;
    static const void *STACK_DEAD_STRUCT_PTR = (void*)0xDEADC0DE3;
    static const void    *STACK_BAD_PTR_MASK = (void*)0xDEADC0DE;
#endif

static const size_t STACK_SIZE_T_POISON = -13;

#ifdef STACK_USE_POISON
    static const char STACK_ELEM_POISON    = 0xFA;           // one-byte poison?
    static const char STACK_FREED_POISON   = 0xFC;
#endif

#ifdef STACK_USE_CANARY   
    static const ULL  STACK_LEFT_CANARY_POISON = 0xFEEDFACECAFEBEE9;
    static const ULL STACK_RIGHT_CANARY_POISON = 0xFEEDFACECAFEBEE8;
    static const size_t STACK_CANARY_WRAPPER_LEN  = 3;
    
    static const ULL STACK_BAD_CANARY_MASK = 0b1111000000; //TODO renaming from here till the end
#else
    static const size_t STACK_CANARY_WRAPPER_LEN = 0;
#endif

#ifndef STACK_VERBOSE
    #define STACK_VERBOSE 0
#endif


#ifndef CANARY_TYPE
    typedef ULL STACK_CANARY_TYPE;
#endif
 
typedef int stack_status;

enum stack_status_enum {
    STACK_OK                      = 0,

    STACK_BAD_STRUCT_PTR          = 0b0001,
    STACK_BAD_MEM_ALLOC           = 0b0010,
    STACK_INTEGRITY_VIOLATED      = 0b0100,
    STACK_DATA_INTEGRITY_VIOLATED = 0b1000,

     STACK_LEFT_STRUCT_CANARY_CORRUPT = 0b0001000000,
    STACK_RIGHT_STRUCT_CANARY_CORRUPT = 0b0010000000,
       STACK_LEFT_DATA_CANARY_CORRUPT = 0b0100000000,
      STACK_RIGHT_DATA_CANARY_CORRUPT = 0b1000000000,

    STACK_BAD_STRUCT_HASH = 0b010000000000,
    STACK_BAD_DATA_HASH   = 0b100000000000
};


//===========================================
// Advanced debug functions


bool ptrValid(const void* ptr);


#ifdef STACK_USE_CANARY
    static bool stack_isCanaryVal(void* ptr);
#endif

#ifdef STACK_USE_STRUCT_HASH
    uint64_t stack_calculateStructHash(stack *this_);
#endif

#ifdef STACK_USE_POISON
    bool stack_isPoisoned(const STACK_TYPE *elem);
#endif

#ifdef STACK_USE_DATA_HASH
    uint64_t stack_calculateDataHash(stack *this_);
#endif


#define  LEFT_CANARY_WRAPPER (this_->dataWrapper)
#define RIGHT_CANARY_WRAPPER ((STACK_CANARY_TYPE*)((char*)this_->dataWrapper + STACK_CANARY_WRAPPER_LEN * sizeof(STACK_CANARY_TYPE) + this_->capacity * sizeof(STACK_TYPE)))


#define STACK_LOG_TO_STREAM(this_, out, message)                                              \
{                                                                                              \
    fprintf(out, "%s\n| %s\n", STACK_LOG_DELIM, message);                                             \
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
    #define STACK_HEALTH_CHECK(this_) ({false;})
#endif

#ifndef NDEBUG
    #define STACK_PTR_VALIDATE(this__) {                                                          \
        if (!ptrValid((void*)this__))   {                                                          \
            if (STACK_VERBOSE > 0)                                                                  \
                fprintf(stderr, "Function %s on line %d recieved bad pointer", __func__, __LINE__);  \
            return STACK_BAD_STRUCT_PTR;                                                                      \
        }                                                                                              \
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


static size_t stack_expandFactorCalc(size_t capacity);

static size_t stack_shrinkageFactorCalc(size_t capacity);

static size_t stack_allocated_size(size_t capacity);


//===========================================
// Stack structure


struct stack                
{
    #ifdef STACK_USE_CANARY
        STACK_CANARY_TYPE leftCanary[STACK_CANARY_WRAPPER_LEN];
    #endif
    
    STACK_CANARY_TYPE *dataWrapper;
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
        STACK_CANARY_TYPE rightCanary[STACK_CANARY_WRAPPER_LEN];
    #endif

} typedef stack;


stack_status stack_ctor(stack *this_);

stack_status stack_dtor(stack *this_);

stack_status stack_push(stack *this_, int item);

stack_status stack_pop(stack *this_, int* item);

stack_status stack_healthCheck(stack *this_);  

stack_status stack_dump(const stack *this_);

stack_status stack_dumpToStream(const stack *this_, FILE *out);

stack_status stack_reallocate(stack *this_, const size_t newCapacity);


#endif
