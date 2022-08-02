#pragma once
#include <ch.h>
#include <hal.h>
#include "rfm69.hpp"


namespace ModeTest {
  struct Report {
    bool timeout = true;
    bool hasReceivedFrame = false;
    uint32_t nbError = 0;
    MUTEX_DECL(mtx);
    
    void lock(void) {chMtxLock(&mtx);}
    void unlock(void) {chMtxUnlock(&mtx);}
    void reset(void) {
      lock();
      timeout = true;
      hasReceivedFrame = false;
      nbError = 0;
      unlock();
    }

  };
  void start(RfMode rfMode, uint32_t baud);
  Report getReport();
}
