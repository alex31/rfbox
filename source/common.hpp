#pragma once

#include "ch.h"

class Lock
{
public:
  Lock(mutex_t &_mtx) : mtx(_mtx) {chMtxLock(&mtx);};
  ~Lock() {chMtxUnlock(&mtx);};
private:
  mutex_t &mtx;
};
