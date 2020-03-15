#include "types.h"
#include "user.h"

struct arguments;
struct extreme_arguments;

void criticalFuction(int* sem);
void frisbee(struct arguments * arg);
void extreme_frisbee(struct extreme_arguments * arg);
int numberOfThrows;

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

struct arguments {
  int player;
  int sem;
  int numPlayers;
  int numThrows;
  int * thrower;
  int * counter;
};


void frisbeeGame(int numPlayers, int numThrows){
  numberOfThrows = numThrows;
  int thrower = 0;
  int counter = 1;
  int sem = sem_initialize(1);

  if (!(numPlayers > 2 && numPlayers <= 7)){
    printf(1, "Need between 2 and 7 players to play frisbee\n");
    exit();
  }

  for (int i = 0; i < numPlayers; i++){
    struct arguments * arg = (struct arguments *) malloc(sizeof(struct arguments));
    arg->player = i;
    arg->sem = sem;
    arg->numPlayers = numPlayers;
    arg->numThrows = numThrows;
    arg->thrower = &thrower;
    arg->counter = &counter;
    KT_Create((void*)&frisbee, (void*)arg);
  }

  for(int i = 0; i < numPlayers; i++)
  {
    KT_Join();
  }

  exit();
}

void frisbee(struct arguments * arg){
  while (1) {

    sem_wait(arg->sem);

    if (arg->numThrows+1 == *arg->counter) {
      sem_signal(arg->sem);
      break;
    }

    if(*arg->thrower == arg->player){
      printf(1, "Pass number no: %d, Thread %d is passing the frisbee to thread %d \n\n ", 
        *arg->counter,
        arg->player,
        (arg->player + 1) % arg->numPlayers
      );

      (*arg->counter)++;
      *arg->thrower = (*arg->thrower + 1) % arg->numPlayers;
    }

    sem_signal(arg->sem);
  }

  exit();
}

struct frisbees {
    int thrower;
    int counter;
  };

struct extreme_arguments {
  int player;
  int sem;
  int sem_mutex;
  int numPlayers;
  int numThrows;
  int numFrisbees;
  struct frisbees * frisbees;
};

void extremeFrisbeeGame(int numPlayers, int numThrows, int numFrisbees){
  numberOfThrows = numThrows;
  int sem = sem_initialize(numFrisbees);
  int sem_mutex = sem_initialize(1);

  if (!(numPlayers > 2 && numPlayers <= 7 )){
    printf(1, "Need between 2 and 7 players to play frisbee\n");
    exit();
  }


  struct frisbees * frisbees = (struct frisbees *) malloc(sizeof(struct frisbees)*numFrisbees);
  for(int i =0; i < numFrisbees; i++){
    frisbees[i].counter = 1;
    frisbees[i].thrower= i;
  }

  for (int i = 0; i < numPlayers; i++){
    struct extreme_arguments * arg = (struct extreme_arguments *) malloc(sizeof(struct extreme_arguments));
    arg->player = i;
    arg->sem = sem;
    arg->sem_mutex = sem_mutex;
    arg->numPlayers = numPlayers;
    arg->numThrows = numThrows;
    arg->numFrisbees = numFrisbees;
    arg->frisbees = frisbees;
    KT_Create((void*)&extreme_frisbee, (void*)arg);
  }

  for(int i = 0; i < numPlayers; i++)
  {
    KT_Join();
  }

  exit();
}

int isEndOfGame(struct extreme_arguments * arg){
  for (int i =0; i < arg->numFrisbees; i++){
    if(arg->frisbees[i].counter != arg->numThrows + 1) return 0;
  }

  return 1;
}

void extreme_frisbee(struct extreme_arguments * arg){
  while (1) {

    sem_wait(arg->sem);

    if (isEndOfGame(arg)) {
      sem_signal(arg->sem);
      break;
    }

    for (int i = 0; i < arg->numFrisbees; i++){

      if(arg->frisbees[i].thrower == arg->player){
        sem_wait(arg->sem_mutex);
        printf(1, "Pass number no: %d for frisbee %d, Thread %d is passing the frisbee to thread %d \n\n ", 
          arg->frisbees[i].counter,
          i+1,
          arg->player,
          (arg->player + 1) % arg->numPlayers
        );
        sem_signal(arg->sem_mutex);

        sem_wait(arg->sem_mutex);
        if(arg->frisbees[i].counter != arg->numThrows + 1)
          arg->frisbees[i].counter++;
        sem_signal(arg->sem_mutex);
        arg->frisbees[i].thrower = (arg->frisbees[i].thrower + 1) % arg->numPlayers;
      }
    }

    sem_signal(arg->sem);
  }

  exit();
}