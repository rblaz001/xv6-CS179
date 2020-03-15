#include "types.h"
#include "user.h"
#include "uthread.h"

int test = 7;

void print_to_stdout(int *x);

// This function tests the user thread library
// It initializes the user thread library then generates two additional user threads
// Each thread is given the entry point of to function print_to_stdout with int test passed by reference
// print_to_stdout prints numbers one through ten, calling UT_yield after each print
// Finally they verify that the arg passed is what is expected
void userThreadsFuction(){
  printf(1, "Starting userThreadsFunction\n");
  if(UT_Init() == -1)
  {
     printf(1, "Panic: UT_Init failed\n");
     exit();
  }
  if(UT_Create( (void*)&print_to_stdout, (void*)&test) == -1)
  {
     printf(1, "Panic: UT_Create 1 failed\n");
     exit();
  }
  if(UT_Create( (void*)&print_to_stdout, (void*)&test) == -1)
  {
     printf(1, "Panic: UT_Create 2 failed\n");
     exit();
  }
  UT_yield();
  print_to_stdout((void*)&test);
  printf(1, "userThreadsFunction ok\n");
}

// Helper function called by userThreadsFunction
void print_to_stdout(int * x)
{
  printf(1, "address of arg = %d\n", x);
  int stdout = 1;

  for(int i = 1; i <= 10; i++)
  {
      printf(stdout, "%d\n", i);
      UT_yield();

    if(i == 10){
      printf(stdout, "Finished\n");
      UT_yield();
    }
  }

  printf(stdout, "Expected arg = 7, Actual arg = %d with address: %d\n", *x, x);

  if(*x != 7)
  {
    printf(stdout, "Panic: arg does not match expected");
    exit();
  }

  // Terminates user thread and yields. 
  // If caller is root user thread, then calls yield until all other user threads exit. 
  UT_exit();
}