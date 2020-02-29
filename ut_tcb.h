#ifndef __ut_tcb__
#define __ut_tcb__

#include "types.h"

struct context{
    uint esp;
    uint ebp;
    uint eip;
};

enum tpcbstate { UNUSED, USED };

struct tpcb{
    int utid;
    void* stack;
    enum tpcbstate state;
};

#endif /*ut_tcb.h*/