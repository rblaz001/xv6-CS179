#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

char buf[8192];
char name[3];
char *echoargv[] = { "echo", "ALL", "TESTS", "PASSED", 0 };
int stdout = 1;
int curr_fd = -1;

// does chdir() call iput(p->cwd) in a transaction?
void test_fnc(void* arg){
  printf(stdout, "in test_fnc\n");
  exit();
}

void open_and_write_to_file(){
    curr_fd = open("small", O_CREATE|O_RDWR);
    printf(stdout, "curr_fd after open file: %d \n", curr_fd);
    
    int i;
    for(i = 0; i < 100; i++){
    if(write(curr_fd, "aaaaaaaaaa", 10) != 10){
      printf(stdout, "error: write aa %d new file failed\n", i);
      exit();
    }
    if(write(curr_fd, "bbbbbbbbbb", 10) != 10){
      printf(stdout, "error: write bb %d new file failed\n", i);
      exit();
    }
  }
}

int read_open_file(void* arg){
  int i = read(curr_fd, buf, 900);

  printf(stdout, "i after read file: %d \n", i);

  if(i == 900){
    printf(stdout, "read succeeded ok\n");
  } else {
    printf(stdout, "read failed\n");
    exit();
  }

  exit();
}


void files_remain_open_test()
{
  printf(stdout, "in files_remain_open_test\n");

  open_and_write_to_file();

  // KT_Create((void*)&open_and_write_to_file, (void*)0);
  KT_Create((void*)&read_open_file, (void*)0);
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

int
main(int argc, char *argv[])
{
  printf(1, "testing kernel level threads!\n");

  create_n_threads(4);
  files_remain_open_test();

  printf(1, "ALL TESTS PASSED");
  exit();
}
