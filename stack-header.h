#ifndef GSTACK_CONFIG_H
#define GSTACK_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// #define FULL_DEBUG //TODO
// #define AUTO_SHRINK 

#ifndef ULL
#define ULL unsigned long long
#endif


#ifdef FULL_DEBUG
    #define STACK_USE_POISON
    #define STACK_USE_PTR_POISON
    #define STACK_USE_CANARY
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
 



#endif
