#pragma once

#include "DiagnosticMacros.hpp"

BEGIN_IOKIT_INCLUDE;
#include <IOKit/IOLib.h>
END_IOKIT_INCLUDE;

class org_pqrs_driver_VirtualHIDManager_GlobalLock final {
public:
  static void initialize(void);
  static void terminate(void);

  class ScopedLock final {
  public:
    ScopedLock(void);
    ~ScopedLock(void);
    bool operator!(void)const;

  private:
    IOLock* lock_;
  };

  class ScopedUnlock final {
  public:
    ScopedUnlock(void);
    ~ScopedUnlock(void);
    bool operator!(void)const;

  private:
    IOLock* lock_;
  };

private:
  static IOLock* lock_;
};
