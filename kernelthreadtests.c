#include "types.h"
#include "user.h"

struct frisB_args;
struct extreme_frisB_args;
struct sem_args;

void criticalFuction(struct sem_args* sem);
void frisbee(struct frisB_args * arg);
void extreme_frisbee(struct extreme_frisB_args * arg);

// argument structure for criticalFunctions
struct sem_args{
  int sem;           // Offset used to reference semaphore
  int mutex;         // Offset used to reference semaphore
  int threadNum;     // Used to identify thread 
};

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

  int sem;
  int mutex;

  sem = sem_initialize(2);    // Initialize semaphore for 2 threads to enter critical section
  mutex = sem_initialize(1);  // Initialize semaphore for mutual exclusion

  if(sem == -1 || mutex == -1)
  {
    printf(stdout, "PANIC: Failed to initialize semaphore\n");
    return;
  }
  else
  {
    printf(stdout, "Semaphore Init\n");
  }
  
  // Generate 4 kernel level threads with same entry point
  for(int i = 0; i < 4; i++)
  {
    struct sem_args* args = (struct sem_args*) malloc(sizeof(struct sem_args));
    args->threadNum = i;
    args->mutex = mutex;
    args->sem = sem;
    KT_Create((void*)&criticalFuction, (void*)args);
  }

  // Waiting for all threads to finish
  for(int i = 0; i < 4; i++)
  {
    // Wrap print statemetns with mutex lock aquisition
    sem_wait(mutex);
    printf(stdout, "Waiting to Join thread %d\n", i);
    sem_signal(mutex);
    KT_Join();
  }
  sem_free(sem);
  sem_free(mutex);

  printf(stdout, "semaphoreTest ok\n");
}

// Critical section where stdout is used to periodically display status of thread
void criticalFuction(struct sem_args* args)
{
  int stdout = 1;
  sem_wait(args->mutex);
  printf(stdout, "Calling Sem Wait\n");
  sem_signal(args->mutex);
 
  sem_wait(args->sem);
  for(int i = 1; i <= 1000000; i++)
  {
    if(i % 10000 == 0)
    {
      sem_wait(args->mutex);
      printf(stdout, "%d\n", i/100);
      sem_signal(args->mutex);
    }

    if(i == 1000000)
    {
      sem_wait(args->mutex);
      printf(stdout, "Finished\n");
      sem_signal(args->mutex);
    }
  }
  sem_signal(args->sem);

  free(args);
  exit();
}

// argument structure for frisbee function
struct frisB_args {
  int player;           // Used to identify thread as particular player
  int sem;              // Offset used to reference semaphore
  int numPlayers;       // Total players playing the game
  int numThrows;        // Total number of throws needed before game ends
  int * thrower;        // Pointer to value indicating who's turn it is to throw frisbee
  int * counter;        // Number frisbee throws
};


// Objective of game is to have frisbee thrown numThrows amount of times
// The frisbee needs to be thrown in round robin
// Paramaters: int numPlayers, int numThrows
// numplayers: number of players that are playing frisbee
// numThrows: number of throws before the game ends
void frisbeeGame(int numPlayers, int numThrows){
  int thrower = 0;
  int counter = 1;
  int sem = sem_initialize(1);

  if (!(numPlayers > 2 && numPlayers <= 7)){
    printf(1, "Need between 2 and 7 players to play frisbee\n");
    exit();
  }

  for (int i = 0; i < numPlayers; i++){
    struct frisB_args * arg = (struct frisB_args *) malloc(sizeof(struct frisB_args));
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
}

// This is the function that contains the critical section
// In the critical section we check if it is our turn to throw the frisbee
// If it is, we declare our throw and throw the frisbee otherwise we just exit critical section
// Threads continue to repeat this process until the maximum number of throws is reached
void frisbee(struct frisB_args * arg){
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

  free(arg);
  exit();
}

struct frisbees {
    int thrower;    // Identifies who's turn it is to throw frisbee
    int counter;    // Number of frisbee throws
  };

struct extreme_frisB_args {
  int player;                     // Used to identify thread as particular player
  int sem;                        // Offset used to reference semaphore
  int sem_mutex;                  // Offset used to reference semaphore
  int numPlayers;                 // Total players playing the game
  int numThrows;                  // Number of throws needed before game ends. For each frisbee
  int numFrisbees;                // Number of frisbees in play 
  struct frisbees * frisbees;     // Pointer to structs holding status of frisbees
};


// Objective of game is to have each frisbee thrown numThrows amount of times
// The frisbees needs to be thrown in round robin
// Paramaters: int numPlayers, int numThrows, int numFrisbees
// numplayers: number of players that are playing frisbee
// numThrows: number of throws before the game ends
// numFrisbees: number of frisbees used in game
void extremeFrisbeeGame(int numPlayers, int numThrows, int numFrisbees){
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
    struct extreme_frisB_args * arg = (struct extreme_frisB_args *) malloc(sizeof(struct extreme_frisB_args));
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

  free(frisbees);
}

int isEndOfGame(struct extreme_frisB_args * arg){
  for (int i =0; i < arg->numFrisbees; i++){
    if(arg->frisbees[i].counter != arg->numThrows + 1) return 0;
  }

  return 1;
}

// This is the function that contains the critical section
// In the critical section we check if it is our turn to throw each frisbee
// If it is, we declare our throw and throw the frisbee otherwise we just exit critical section
// Threads continue to repeat this process until the maximum number of throws is reached
void extreme_frisbee(struct extreme_frisB_args * arg){
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

  free(arg);
  exit();
}