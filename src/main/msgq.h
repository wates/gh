#ifndef MSGQ_H_INCLUDED
#define MSGQ_H_INCLUDED

#include "pipe.h"

namespace gh {
  struct MessageQueue
    :public Simplex
  {
    virtual void Fetch() = 0;
    virtual bool Lock(uint8_t** buffer, int length) = 0;
    virtual void Unlock() = 0;

    static MessageQueue* Create();
  };
}

#endif
