#ifndef __UTHREAD__H
#define __UTHREAD__H

int UT_Init(void);
int UT_Create(void (*fnc)(void*), void* arg);
void increment_sched_index(void);
void UT_Scheduler(void);
void UT_yield(void);
int UT_shutdown(void);
void root_exit(void);
void UT_exit(void);
void UT_free(void);

#endif /* uthread.h */