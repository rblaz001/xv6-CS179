#include "defs.h"
#include "semaphore.h"
#include "spinlock.h"

void sem_signal(struct semaphore* s) {
    acquire(&s->lock);
    if(s->count <= 0)
        sem_sleep(s);
    s->count--;
    release(&s->lock);
}

void sem_wait(struct semaphore* s) {
    acquire(&s->lock);
    s->count++;
    sem_wakeup(s);
    release(&s->lock);
}

void sem_init(struct semaphore* s) {
    initlock(&s->lock, "semaphore lock");
    s->queue.count = 0;
    s->queue.front = 0;
    s->queue.back = -1;
    for(int i = 0; i < MAXQ; i++){
        s->queue.q[i] = 0;
    }
}

void sem_sleep(struct semamphore* s){

}

void sem_wakeup(struct semaphore* s){

}

// https://www.tutorialspoint.com/data_structures_algorithms/queue_program_in_c.htm
// Used this resource to help implement queue in c

int isEmpty(struct queue* que) {
    if(que->count == 0)
        return 1;
    else
        return 0;
}

int isFull(struct queue* que) {
    if(que->count == MAXQ)
        return 1;
    else
        return 0;
}

void push_back(struct queue* que, struct proc* p) {
    if(!isFull(que)){
        
        if(que->back == MAXQ-1){
            que->back = -1;
        }

        que->q[++que->back] = p;
        que->count++;
    }
}

struct proc* pop_front(struct queue* que) {
    if(isEmpty(que))
        return 0;
    struct proc* p = que->q[que->front++];
	
    if(que->front == MAXQ) {
       que->front = 0;
    }
	
    que->count--;
    return p;
}