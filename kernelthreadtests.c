#include "types.h"
#include "user.h"

void criticalFuction(int* sem);

// This is a test for kernel level thread synchronization using semaphores.
// Generate 4 user threads that are set to enter a critical function.
// The objective of this test is to have threads scheduled to enter the critical
// section while other threads are scheduled to wait.
// This is achieved by looping a large amount of times to allow time for threads
// to announce that they are waiting.
void semaphoreTest()
{
  int stdout = 1;
  printf(stdout, "Starting semaphoreTest\n");

  int sem = sem_initialize(2); // Initialize semaphore for 2 threads to enter critical section

  if(sem == -1)
  {
    printf(stdout, "PANIC\n");
  }
  else
  {
    printf(stdout, "Semaphore Init\n");
  }
  
  // Generate 4 kernel level threads with same entry point
  for(int i = 0; i < 4; i++)
  {
    KT_Create((void*)&criticalFuction, (void*)&sem);
  }

  // Waiting for all threads to finish
  for(int i = 0; i < 4; i++)
  {
    printf(stdout, "Waiting to Join thread %d\n", i);
    KT_Join();
  }
  sem_free(sem);    // Freeing lock

  printf(stdout, "semaphoreTest ok\n");
}

// Critical section where stdout is used to periodically display status of thread
void criticalFuction(int* sem)
{
  int stdout = 1;
  printf(stdout, "Calling Sem Wait");

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