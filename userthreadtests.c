#include "types.h"
#include "user.h"
#include "uthread.c"

void print_to_io(void * x)
{
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

  UT_exit();
}

void userThreadsFuction(){
  UT_Init();
  UT_Create( (void*)&print_to_io, (void*)0);
  UT_Create( (void*)&print_to_io, (void*)0);
  UT_yield();
  print_to_io((void*)0);
}