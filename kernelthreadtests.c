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

// does chdir() call iput(p->cwd) in a transaction?
void test_fnc(void* arg){
  exit();
}
void create_n_threds(int num_threads)
{
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

  create_n_threds(4);

  printf(1, "ALL TESTS PASSED");
  exit();
}
