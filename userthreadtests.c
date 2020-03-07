#include "types.h"
#include "user.h"
#include "uthread.c"

void print_to_io(void * x)
{
  int stdout = 1;

  for(int i = 1; i <= 10; i++)
  {
      UT_yield();
      printf(stdout, "%d\n", i);

    if(i == 10){
      UT_yield();
      printf(stdout, "Finished\n");
    }
  }

  UT_exit();
}

void userThreadsFuction(){
    int stdout = 1;
  UT_Init();
  printf(stdout, "after init\n");
  UT_Create( (void*)&print_to_io, (void*)0);
  printf(stdout, "after first ut\n");
  UT_Create( (void*)&print_to_io, (void*)0);
  printf(stdout, "after second ut\n");
  UT_yield();
  printf(stdout, "after yield ut\n");
  print_to_io((void*)0);
}