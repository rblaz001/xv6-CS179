// #include "spinlock.h"
// #include "proc.h"

// #define MAXQ 7

// struct queue {
//     struct proc* q[MAXQ];
//     int front;
//     int back;
//     int count;
// };
 
// struct semaphore {
//     struct spinlock lock;
//     struct queue queue;
//     int count;
//     enum proc state;
// };