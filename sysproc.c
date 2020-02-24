#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_kt_create(void)
{
  char* fnc;
  char* arg;
  
  //Possible exploit by not having size of arg value
  if(argptr(0, &fnc, sizeof(int)) || argptr(1, &arg, 0) < 0)
    return -1;

  return KT_Create((void*)fnc, arg);
}

int
sys_kt_join(void)
{
  int tid;

  if(argint(0, &tid) < 0)
    return -1;

  return KT_Join(tid);
}

int
sys_sem_initialize(void)
{
  int initCount;

  if(argint(0, &initCount) < 0)
    return -1;

  if(initCount <= 0 && initCount >= 6)
    return -1;

  return sem_initialize(initCount);
}

int
sys_sem_wait(void)
{
  int index;

  if(argint(0, &index) < 0)
    return -1;

  if(index < 0 || index >= NPROC)
    return -1;
  
  return sem_wait(index);
}

int
sys_sem_signal(void)
{
  int index;

  if(argint(0, &index) < 0)
    return -1;

  if(index < 0 || index > NPROC)
    return -1;
  
  return sem_signal(index);
}

int
sys_sem_free(void)
{
  int index;

  if(argint(0, &index) < 0)
    return -1;

  if(index < 0 || index > NPROC)
    return -1;
  
  sem_free(index);
  return 0;
}