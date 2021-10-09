#ifndef GSTACK_HEADER_H
#define GSTACK_HEADER_H

/**
 * @file Header for generalized stack structure with additional debug options
 */

#ifdef CHEAP_DEBUG                       /// Debug options that don't require much time or space
    #define STACK_USE_PTR_POISON
    #define STACK_USE_CANARY
    #define STACK_USE_STRUCT_HASH
    #define STACK_USE_CAPACITY_SYS_CHECK
#endif

#ifdef FULL_DEBUG                        /// All implemented debug options
    #define STACK_USE_POISON
    #define STACK_USE_PTR_POISON
    #define STACK_USE_CANARY
    #define STACK_USE_STRUCT_HASH
    #define STACK_USE_DATA_HASH
    #define STACK_USE_CAPACITY_SYS_CHECK  ///  WARNING: CAPACITY_SYS_CHECK must be used carefully 
                                          ///  without sanitizer, because real allocated size 
                                          ///  could be greater than required; 
                                          ///  for more information read `man 3 malloc_usable_size`
    #define STACK_USE_PTR_SYS_CHECK      
    #define STACK_VERBOSE 2
#endif


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <nmmintrin.h>          /// for crc32 intrinsic
#include <inttypes.h>
#define __STDC_FORMAT_MACROS    

#ifdef STACK_USE_CAPACITY_SYS_CHECK
    #include <malloc.h>             /// for sys real capacity checks
#endif

#ifdef _WIN32
    #include <windows.h>            /// for os-dependent sys ptr checks
#endif
#ifdef __unix__
    #include <unistd.h>
    #include <sys/mman.h>
#endif

#include "gstack-config.h"


//===========================================
// Stack options configuration

struct stack;

static const char STACK_LOG_DELIM[] = "===========================";      /// delim for stack logs

static const size_t STACK_STARTING_CAPACITY = 2;                          /// capacity when stack is freshly created

static const double STACK_EXPAND_FACTOR = 2;                              /// stack reallocate expand factor
static const double STACK_SHRINKAGE_FACTOR = 3;                           /// stack reallocate shrink factor

static STACK_TYPE STACK_REFERENCE_POISONED_ELEM;                          /// reference to a poisoned stack elem for easy copm; filled in constructor       //TODO maybe fill in compilation


//===========================================
// Debug options configuration


#ifdef STACK_USE_PTR_POISON
    static const void     *STACK_INVALID_PTR = (void*)0xDEADC0DE1;      /// ptr poison for pointers that got invalidated in runtime
    static const void       *STACK_FREED_PTR = (void*)0xDEADC0DE2;      /// ptr poison for pointers that got freed in runtime
    static const void *STACK_DEAD_STRUCT_PTR = (void*)0xDEADC0DE3;      /// ptr poison for pointers to dead (freed and invalid) structures 
    static const void    *STACK_BAD_PTR_MASK = (void*)0xDEADC0DE;       /// mask for checking if ptr is invalid (comp ptr>>4 with MASK)
#endif

static const size_t STACK_SIZE_T_POISON = -13;                          /// poison for all size_t vars

#ifdef STACK_USE_POISON
    static const char STACK_ELEM_POISON    = 0xFA;           /// one-byte poison for in-use structures
    static const char STACK_FREED_POISON   = 0xFC;           /// one-byte poison for freed structures
#endif

typedef unsigned long long STACK_CANARY_TYPE;          /// Type for canaries can be configured
#ifdef STACK_USE_CANARY   
    static const STACK_CANARY_TYPE  STACK_LEFT_CANARY_POISON = 0xFEEDFACECAFEBEE9;    /// Poison for left  struct and data canaries
    static const STACK_CANARY_TYPE STACK_RIGHT_CANARY_POISON = 0xFEEDFACECAFEBEE8;    /// Poison for right struct and data canaries
    static const size_t STACK_CANARY_WRAPPER_LEN  = 3;                  /// Len of all canary wrappers
    
    // static const ULL STACK_BAD_CANARY_MASK = 0b1111000000;           /// Mask for all status codes associated with a bad canary
#else
    static const size_t STACK_CANARY_WRAPPER_LEN = 0;                   /// Service value for turned off canaries
#endif

#ifndef STACK_VERBOSE
    #define STACK_VERBOSE 0                 /// Service value for stack verbosity if not stated otherwise is 0
#endif


 
typedef int stack_status;                   /// stack status is a bitset inside an int
    
enum stack_status_enum {                        /// ERROR codes for stack
    STACK_OK                      = 0,                  /// All_is_fine status

    STACK_BAD_STRUCT_PTR          = 0b00001,            /// Bad ptr for stack structure provided
    STACK_BAD_DATA_PTR            = 0b00010,            /// Bad ptr for stack data
    STACK_BAD_MEM_ALLOC           = 0b00100,            /// Error during memory (re)allocation
    STACK_INTEGRITY_VIOLATED      = 0b01000,            /// Stack structure intergrity violated
    STACK_DATA_INTEGRITY_VIOLATED = 0b10000,            /// Stack data intergrity violated

     STACK_LEFT_STRUCT_CANARY_CORRUPT = 0b0001000000,   /// Stack canary has been modified
    STACK_RIGHT_STRUCT_CANARY_CORRUPT = 0b0010000000,   /// could happen if big chank of data
       STACK_LEFT_DATA_CANARY_CORRUPT = 0b0100000000,   /// has been carelessly filled with data
      STACK_RIGHT_DATA_CANARY_CORRUPT = 0b1000000000,   /// or if stack data has been writen above it

    STACK_BAD_STRUCT_HASH = 0b0010000000000,            /// Bad hash of all stack structure filds
    STACK_BAD_DATA_HASH   = 0b0100000000000,            /// Bad hash of all the stack data
    STACK_BAD_CAPACITY    = 0b1000000000000             /// Stack capacity has been modified and/or is clearly incorrect
};


//===========================================
// Advanced debug functions


/**
 * @fn static bool ptrValid(const void* ptr)
 * @brief returns true if ptr has passed verification checks, false otherwise
 * @param ptr some pointer associated with the stack
 * @return `true` if ptr is good, `false` otherwise
 */
static bool ptrValid(const void* ptr);


/**
 * @fn static bool stack_isCanaryVal(void* ptr)
 * @brief checks if value of ptr is a stack canary
 * @param ptr pointer to a value (of STACK_CANARY_TYPE)
 * @return `true` if it is a canary, `false` otherwise
 */
#ifdef STACK_USE_CANARY
    static bool stack_isCanaryVal(void* ptr);
#endif


/**
 * @fn static uint64_t stack_calculateStructHash(const stack *this_)
 * @brief calculates stack struct hash
 * @param this_ pointer to const stack struct
 * @return uint64_t hash value
 */
#ifdef STACK_USE_STRUCT_HASH
    static uint64_t stack_calculateStructHash(const stack *this_);
#endif


/**
 * @fn static uint64_t stack_calculateDataHash(const stack *this_)
 * @brief calculates stack data hash bytewise
 * @param this_ pointer to const stack struct
 * @return uint64_t hash value
 */
#ifdef STACK_USE_DATA_HASH
    static uint64_t stack_calculateDataHash(const stack *this_);
#endif


/**
 * @fn static bool stack_isPoisoned(const STACK_TYPE *elem)
 * @brief check if stack elem is filled with one-byte poison
 * @param pointer to a stack elem
 * @return `true` if poisoned, `false` otherwise
 */
#ifdef STACK_USE_POISON
    static bool stack_isPoisoned(const STACK_TYPE *elem);
#endif


/**
 * @fn static size_t stack_getRealCapacity(void* ptr);
 * @brief gets real capacity from the allocator
 * @param ptr to allocated data
 * @return real capacity
 */
#ifdef STACK_USE_CAPACITY_SYS_CHECK
    static size_t stack_getRealCapacity(void* ptr);
#endif


/// macro for accessing left  data canary wrapper from inside of a func with defined `this_`
#define  LEFT_CANARY_WRAPPER (this_->dataWrapper)
/// macro for accessing right data canary wrapper from inside of a func with defined `this_`
#define RIGHT_CANARY_WRAPPER ((STACK_CANARY_TYPE*)((char*)this_->dataWrapper + STACK_CANARY_WRAPPER_LEN * sizeof(STACK_CANARY_TYPE) + this_->capacity * sizeof(STACK_TYPE)))


/**
 * @fn STACK_LOG_TO_STREAM(this_, out, message)
 * @brief macro that logs message and stack to `out` stream
 * @param this_ pointer to stack structure
 * @param out `FILE*` stream to log to
 * @param message c-style string to log with stack
 */
#define STACK_LOG_TO_STREAM(this_, out, message)                                                    \
{                                                                                                    \
    fprintf(out, "%s\n| %s\n", STACK_LOG_DELIM, message);                                             \
    fprintf(out, "| called from func %s on line %d of file %s\n", __func__, __LINE__, __FILE__);       \
    stack_dumpToStream(this_, out);                                                                     \
}


/**
 * @fn STACK_HEALTH_CHECK(this_)
 * @brief macro to run healthcheck and log results and the place it was called from
 * @param this_ pointer to stack structure
 * @return stack_status 
 */
#ifndef NDEBUG
    #define STACK_HEALTH_CHECK(this_) ({                                                                                \
        if (stack_healthCheck(this_)) {                                                                                  \
            fprintf(this_->logStream, "Probles found in healthcheck run from %s on line %zu\n\n", __func__, __LINE__);    \
        }                                                                                                                  \
        this_->status;                                                                                                      \
    })
#else
    #define STACK_HEALTH_CHECK(this_) ({false;})
#endif


/**
 * @fn STACK_PTR_VALIDATE(this__)
 * @brief macro to run ptr checks inside stack_* functions that return `stack_status`
 * @param this__ pointer to stack structure
 * @return stack_status out of func it was used from
 */
#ifndef NDEBUG
    #define STACK_PTR_VALIDATE(this__) {                                                          \
        if (!ptrValid((void*)this__))   {                                                          \
            if (STACK_VERBOSE > 0)                                                                  \
                fprintf(stderr, "Function %s on line %d recieved bad pointer", __func__, __LINE__);  \
            return STACK_BAD_STRUCT_PTR;                                                              \
        }                                                                                              \
    }
#else
    #define STACK_PTR_VALIDATE(this__) {}
#endif

/**
 * @fn STACK_RECALCULATE_DATA_HASH(this_)
 * @brief recalculates data hash if defined `STACK_USE_DATA_HASH`
 * @param this_ pointer to stack structure
 */
#ifdef STACK_USE_DATA_HASH
    #define STACK_RECALCULATE_DATA_HASH(this_) {         \
        this_->dataHash = stack_calculateDataHash(this_); \
    }
#else
    #define STACK_RECALCULATE_DATA_HASH(this_) {}
#endif


//===========================================
// Auxiliary stack functions


/**
 * @addtogroup Auxiliary_funcs
 * @{
 * @fn static size_t stack_expandFactorCalc(size_t capacity)
 * @brief calculates expanded capacity of the stack
 * @param capacity current capacity to expand from  
 * @return new capacity
 */
static size_t stack_expandFactorCalc(size_t capacity);


/**
 * @fn static size_t stack_shrinkageFactorCalc(size_t capacity)
 * @brief calculates shrinked capacity of the stack
 * @param capacity current capacity to shrink from
 * @return new capacity
 */
static size_t stack_shrinkageFactorCalc(size_t capacity);


/**
 * @fn static size_t stack_allocated_size(size_t capacity)
 * @brief calculates allocated data size by the stacks capacity
 * @param capacity to get allocated size from
 * @return allocated size
 * @}
 */
static size_t stack_allocated_size(size_t capacity);


//===========================================
// Stack structure

/**
 * @addtogroup Stack_struct
 * @{
 * @stuct stack
 * @brief generalized stack structure with additional debug features
 */
struct stack                
{
    /// @brief left canary array
    #ifdef STACK_USE_CANARY
        STACK_CANARY_TYPE leftCanary[STACK_CANARY_WRAPPER_LEN];
    #endif
    
    /// @brief ptr to all allocated memory including capacity * sizeof(STACK_TYPE) of data and 2 data wrapper
    STACK_CANARY_TYPE *dataWrapper;
    /// @brief ptr to the stack data
    STACK_TYPE *data;                                   //TODO use macro instead of `data`
    /// @brief capacity of the stack
    size_t capacity;
    /// @brief current lenght of the stack
    size_t len;

    /// @brief bitset of stack statuses
    mutable stack_status status;

    /// @brief outp stream for stack logging
    FILE *logStream;                                    //TODO move logStream to static var
    
    /// @brief hash value of stack structure fields
    #ifdef STACK_USE_STRUCT_HASH
        uint64_t structHash;
    #endif

    /// @brief hash value of bitewise stack data
    #ifdef STACK_USE_DATA_HASH
        uint64_t dataHash;
    #endif
    
    /// @brief right canary array
    #ifdef STACK_USE_CANARY
        STACK_CANARY_TYPE rightCanary[STACK_CANARY_WRAPPER_LEN];
    #endif

} typedef stack;


/**
 * @fn static stack_status stack_ctor(stack *this_)
 * @brief stack constructor
 * @param this_ pointer to memory allocated for stack structure
 * @return bitset of stack status
 */
static stack_status stack_ctor(stack *this_);


/**
 * @fn static stack_status stack_dtor(stack *this_)
 * @brief stack destructor
 * @param this_ pointer to stack structure
 * @return bitset of stack status
 */
static stack_status stack_dtor(stack *this_);


/**
 * @fn static stack_status stack_push(stack *this_, STACK_TYPE item)
 * @brief pushes `item` into stack
 * @param this_ pointer to stack
 * @param item elem to be pushed
 * @return bitset of stack status
 */
static stack_status stack_push(stack *this_, STACK_TYPE item);


/**
 * @fn static stack_status stack_pop (stack *this_, STACK_TYPE item)
 * @brief pops last elem from stack
 * @param this_ pointer to stack
 * @param item pointer to var to write to
 * @return bitset of stack status
 */
static stack_status stack_pop (stack *this_, STACK_TYPE* item);


/**
 * @fn static stack_status stack_healthCheck(stack *this_)
 * @brief checks stacks state and logs every problem
 * @param this_ pointer to stack; const if no capacity sys check because it overrides when capacity if nessasary
 * @return bitset of stack status (of errors)
 */
#ifdef STACK_USE_CAPACITY_SYS_CHECK
static stack_status stack_healthCheck(stack *this_);
#else
static stack_status stack_healthCheck(const stack *this_);  
#endif


/**
 * @fn static stack_status stack_dump(const stack *this_)
 * @brief dumps stack structure and data into this_->logStream
 * @param this_ pointer to stack
 * @return bitset of stack status
 */
static stack_status stack_dump(const stack *this_);


/**
 * @fn static stack_status stack_dumpToStream(const stack *this_, FILE *out)
 * @brief dumps stack structure and data into `out`
 * @param this_ pointer to stack
 * @param out stream for logs
 * @return bitset of stack status
 */
static stack_status stack_dumpToStream(const stack *this_, FILE *out);


/**
 * @fn static stack_status stack_reallocate(stack *this_, const size_t newCapacity)
 * @brief reallocates memory for stack data
 * @param this_ pointer to stack
 * @param newCapacity new capacity to reallocate to
 * @return bitset of stack status
 */
static stack_status stack_reallocate(stack *this_, const size_t newCapacity);


#endif
