typedef int STACK_TYPE;
#define ELEM_PRINTF_FORM "%d"

#include "gstack.h"
#include "gtest/gtest.h"
 

struct myFixture : public ::testing::Test {
    public:
        char LeftButcher[8];
        stack S;
        char RightButcher[8];
        char Skip[800];
        stack *Stack = &S;

        void SetUp() override {
            // Stack = new stack;
            stack_ctor(Stack);
            stack_push(Stack, 10);
            stack_push(Stack, 11);
            stack_push(Stack, 12);
            stack_push(Stack, 13);
            stack_push(Stack, 14);
            stack_push(Stack, 15);
            stack_push(Stack, 16);
            stack_push(Stack, 17);
            stack_push(Stack, 24);
        }
        void TearDown() override {
            stack_dtor(Stack);
            // delete Stack;
        }
};

TEST(General, Modes)
  {
      printf("Debug modes:\n {\n");
      #ifdef STACK_USE_POISON
          printf("\tSTACK_USE_POISON\n");
      #endif
      #ifdef STACK_USE_WRAPPER
          printf("\tSTACK_USE_WRAPPER\n");
      #endif
      #ifdef STACK_USE_PTR_POISON
          printf("\tSTACK_USE_PTR_POISON\n");
      #endif
      #ifdef STACK_USE_CANARY
          printf("\tSTACK_USE_CANARY\n");
      #endif
      #ifdef STACK_USE_STRUCT_HASH
          printf("\tSTACK_USE_STRUCT_HASH\n");
      #endif
      #ifdef STACK_USE_DATA_HASH
          printf("\tSTACK_USE_DATA_HASH\n");
      #endif
      #ifdef FULL_DEBUG
          printf("\tFULL_DEBUG\n");
      #endif
      #ifdef __SANITIZE_ADDRESS__
          printf("\t__SANITIZE_ADDRESS__\n");                   
      #endif                                   
      #ifdef NDEBUG
          printf("\tNDEBUG\n");
      #endif
      #ifdef STACK_VERBOSE
          printf("\tSTACK_VERBOSE = %d\n", STACK_VERBOSE);
      #endif
      printf(" }\n");
      #if defined(__SANITIZE_ADDRESS__)
          printf("\tERROR: do not compile demo with sanitizers!\n");                 
          exit(1);
      #endif                                   

  }
    
    
TEST_F(myFixture, StructureCanary)
{
    strcpy(LeftButcher + 7, "This is a string11111");
    puts("Altered stack structure with left butcher:\n");
    stack_healthCheck(Stack);
    strcpy(RightButcher - 36, "Another string!");
    puts("Altered stacks structure with right butcher:\n");
    stack_healthCheck(Stack);
}


TEST_F(myFixture, StructureHashCapacity)
{
    Stack->capacity = 100;
    puts("Altered capacity:\n");
    stack_healthCheck(Stack);
}

TEST_F(myFixture, StructureHashLen)
{
    Stack->len = 1000;
    puts("Altered len:\n");
    stack_healthCheck(Stack);
}


TEST_F(myFixture, DataCanary)
{
    stack *this_ = Stack;
    char* lButcher = (char*)(LEFT_CANARY_WRAPPER);
    char* rButcher = (char*)(RIGHT_CANARY_WRAPPER) - 16;
    puts("\nAltered data from left:\n");
    strcpy(lButcher, "Somewhat long string of a sorts...");
    stack_healthCheck(Stack);
    puts("\nAltered data from right:\n");
    strcpy(rButcher, "Hamster running down the cliff...");
    stack_healthCheck(Stack);

}
