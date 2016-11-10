#include "GlobalLock.hpp"

IOLock* org_pqrs_driver_VirtualHIDManager_GlobalLock::lock_ = nullptr;

void org_pqrs_driver_VirtualHIDManager_GlobalLock::initialize(void) {
  lock_ = IOLockAlloc();
  if (!lock_) {
    IOLog("org_pqrs_driver_VirtualHIDManager_GlobalLock::IOLockAlloc failed.\n");
  }
}

void org_pqrs_driver_VirtualHIDManager_GlobalLock::terminate(void) {
  if (!lock_) return;

  IOLockLock(lock_);
  IOLock* tmp = lock_;
  lock_ = nullptr;
  IOLockUnlock(tmp);

  IOLockFree(tmp);
}

// ------------------------------------------------------------
org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock::ScopedLock(void) {
  lock_ = org_pqrs_driver_VirtualHIDManager_GlobalLock::lock_;
  if (!lock_) return;

  IOLockLock(lock_);
}

org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock::~ScopedLock(void) {
  if (!lock_) return;

  IOLockUnlock(lock_);
}

bool org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedLock::operator!(void)const {
  return lock_ == nullptr;
}

// ------------------------------------------------------------
org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedUnlock::ScopedUnlock(void) {
  lock_ = org_pqrs_driver_VirtualHIDManager_GlobalLock::lock_;
  if (!lock_) return;

  IOLockUnlock(lock_);
}

org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedUnlock::~ScopedUnlock(void) {
  if (!lock_) return;

  IOLockLock(lock_);
}

bool org_pqrs_driver_VirtualHIDManager_GlobalLock::ScopedUnlock::operator!(void)const {
  return lock_ == nullptr;
}
