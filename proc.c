#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
// #include "semaphore.h"

struct queue {
    struct proc* q[MAXQ];
    int front;
    int back;
    int count;
};
 
struct semaphore {
    struct spinlock lock;
    struct queue queue;
    int count;
    enum procstate state;
};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];   // 8 threads possible per process
} ptable;

// process stack list table
struct {
  struct spinlock lock;
  struct psl psl[NPROC];
} sltable;

// semaphore table
struct {
  struct spinlock lock;
  struct semaphore sem[NPROC];
} semtable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&sltable.lock, "sltable");   // Initialize lock for sltable
  initlock(&semtable.lock, "semtable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

static struct proc*
tallocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  int slindex = 0;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  if(curproc->psl != 0)
    slindex = curproc->slindex;

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz, slindex)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;
  // Offset stack pointer for new process based off of old process stack location
  np->tf->esp += slindex*2*PGSIZE;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

int 
KT_Create(void (*fnc)(void*), void* arg)
{
  
  struct psl* sl;
  int slindex = -1; // Expected to be update to available index value in psl otherwise remains -1

  // Check for if a null pointer is passed as a fnc
  void *sp = 0;
  if (fnc == 0)
    return -1;

  struct proc *curproc = myproc();

  acquire(&sltable.lock);
  if (curproc->psl == 0){
    for(sl = sltable.psl; sl < &sltable.psl[NPROC]; sl++){
      if (sl->state != UNUSED)
        continue;
      
      sl->state = 1;
      curproc->psl = sl;
      // Setting the first stack position to occupied in our psl's stackz
      sl->stackz[0] = 1;
      curproc->slindex = 0;   // Initialize current thread stack list index 
      sl->thread_count = 2;   // Initialize Thread Count
      slindex = 1;            // Set stack list index for new stack to 1

    //allocate second thread's stack
    if ((sp = (void*)allocuvm(curproc->pgdir, STACKBOTTOM - 4*PGSIZE, STACKBOTTOM - 2*PGSIZE)) == 0)
      goto bad;

    clearpteu(curproc->pgdir, (char*)(sp - 4*PGSIZE)); // Clears 1 page table upward starting from pointer
    sl->stackz[1] = 1;
    break;
    }
  }
  else{
    if(curproc->psl->thread_count == 8){
      release(&sltable.lock);
      return -1;
    }

    // Increment Thread Count
    curproc->psl->thread_count++;

    // Allocating user stack
    for (int i = 0; i < 8; i++){
      if (curproc->psl->stackz[i] == 0){
        if ((sp = (void*)allocuvm(curproc->pgdir, STACKBOTTOM - 2*PGSIZE - i*2*PGSIZE, STACKBOTTOM - i*2*PGSIZE)) == 0)
          goto bad;

        clearpteu(curproc->pgdir, (char*)(sp - 2*PGSIZE)); // Clears 1 page table upward starting from pointer
        curproc->psl->stackz[i] = 1;  // Mark first unused stack list entry to 1
        slindex = i;

        break;
      }
    } 
  }
  release(&sltable.lock);

  return clone(sp, slindex, fnc, arg);
  
  bad:
    release(&sltable.lock);
    if(curproc->pgdir)
      freevm(curproc->pgdir);
    return -1;
}

int 
clone(void* sp, int slindex, void (*fnc)(void*), void* arg)
{
  struct proc* nt;
  struct proc* cur_thread = myproc();

   if((nt = tallocproc()) == 0){
    return -1;
  }

  nt->pthread = cur_thread;
  nt->psl = cur_thread->psl;
  nt->slindex = slindex;
  nt->pgdir = cur_thread->pgdir;
  nt->sz = cur_thread->sz;
  *nt->tf = *cur_thread->tf;

  for(int i = 0; i < NOFILE; i++){
    if(cur_thread->ofile[i])
      nt->ofile[i] = filedup(cur_thread->ofile[i]);
  }
  nt->cwd = idup(cur_thread->cwd);

  safestrcpy(nt->name, cur_thread->name, sizeof(cur_thread->name));

  nt->pid = cur_thread->pid;
  nt->tid = cur_thread->tid + 1;

  //Populating user stack with fake return and argument to passed function
  uint ustack[2];
  
  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = (uint)arg;         // pointer to argument that is passed to function
  sp -= 2*4;               // make room for ustack on the stack

  if(copyout(nt->pgdir, (uint)sp, ustack, 2*4) < 0)
    return -1;

  nt->tf->eip = (uint)fnc;  // set entry point to function pointer
  // Need to update the stack pointer to point to the top of the stack.
  nt->tf->esp = (uint)sp;   // set user stack 

  acquire(&ptable.lock);

  nt->state = RUNNABLE;

  release(&ptable.lock);

  return nt->tid;
}

void 
clear_psl(struct proc* curproc)
{
  // Free user stack memory and sltable entry
  acquire(&sltable.lock);

  curproc->psl->stackz[curproc->slindex] = 0;
  // Clear page table entry for current threads user stack
  // Note: Should replace STACKBOTTOM - 2*PGSIZE with
  //       #define xxxxx
  deallocuvm(curproc->pgdir, (STACKBOTTOM - curproc->slindex*2*PGSIZE), 
                              (STACKBOTTOM - 2*PGSIZE - curproc->slindex*2*PGSIZE));
  curproc->slindex = 0;

  curproc->psl->thread_count -= 1;

  if(curproc->psl->thread_count == 0)
    curproc->psl->state = UNUSED;

  release(&sltable.lock);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  // Will only close files if it is the last reference to it
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);
  
  if(curproc->psl != 0)
  {
    // Parent thread might be sleeping in KT_join().
    wakeup1(curproc->psl);
  }

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        if(p->psl == 0 || p->psl->thread_count == 0)
          freevm(p->pgdir);
        if(p->psl != 0)
          clear_psl(p);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->tid = 0;
        p->pthread = 0;
        p->psl = 0;
        p->slindex = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
sem_initialize(int initCount)
{
  acquire(&semtable.lock);
  for(int i = 0; i < NPROC; i++){
    if (semtable.sem[i].state != UNUSED)
      continue;

    sem_init(&semtable.sem[i], initCount);
    release(&semtable.lock);
    return i;
  }

  release(&semtable.lock);
  // No free semaphores
  return -1;
}

  //----------------------------------------------------------

int 
sem_signal(int index) 
{
  struct semaphore* s = &semtable.sem[index];

  if(s->state == UNUSED)
    return -1;

  acquire(&s->lock);
  
  s->count++;
  sem_wakeup(s);

  release(&s->lock);
  return 0;
}

int 
sem_wait(int index) 
{
  struct semaphore* s = &semtable.sem[index];

  if(s->state == UNUSED)
    return -1;

  acquire(&s->lock);
  if(s->count <= 0)
  {
    s->count--;
    sem_sleep(s);
  }
  else
  {
    s->count--;
  }
  release(&s->lock);

  return 0;
}

void 
sem_init(struct semaphore* s, int initCount) 
{
  initlock(&s->lock, "semaphore lock");
  s->count = initCount;
  s->queue.count = 0;
  s->queue.front = 0;
  s->queue.back = -1;
  s->state = 1;
  for(int i = 0; i < MAXQ; i++){
    s->queue.q[i] = 0;
  }
}

void 
sem_free(int index) 
{
  struct semaphore* s = &semtable.sem[index];

    acquire(&s->lock);
    s->state = UNUSED;
    release(&s->lock);
}

void 
sem_sleep(struct semaphore* s)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sem_sleep");

  acquire(&ptable.lock);  //DOC: sleeplock1

  // Go to sleep.
  push_back(&s->queue, p);
  release(&s->lock);

  p->state = SEM_SLEEPING;

  sched();

  // Reacquire semaphore lock.
  release(&ptable.lock);
  acquire(&s->lock);
}

void 
sem_wakeup(struct semaphore* s)
{
  struct proc* p;
  
  acquire(&ptable.lock);
  p = pop_front(&s->queue);
  if(p){
    p->state = RUNNABLE;
  }
  release(&ptable.lock);
}

// https://www.tutorialspoint.com/data_structures_algorithms/queue_program_in_c.htm
// Used this resource to help implement queue in c

int 
isEmpty(struct queue* que) 
{
  if(que->count == 0)
    return 1;
  else
    return 0;
}

int 
isFull(struct queue* que) 
{
  if(que->count == MAXQ)
    return 1;
  else
    return 0;
}

void 
push_back(struct queue* que, struct proc* p) 
{
  if(!isFull(que)){
      
    if(que->back == MAXQ-1){
      que->back = -1;
    }

    que->q[++que->back] = p;
    que->count++;
  }
}

struct 
proc* pop_front(struct queue* que) 
{
  if(isEmpty(que))
    return 0;
  struct proc* p = que->q[que->front++];

  if(que->front == MAXQ) {
    que->front = 0;
  }

  que->count--;
  return p;
}

int
KT_Join(void)
{
  struct proc *p;
  int havekids, tid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pthread != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        tid = p->tid;
        kfree(p->kstack);
        p->kstack = 0;
        if(p->psl == 0 || p->psl->thread_count == 0)
          freevm(p->pgdir);
        if(p->psl != 0)
          clear_psl(p);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->tid = 0;
        p->pthread = 0;
        p->psl = 0;
        p->slindex = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return tid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc->psl, &ptable.lock);  //DOC: wait-sleep
  }
}