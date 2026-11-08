#import <Foundation/Foundation.h>

#include "pqrs/osx/autorelease_pool/impl/impl.h"

// NSAutoreleasePool is unavailable under ARC.
// Manual reference counting is required in order to keep the pool alive across
// the C++ scoped_autorelease_pool constructor and destructor calls.
#if __has_feature(objc_arc)
#error "PQRSOSXAutoreleasePoolImpl.m must be compiled with -fno-objc-arc"
#endif

void* pqrs_osx_autorelease_pool_push(void) {
  return [[NSAutoreleasePool alloc] init];
}

void pqrs_osx_autorelease_pool_pop(void* token) {
  [(NSAutoreleasePool*)token drain];
}
