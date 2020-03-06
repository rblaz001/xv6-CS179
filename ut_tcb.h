#ifndef __ut_tcb__
#define __ut_tcb__

#include "types.h"

struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum tpcbstate { UNUSED, USED };

struct tpcb{
    int utid;
    void* sp[4096];
    struct context * context;
    enum tpcbstate state;
};

#endif /*ut_tcb.h*/