#include "types.h"
#include "user.h"
#include "uthread.h"

int test = 7;

void print_to_io(int * x)
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

  printf(stdout, "Expected arg = 7, Actual arg = %d\n", *x);

  UT_exit();
}

void userThreadsFuction(){
  printf(1, "Starting userThreadsFunction\n");
  UT_Init();
  UT_Create( (void*)&print_to_io, (void*)&test);
  UT_Create( (void*)&print_to_io, (void*)&test);
  UT_yield();
  print_to_io((void*)&test);
  printf(1, "userThreadsFunction ok\n");
}