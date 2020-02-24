#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

char buf[21];
char name[3];
char *echoargv[] = { "echo", "ALL", "TESTS", "PASSED", 0 };
int stdout = 1;
int curr_fd = -1;

// does chdir() call iput(p->cwd) in a transaction?
void test_fnc(void* arg){
  printf(stdout, "in test_fnc\n");
  exit();
}

int read_open_file(int* arg){
  while((int volatile)*arg != 1){}
  open("small", 0);
  int i = read(curr_fd, buf, 21);

  printf(stdout, "i after read file: %d \n", i);

  if(i == 11){
    printf(stdout, "read succeeded ok: \n", buf);
  } else {
    printf(stdout, "read failed\n");
    exit();
  }
  exit();
}

void open_and_write_to_file(int* arg){
    curr_fd = open("small", O_CREATE|O_RDWR);
    printf(stdout, "curr_fd after open file: %d \n", curr_fd);
    printf(stdout, "ARG VALUE: %d \n", 0);
    
    if(write(curr_fd, "aaaaaaaaaa", 11) != 11){
      printf(stdout, "error: write aa new file failed\n");
      exit();
    }
  printf(stdout, "AFTER OPEN_WRITE \n");
  *arg += 1;
}

void write_to_file(int* arg)
{
  printf(stdout, "curr_fd after open file: %d \n", curr_fd);
  if(write(curr_fd, "bbbbbbbbbb", 11) != 11){
      printf(stdout, "error: write aa new file failed\n");
      exit();
    }
  printf(stdout, "AFTER WRITE \n");
  *arg += 1;
  exit();
}

int dummy_fnc(int * x){
  return 1;
}

void files_remain_open_test()
{
  printf(stdout, "in files_remain_open_test\n");

  int volatile var = 0;
  int volatile * x = &var;

  open_and_write_to_file((void* )x);

  // KT_Create((void*)&write_to_file, (void* )x);
  KT_Create((void*)&read_open_file, (void* )x);
  printf(stdout, "x VALUE: %d \n", *x);
}

void create_n_threads(int num_threads)
{
  printf(stdout, "creating threads test\n");
  int pid = -1;

  for(int i = 0; i < num_threads; i++){
    if(pid != 0){
      pid = KT_Create((void*)&test_fnc, (void*)0);
    }
  }

  if(pid != 0){
    // This is the parent process
    printf(stdout, "creaton of %d threads passed\n", num_threads);
  }
}

void criticalFuction(int* sem)
{
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
}

int
main(int argc, char *argv[])
{
  printf(1, "testing kernel level threads!\n");

  // create_n_threads(4);
  // files_remain_open_test();
  semaphoreTest();

  printf(1, "ALL TESTS PASSED");
  exit();
}
