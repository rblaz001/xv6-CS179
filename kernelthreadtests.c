#include "types.h"
#include "user.h"

void criticalFuction(int* sem)
{
  int stdout = 1;
  if(sem_wait(*sem) == -1)
  {
    printf(stdout, "panic\n");
    exit(); 
  } 
  for(int i = 1; i <= 1000000; i++)
  {
    if(i % 10000 == 0)
      printf(stdout, "%d\n", i/100);

    if(i == 1000000)
      printf(stdout, "Finished\n");
  }
  sem_signal(*sem);

  exit();
}

void semaphoreTest()
{
  int stdout = 1;
  int sem = sem_initialize(1);

  if(sem == -1)
  {
    printf(stdout, "PANIC\n");
  }
  else
  {
    printf(stdout, "Semaphore Init\n");
  }
  
  for(int i = 0; i < 4; i++)
  {
    KT_Create((void*)&criticalFuction, (void*)&sem);
  }

  for(int i = 0; i < 4; i++)
  {
    printf(stdout, "Waiting to Join thread %d\n", i);
    KT_Join();
  }
  sem_free(sem);
}