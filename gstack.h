#ifndef GSTACK_H
#define GSTACK_H

#include "gstack-header.h"


//===========================================
// Advanced debug functions


static bool ptrValid(const void* ptr)         
{
    if (ptr == NULL) {
        return false;
    }

    #ifdef STACK_USE_PTR_POISON
        if ((size_t)ptr>>4 == (size_t)STACK_BAD_PTR_MASK) {           
            return false;
        }
    #endif  
    
    #ifdef STACK_USE_PTR_SYS_CHECK
        #ifdef __unix__
            size_t page_size = sysconf(_SC_PAGESIZE);
            void *base = (void *)((((size_t)ptr) / page_size) * page_size);
            return msync(base, page_size, MS_ASYNC) == 0;
         #else 
            #ifdef _WIN32
                MEMORY_BASIC_INFORMATION mbi = {};
                if (!VirtualQuery(ptr, &mbi, sizeof (mbi)))
                    return false;

                if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
                    return false;  // Guard page -> bad ptr
    
                DWORD readRights = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY
                    | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    
                return (mbi.Protect & readRights) != 0;
            #else
                fprintf(stderr, "WARNING: your OS is unsupported, system pointer checks are diabled!\n");
            #endif
       #endif
    #endif

    return true;
}


#ifdef STACK_USE_CANARY
    static bool stack_isCanaryVal(void* ptr) 
    {
        assert(ptrValid(ptr));

        if (*(STACK_CANARY_TYPE*)ptr == STACK_LEFT_CANARY_POISON)
            return true;
        if (*(STACK_CANARY_TYPE*)ptr == STACK_RIGHT_CANARY_POISON)
            return true;
        return false;
    }
#endif


#ifdef STACK_USE_POISON
    static bool stack_isPoisoned(const STACK_TYPE *elem)                       
    {
        assert(ptrValid(elem));
        return !memcmp(elem, &STACK_REFERENCE_POISONED_ELEM, sizeof(STACK_TYPE));
    }
#endif


#ifdef STACK_USE_CAPACITY_SYS_CHECK
    static size_t stack_getRealCapacity(void *ptr) 
    {
        if (!ptrValid(ptr)) {
            fprintf(stderr, "WARNING: getRealCapacity got a bad pointer!\n");
            return STACK_SIZE_T_POISON;
        }
        size_t allocatedSize = 0;
        #ifdef __unix__
            allocatedSize = malloc_usable_size(ptr);
        #else
            #ifdef _WIN32
                allocatedSize = _msize(ptr); 
            #else
                fprintf(stderr, "WARNING: your OS is unsupported, real capacity check is skipped!\n");
                return STACK_SIZE_T_POISON;              
            #endif
        #endif

        return (allocatedSize - 2 * STACK_CANARY_WRAPPER_LEN * sizeof(STACK_CANARY_TYPE)) / sizeof(STACK_TYPE);
    }
#endif


//===========================================
// Auxiliary stack functions


static size_t stack_expandFactorCalc(size_t capacity)          
{
    #ifdef STACK_USE_CANARY
        if (capacity <= 1)
            return 2 * sizeof(STACK_CANARY_TYPE);

        size_t newCapacity = (((size_t)(capacity * STACK_EXPAND_FACTOR) - 1) / sizeof(STACK_CANARY_TYPE) + 1) * sizeof(STACK_CANARY_TYPE);        // formula for least denominator of sizeof(STACK_CANARY_TYPE), that is > new capacity
        if (newCapacity <= capacity)
            return capacity + 1;
        return newCapacity;
    #else
        if (capacity <= 1)
            return 2;
        size_t newCapacity = capacity * STACK_EXPAND_FACTOR;
        if (newCapacity <= capacity)
            return capacity + 1;
        return newCapacity;
    #endif 
}


static size_t stack_shrinkageFactorCalc(size_t capacity)
{
    #ifdef STACK_USE_CANARY
        if (capacity <= 1)          
            return 2 * sizeof(STACK_CANARY_TYPE);
        size_t newCapacity = (size_t)(capacity * STACK_SHRINKAGE_FACTOR) / sizeof(STACK_CANARY_TYPE) * sizeof(STACK_CANARY_TYPE);               // formula for greatest denominator of sizeof(STACK_CANARY_TYPE), that is < new capacity
        if (newCapacity >= capacity)
            return capacity - 1;
        return newCapacity;
    #else
        if (capacity <= 1)
            return 2;
        size_t newCapacity = capacity * STACK_SHRINKAGE_FACTOR;
        if (newCapacity >= capacity)
            return capacity - 1;
        return newCapacity;
    #endif
}


static size_t stack_allocated_size(size_t capacity) 
{
    return (capacity * sizeof(STACK_TYPE) + 2 * STACK_CANARY_WRAPPER_LEN * sizeof(STACK_CANARY_TYPE));
}


//===========================================
// Stack implementation


static stack_status stack_ctor(stack *this_)
{
    STACK_PTR_VALIDATE(this_);

    this_->capacity = STACK_SIZE_T_POISON;
    this_->len      = STACK_SIZE_T_POISON;
    this_->logStream = stdout;          //TODO
    
    this_->dataWrapper = (STACK_CANARY_TYPE*)calloc(stack_allocated_size(STACK_STARTING_CAPACITY), sizeof(char));

    if (!this_->dataWrapper) {
        #ifdef STACK_USE_PTR_POISON
            this_->dataWrapper = (STACK_CANARY_TYPE*)STACK_DEAD_STRUCT_PTR;
            this_->data        =  (STACK_TYPE*)STACK_DEAD_STRUCT_PTR;
        #endif

        this_->status = STACK_BAD_MEM_ALLOC;
        return this_->status;
    }

    this_->data = (STACK_TYPE*)(this_->dataWrapper + STACK_CANARY_WRAPPER_LEN);
    this_->capacity = STACK_STARTING_CAPACITY;
    this_->len = 0;
    this_->status = STACK_OK;

    #ifdef STACK_USE_CANARY
       for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {
             LEFT_CANARY_WRAPPER[i] =  STACK_LEFT_CANARY_POISON;
            RIGHT_CANARY_WRAPPER[i] = STACK_RIGHT_CANARY_POISON;
            this_-> leftCanary[i]   =  STACK_LEFT_CANARY_POISON;
            this_->rightCanary[i]   = STACK_RIGHT_CANARY_POISON;
        }
    #endif  

    #ifdef STACK_USE_POISON
        memset((char*)(&STACK_REFERENCE_POISONED_ELEM), STACK_ELEM_POISON, sizeof(STACK_TYPE));
        memset((char*)this_->data, STACK_ELEM_POISON, this_->capacity * sizeof(STACK_TYPE));
    #endif

    #ifdef STACK_USE_DATA_HASH
        this_->dataHash = stack_calculateDataHash(this_);
    #endif

    #ifdef STACK_USE_STRUCT_HASH
        this_->structHash = stack_calculateStructHash(this_);
    #endif

    return STACK_HEALTH_CHECK(this_);
}   


static stack_status stack_dtor(stack *this_)           
{
    STACK_PTR_VALIDATE(this_);          

    STACK_HEALTH_CHECK(this_);

    #ifdef STACK_USE_CAPACITY_SYS_CHECK
        size_t newCapacity = stack_getRealCapacity(this_->dataWrapper);
        if (newCapacity != STACK_SIZE_T_POISON)
            this_->capacity = newCapacity;
    #endif

    #ifdef STACK_USE_POISON
        memset((char*)this_->dataWrapper, STACK_FREED_POISON, stack_allocated_size(this_->capacity));
    #endif


    this_->capacity = STACK_SIZE_T_POISON;
    this_->len      = STACK_SIZE_T_POISON;

    if (!ptrValid(this_->dataWrapper)) {
        fprintf(this_->logStream, "ERROR: pointer to the data is invalid!\n");      //TODO think if dtor should be silent here too?
        return STACK_BAD_DATA_PTR;
    }

    free(this_->dataWrapper);
    
    #ifdef STACK_USE_PTR_POISON
        this_->dataWrapper = (STACK_CANARY_TYPE*)STACK_FREED_PTR;
        this_->data        = (STACK_TYPE*)STACK_FREED_PTR;
    #endif

    return STACK_HEALTH_CHECK(this_);
}


static stack_status stack_push(stack *this_, STACK_TYPE item)
{
    STACK_PTR_VALIDATE(this_);

    if (STACK_HEALTH_CHECK(this_))
        return this_->status;
    
    FILE *out = this_->logStream;       //TODO
   
    if (this_->len == this_->capacity) {
        this_->status |= stack_reallocate(this_, stack_expandFactorCalc(this_->capacity));
    }
    

    #ifdef STACK_USE_POISON  
        if (!stack_isPoisoned(&this_->data[this_->len])) {
            STACK_LOG_TO_STREAM(this_, out, "Stack structure corrupt, element was modified!");
            this_->status |= STACK_DATA_INTEGRITY_VIOLATED;
        }
    #endif
    
    #if defined(STACK_USE_CANARY) && defined(STACK_USE_POISON)
        if ((STACK_CANARY_TYPE)this_->data[this_->len] == STACK_RIGHT_CANARY_POISON) {
            STACK_LOG_TO_STREAM(this_, out, "WARNING: Requested elem in wrapper, stack didn't reallocate?");
            this_->status |= STACK_BAD_MEM_ALLOC;
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


static stack_status stack_pop(stack *this_, STACK_TYPE* item)
{
    STACK_PTR_VALIDATE(this_);

    if (STACK_HEALTH_CHECK(this_))
        return this_->status;
    
    if (this_->len == 0) {
        STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: trying to pop from empty stack!");
    }

    this_->len -= 1;

    if (ptrValid(item)) {   
        *item = this_->data[this_->len];
        #ifdef STACK_USE_POISON             //TODO do smth else?
            if (stack_isPoisoned(item)) {                               
                STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING: accessed uninitilized element!");
            }
            memset((char*)(&this_->data[this_->len]), STACK_ELEM_POISON, sizeof(STACK_TYPE));
        #endif

        #ifdef STACK_USE_CANARY
            // if (stack_isCanaryVal(item))                                                                                //TODO think if it is possible to do this check properly
            //     STACK_LOG_TO_STREAM(this_, this_->logStream, "WARNING accessed cannary wrapper element!");
        #endif
    }
   
    #ifdef AUTO_SHRINK
        size_t newCapacity = stack_shrinkageFactorCalc(this_->capacity);

        if (this_->len < newCapacity && this_->capacity > newCapacity)
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


static stack_status stack_reallocate(stack *this_, const size_t newCapacity)
{
    STACK_HEALTH_CHECK(this_);

    #ifdef STACK_USE_POISON
        if (newCapacity < this_->capacity)
        {
            memset((char*)(this_->data + newCapacity), STACK_FREED_POISON, (this_->capacity - newCapacity) * sizeof(STACK_TYPE));
        }
    #endif

    STACK_CANARY_TYPE *newDataWrapper = (STACK_CANARY_TYPE*)realloc(this_->dataWrapper, stack_allocated_size(newCapacity));
    if (newDataWrapper == NULL)             // reallocation failed
    {
        #ifdef STACK_USE_PTR_POISON
            newDataWrapper = (STACK_CANARY_TYPE*)STACK_INVALID_PTR;
        #endif
        return STACK_BAD_MEM_ALLOC;
    }

    if (this_->dataWrapper != newDataWrapper) { 
        this_->dataWrapper = newDataWrapper;
        this_->data = (STACK_TYPE*)(this_->dataWrapper + STACK_CANARY_WRAPPER_LEN);
    }

    #ifdef STACK_USE_POISON
        if (newCapacity > this_->capacity)
            memset((char*)(this_->data + this_->capacity), STACK_ELEM_POISON, (newCapacity - this_->capacity) * sizeof(STACK_TYPE));
    #endif

    this_->capacity = newCapacity;

    #ifdef STACK_USE_CANARY
        for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {
            RIGHT_CANARY_WRAPPER[i] = STACK_RIGHT_CANARY_POISON;
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


static stack_status stack_dumpToStream(const stack *this_, FILE *out)
{
    STACK_PTR_VALIDATE(this_);
    if (!ptrValid(out)) {
        fprintf(stderr, "WARNING: Bad log stream provided, outputing to stderr.\n");
        out = stderr;
    }


    fprintf(out, "%s\n", STACK_LOG_DELIM);
    fprintf(out, "| Stack [%p] :\n", this_);
    fprintf(out, "|----------------\n");

    fprintf(out, "| Current status = %d\n", this_->status); 
    if (this_->status & STACK_BAD_STRUCT_PTR)
        fprintf(out, "| Bad self ptr \n");
    if (this_->status & STACK_BAD_MEM_ALLOC)
        fprintf(out, "| Bad memory allocation \n");
    if (this_->status & STACK_INTEGRITY_VIOLATED)
        fprintf(out, "| Stack integrity violated \n");
    if (this_->status & STACK_DATA_INTEGRITY_VIOLATED)
        fprintf(out, "| Data integrity violated \n");
    if (this_->status & STACK_LEFT_STRUCT_CANARY_CORRUPT)
        fprintf(out, "| Left structure canary corrupted \n");
    if (this_->status & STACK_RIGHT_STRUCT_CANARY_CORRUPT)
        fprintf(out, "| Right structure canary corrupted \n");
    if (this_->status & STACK_LEFT_DATA_CANARY_CORRUPT)
        fprintf(out, "| Left data canary corrupted \n");
    if (this_->status & STACK_RIGHT_DATA_CANARY_CORRUPT)
        fprintf(out, "| Right data canary corrupted \n");
    if (this_->status & STACK_BAD_STRUCT_HASH)
        fprintf(out, "| Bad structure hash, stack may be corrupted \n");
    if (this_->status & STACK_BAD_DATA_HASH)
        fprintf(out, "| Bad data hash, stack data may be corrupted \n");
    if (this_->status & STACK_BAD_CAPACITY)
        fprintf(out, "| Bad capacity, capacity value differs from the allocated one\n");

    size_t capacity = this_->capacity;
    #ifdef STACK_USE_CAPACITY_SYS_CHECK
        if (this_->status & STACK_BAD_CAPACITY) {
            capacity = stack_getRealCapacity(this_->dataWrapper);
            if (capacity == STACK_SIZE_T_POISON) {
                capacity = this_->capacity;
            }
        }
    #endif
 
    if (STACK_VERBOSE >= 1) {
        fprintf(out, "|----------------\n");
        fprintf(out, "| Capacity         = %zu\n", this_->capacity);
        #ifdef STACK_USE_CAPACITY_SYS_CHECK
            fprintf(out, "| Real capacity    = %zu\n", capacity);
        #endif
        fprintf(out, "| Len              = %zu\n", this_->len);
        fprintf(out, "| Data wrapper ptr = %p\n",  this_->dataWrapper);
        fprintf(out, "| Data ptr         = %p\n",  this_->data);
        fprintf(out, "| Elem size        = %zu\n", sizeof(STACK_TYPE));
        #ifdef STACK_USE_STRUCT_HASH
            fprintf(out, "| Struct hash      = %zu\n", this_->structHash);
        #endif
        #ifdef STACK_USE_DATA_HASH
            fprintf(out, "| Data hash        = %zu\n", this_->dataHash);
        #endif
        fprintf(out, "|   {\n");

        #ifdef STACK_USE_CANARY                    //TODO read about graphviz 
            for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {                 
                    fprintf(out, "| l   %llx\n", LEFT_CANARY_WRAPPER[i]);           // `l` for left canary
            }
        #endif
        
        size_t cap = fmin(this_->len, capacity);          // in case structure is corrupt and len > capacity

        for (size_t i = 0; i < cap; ++i) {
                fprintf(out, "| *   %d\n", this_->data[i]);      //TODO add generalized print // `*` for in-use cells
        }

        bool printAll = true;
    
        
        #ifdef STACK_USE_POISON
            if (capacity == this_->capacity) {
                printAll = false;
                for (size_t i = this_->len; i < capacity; ++i) {
                    if (!stack_isPoisoned(&this_->data[i]))
                        printAll = true;
                }
            }
        #endif  
        

        if (capacity < this_->len)
            printAll = true;

        if (capacity - this_->len > 10 && !printAll) {                      // shortens outp of the same poison     
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     ...\n");
            fprintf(out, "|     %x\n", this_->data[this_->len]);
            fprintf(out, "|     %x\n", this_->data[this_->len]);
        }
        else {
            for (size_t i = this_->len; i < capacity; ++i) {
                fprintf(out, "|     %x\n", this_->data[i]);
            }
        }
    
        #ifdef STACK_USE_CANARY             
            #ifdef STACK_USE_CAPACITY_SYS_CHECK
                for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {
                    fprintf(out, "| r   %llx\n", ((STACK_CANARY_TYPE*)((char*)this_->dataWrapper + STACK_CANARY_WRAPPER_LEN * sizeof(STACK_CANARY_TYPE) + capacity * sizeof(STACK_TYPE)))[i]);
                }
            #else
                for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {
                        fprintf(out, "| r   %llx\n", RIGHT_CANARY_WRAPPER[i]);  // `r` for right canary
                }
            #endif
        #endif

        fprintf(out, "|  }\n");
    }
    fprintf(out, "%s\n", STACK_LOG_DELIM);

    return this_->status;
}

static stack_status stack_dump(const stack *this_) 
{
    return stack_dumpToStream(this_, this_->logStream);
}

#ifdef STACK_USE_CAPACITY_SYS_CHECK
static stack_status stack_healthCheck(stack *this_)            // healthcheck changes this_->capacity to realCapacity if the current value is definetly wrong
#else
static stack_status stack_healthCheck(const stack *this_)
#endif
{
    STACK_PTR_VALIDATE(this_);

    FILE *out = this_->logStream;

    if ((this_->capacity == 0 || this_->capacity == STACK_SIZE_T_POISON) &&                     // checks if properly empty
        (this_->len      == 0 || this_->len      == STACK_SIZE_T_POISON)) 
    {
    #ifdef STACK_USE_PTR_POISON                                                     
        if (this_->dataWrapper == (STACK_CANARY_TYPE*)STACK_FREED_PTR   &&
            this_->data        ==  (STACK_TYPE*)STACK_FREED_PTR)                    //TODO think if invalid would fit here 
        {        
            this_->status = STACK_OK;
            return STACK_OK;
        }
    #else
        this_->status = STACK_OK;
        return STACK_OK;
    #endif
    }

    uint64_t hash = 0;

    #ifdef STACK_USE_STRUCT_HASH
        hash = stack_calculateStructHash(this_);
        if (this_->structHash != hash) 
            this_->status |= STACK_BAD_STRUCT_HASH;
    #endif

    if (this_->len > this_->capacity || this_->capacity > 1e20)
        this_->status |= STACK_INTEGRITY_VIOLATED;

    #ifdef STACK_USE_CANARY
    for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {             
        if (this_->leftCanary[i] != STACK_LEFT_CANARY_POISON) {
            this_->status |= STACK_LEFT_STRUCT_CANARY_CORRUPT;
        }

        if (this_->rightCanary[i] != STACK_RIGHT_CANARY_POISON) {
            this_->status |= STACK_RIGHT_STRUCT_CANARY_CORRUPT;
        }
    }
    #endif
    
    if (!ptrValid(this_->dataWrapper)) {
        this_->status |= STACK_BAD_DATA_PTR;
    }
    
    #ifdef STACK_USE_CAPACITY_SYS_CHECK
        size_t capacity = stack_getRealCapacity(this_->dataWrapper);
        if (capacity != STACK_SIZE_T_POISON) {
            if ((capacity < this_->capacity)) {
                STACK_LOG_TO_STREAM(this_, stderr, "Capacity != RealCapacity");
                this_->status |= STACK_BAD_CAPACITY;
                this_->capacity = capacity;
            }
            else {
                #if defined(__SANITIZE_ADDRESS__)
                if (capacity != this_->capacity) {
                    STACK_LOG_TO_STREAM(this_, stderr, "Capacity != RealCapacity");
                    this_->status |= STACK_BAD_CAPACITY;
                    this_->capacity = capacity;
                }
                #endif
            }
       }
    #endif
 

    /// All stack struct checks should happen above here
    /// All stack data   chechs should happen below here


    #ifdef STACK_USE_DATA_HASH
        hash = stack_calculateDataHash(this_);
        if (this_->dataHash != hash) 
            this_->status |= STACK_BAD_DATA_HASH;
    #endif

    #ifdef STACK_USE_CANARY
    for (size_t i = 0; i < STACK_CANARY_WRAPPER_LEN; ++i) {             
        if (LEFT_CANARY_WRAPPER[i] != STACK_LEFT_CANARY_POISON) {      
            this_->status |= STACK_LEFT_DATA_CANARY_CORRUPT;
        }
        
        if (RIGHT_CANARY_WRAPPER[i] != STACK_RIGHT_CANARY_POISON) {
            this_->status |= STACK_RIGHT_DATA_CANARY_CORRUPT;
            }
    }
    #endif


    #ifdef STACK_USE_POISON
        for (size_t i = this_->len; i < this_->capacity; ++i) {
            if (!stack_isPoisoned(&this_->data[i])) {
                this_->status |= STACK_DATA_INTEGRITY_VIOLATED;
            }
        }
    #endif
    
 
    if (this_->status)
        STACK_LOG_TO_STREAM(this_, out, "Problems found during healthcheck!");

    return this_->status;
}


#ifdef STACK_USE_STRUCT_HASH
static uint64_t stack_calculateStructHash(const stack *this_)
{
    assert(ptrValid(this_));

    uint64_t hash = 0;

    hash = _mm_crc32_u64(hash, (uint64_t)(this_->dataWrapper));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->data));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->capacity));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->len));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->status));
    hash = _mm_crc32_u64(hash, (uint64_t)(this_->logStream));
    
    #ifdef STACK_USE_DATA_HASH
        hash = _mm_crc32_u64(hash, (uint64_t)(this_->dataHash));
    #endif

    return hash;
}
#endif


#ifdef STACK_USE_DATA_HASH
static uint64_t stack_calculateDataHash(const stack *this_)
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
