#ifndef __uthread__
#define __uthread__
#include  "ut_tcb.h"
#include  "user.h"
#include  "uthread.h"

#define UT_COUNT 8
const int PGSIZE = 4096;
int tid = 0;

void swtch(struct context**, struct context*);

// This struct is a table of all userthread process control blocks.
struct table {
  struct tpcb tpcb[UT_COUNT];
}*utable;

int sched_index = 1;
int uthread_count = 0;

// Dynamically create memory for utable and initialize all entries to default values.
// Initialize the first entry to the current userthread.
int
UT_Init(){
  utable = (struct table*)malloc(sizeof(struct table));
  if(utable == 0) return -1;
  struct tpcb * t;

  for (t = utable->tpcb; t < &utable->tpcb[UT_COUNT]; t++){
    t->state = UNUSED;
    t->utid = -1;
  }

  t = &utable->tpcb[0];
  t->state = USED;
  t->utid = tid++;
  uthread_count = 1;
  sched_index = 1;

  return 0;  
}

// Traverse through user thread table to find an UNUSED entry.
// Initializing the user stack (which has already been allocated) 
// with provided function pointer and arg pointer
// Paramaters: void (*fnc)(void*), void* arg
// fnc: pointer to a function. This will be used as the entry point for the newly generated thread
// arg: argument that will be passed to function fnc is pointing to. Is initialized on user stack
int
UT_Create(void (*fnc)(void*), void* arg){
  struct tpcb * t;

  for (t = utable->tpcb; t < &utable->tpcb[UT_COUNT]; t++){
    if (t->state != UNUSED)
      continue;
    t->state = USED;
    t->utid = tid++;

    //Populating user stack with fake return and argument(s) to passed function
    uint ustack[2];
    ustack[0] = 0xffffffff;         // fake return PC
    ustack[1] = (uint)arg;          // pointer to argument that is passed to function
    void * sp = t->sp + PGSIZE-1;  
    sp -= 2*4;                      // make room for ustack on the stack
    memmove(sp, ustack, 2*4);       // Copying ustack array onto user thread stack

    sp -= sizeof(struct context);                   // making room for context struct on the stack
    t->context = (struct context *)sp;              // assign context to our current stack pointer
    memset(t->context, 0, sizeof *t->context);      // sets all the values in the context to 0
    t->context->eip = (uint)fnc;                    // setting instruciton ptr to fnc entry point
    t->context->ebp = (uint)&t->context->eip + 4;   // setting ebp to value after popping eip off the stack
    uthread_count++;
    return t->utid;
  }

  return -1;
}

// Helper function used to increment the index of the scheduler
// If index hits the the end of the utable, we wrap around to
// the beginning of the utable
void
increment_sched_index(){
  if(sched_index == UT_COUNT - 1){
    sched_index = 0;
  }
  else{
    sched_index++;
  }
}

// Schedules userthreads in a round robin priority
// A global index is used to manage round robin traversal of userthreads
// If we find a userthread, we switch the context to this next userthread. 
void
UT_Scheduler(void){
  if(uthread_count == 1 && sched_index == 1)
    return;
  struct tpcb * t = &utable->tpcb[sched_index];
  struct tpcb * curr_tpcb;
  if(sched_index == 0){
    curr_tpcb = &utable->tpcb[UT_COUNT - 1];
  }
  else{
    curr_tpcb = &utable->tpcb[sched_index - 1];
  }

  // Run through utable, until a userthread is found
  // userthread is guaranteed to be found
  // At very least, it will schedule itself.  
  while( 1 ){
    if (t->state == USED){
      increment_sched_index();
      break;
    }
    increment_sched_index();
    t = &utable->tpcb[sched_index];
  }
  // Calling assemby function to switch context
  // At completion of this call, we will be on the 
  // userstack of the scheduled userthread.
  swtch(&curr_tpcb->context, t->context);
}

// In order to yield, we simply call scheduler
// which handles the context switching between 
// userthreads.
void
UT_yield(void){
  UT_Scheduler();
}

// Check if current user thread is the root user thread.
// If so we yield the processor until all other userthreads have exited
// then we free the user table and return to normal execution.
int
UT_shutdown(void){
  if(sched_index == 1){
    while(uthread_count != 1){
      UT_yield();
    }
    free(utable);
    return 0;
  }
  return -1;
}

// First checks if we are the root thread.
// If so, we wait until all other userthreads have exited
// before cleaing up the utable.
// Else, the current userthread is set to UNUSED and the
// scheduler is called.
void
UT_exit(void){
  if (UT_shutdown() == 0)
    return;
  struct tpcb * curr_tpcb;
  if(sched_index == 0){
      curr_tpcb = &utable->tpcb[UT_COUNT - 1];
    }
  else{
      curr_tpcb = &utable->tpcb[sched_index - 1];
  }
  curr_tpcb->state = UNUSED;
  uthread_count--;

  UT_Scheduler();
}

#endif /*__uthread__*/