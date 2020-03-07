#ifndef __uthread__
#define __uthread__
#include  "ut_tcb.h"
#include  "user.h"


#define UT_COUNT 8
const int PGSIZE = 4096;
int tid = 0;

void swtch(struct context**, struct context*);

struct table {
  struct tpcb tpcb[UT_COUNT];
}*utable;

int sched_index = 1;

int
UT_Init(){
  utable = (struct table*)malloc(sizeof(struct table));
  struct tpcb * t;
  for (t = utable->tpcb; t < &utable->tpcb[UT_COUNT]; t++){
    t->state = UNUSED;
    t->utid = -1;
  }
  if(utable == 0) return -1;

  t = &utable->tpcb[0];
  t->state = USED;
  t->utid = tid++;

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
    uint ustack[2];
    ustack[0] = 0xffffffff;  // fake return PC
    ustack[1] = (uint)arg;         // pointer to argument that is passed to function
    void * sp = t->sp + PGSIZE-1;
    sp -= 3*4;
    memmove(sp+4, ustack, 2*4);

    sp -= sizeof(struct context);
    t->context = (struct context *)sp;
    memset(t->context, 0, sizeof *t->context);
    t->context->eip = (uint)fnc;
    t->context->ebp = (uint)&t->context->eip + 4;
    return t->utid;
  }

  return -1;
}

void
increment_sched_index(){
  if(sched_index == UT_COUNT - 1){
    sched_index = 0;
  }
  else{
    sched_index++;
  }
}

void
UT_Scheduler(void){
  struct tpcb * t = &utable->tpcb[sched_index];
  struct tpcb * curr_tpcb;
  if(sched_index == 0){
    curr_tpcb = &utable->tpcb[UT_COUNT - 1];
  }
  else{
    curr_tpcb = &utable->tpcb[sched_index - 1];
  }

  while( 1 ){
    if (t->state == USED){
      increment_sched_index();
      break;
    }
    increment_sched_index();
    t = &utable->tpcb[sched_index];
  }
  swtch(&curr_tpcb->context, t->context);
}

void
UT_exit(void){
  struct tpcb * curr_tpcb;
  struct tpcb * t = &utable->tpcb[sched_index];
  if(sched_index == 0){
      curr_tpcb = &utable->tpcb[UT_COUNT - 1];
    }
  else{
      curr_tpcb = &utable->tpcb[sched_index - 1];
  }
  curr_tpcb->state = UNUSED;

  swtch(&curr_tpcb->context, t->context);
}

void
UT_yield(void){
  UT_Scheduler();
}

void
UT_free()
{
  free(utable);
}

#endif /*__uthread__*/