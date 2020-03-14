#ifndef __ut_tcb__
#define __ut_tcb__

#include "types.h"

// Saved registers for user stack context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across user contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum tpcbstate { UNUSED, USED };

struct tpcb{
    int utid;                   // user thread ID corresponding to user thread
    void* sp[4096];             // stack pointer with the size of a single page
    struct context * context;   // pointer to uster stack context structure
    enum tpcbstate state;       // state that declares if thread pcb is used or unused
};

#endif /*ut_tcb.h*/