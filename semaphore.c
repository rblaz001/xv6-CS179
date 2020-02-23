// #include "defs.h"
// #include "semaphore.h"
// #include "spinlock.h"
// #include "proc.c"

// void sem_signal(struct semaphore* s) {
//     acquire(&s->lock);
//     if(s->count <= 0)
//         sem_sleep(s);
//     s->count--;
//     release(&s->lock);
// }

// void sem_wait(struct semaphore* s) {
//     acquire(&s->lock);
//     s->count++;
//     sem_wakeup(s);
//     release(&s->lock);
// }

// void sem_init(struct semaphore* s) {
//     initlock(&s->lock, "semaphore lock");
//     s->queue.count = 0;
//     s->queue.front = 0;
//     s->queue.back = -1;
//     for(int i = 0; i < MAXQ; i++){
//         s->queue.q[i] = 0;
//     }
// }

// void sem_free(struct semaphore* s) {
//     acquire(&s->lock);
//     s->state = UNUSED;
//     release(&s->lock);
// }

// void sem_sleep(struct semaphore* s){
//     struct proc *p = myproc();
  
//     if(p == 0)
//         panic("sem_sleep");

//     acquire(&ptable.lock);  //DOC: sleeplock1
//     release(&s->lock);

//     // Go to sleep.
//     push_back(&s->queue, p);
//     p->state = SEM_SLEEPING;

//     sched();

//     // Reacquire semaphore lock.
//     release(&ptable.lock);
//     acquire(&s->lock);
// }

// void sem_wakeup(struct semaphore* s){
//     struct proc* p;
    
//     acquire(&ptable.lock);
//     p = pop_front(&s->queue);
//     if(p){
//         p->state = RUNNABLE;
//     }
//     release(&ptable.lock);
// }

// // https://www.tutorialspoint.com/data_structures_algorithms/queue_program_in_c.htm
// // Used this resource to help implement queue in c

// int isEmpty(struct queue* que) {
//     if(que->count == 0)
//         return 1;
//     else
//         return 0;
// }

// int isFull(struct queue* que) {
//     if(que->count == MAXQ)
//         return 1;
//     else
//         return 0;
// }

// void push_back(struct queue* que, struct proc* p) {
//     if(!isFull(que)){
        
//         if(que->back == MAXQ-1){
//             que->back = -1;
//         }

//         que->q[++que->back] = p;
//         que->count++;
//     }
// }

// struct proc* pop_front(struct queue* que) {
//     if(isEmpty(que))
//         return 0;
//     struct proc* p = que->q[que->front++];
	
//     if(que->front == MAXQ) {
//        que->front = 0;
//     }
	
//     que->count--;
//     return p;
// }