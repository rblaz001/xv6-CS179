#ifndef __uthread__
#define __uthread__
#include  "ut_tcb.h"
#include  "user.h"

const int UT_COUNT =  8;
const int PGSIZE = 4096;
int tid = 0;

struct table {
  struct tpcb tpcb[UT_COUNT];
};

struct table * utable;

int
UT_Init(){
  utable = (struct table *)malloc(sizeof(struct table));
  struct tpcb * t;
  for (t = utable->tpcb; t < &utable->tpcb[UT_COUNT]; t++){
    t->state = UNUSED;
    t->utid = -1;
  }
  if(utable == 0) return -1;
  return 0;  
}

int
UT_Create(void (*fnc)(void*), void* arg){
  struct tpcb * t;
  for (t = utable->tpcb; t < &utable->tpcb[UT_COUNT]; t++){
    if (t->state != UNUSED)
      continue;
    t->state = USED;
    t->utid = tid++;
    t->mallocptr = malloc(PGSIZE);
    void * sp = t->mallocptr + PGSIZE - 1;
    uint ustack[2];
    ustack[0] = 0xffffffff;  // fake return PC
    ustack[1] = (uint)arg;         // pointer to argument that is passed to function
    sp -= 3*4;

    sp -= sizeof(struct context);
    t->context = (struct context *)sp;
    memset(t->context, 0, sizeof *t->context);
    t->context->eip = (uint)fnc;
    t->context->ebp = (uint)&t->context->eip + 4;
    return t->utid;
  }

  return -1;
}

int
UT_exit(void){
  return -1;
}

int
UT_yield(void){
  return -1;
}

void
UT_Scheduler(void){
}

int
UT_Free(){

}

#endif /*__uthread__*/