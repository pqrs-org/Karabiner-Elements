#include <CoreGraphics/CoreGraphics.h>
#include <iostream>
#include <pqrs/cf/cf_ptr.hpp>

namespace {
pqrs::cf::cf_ptr<CFMachPortRef> event_tap;

CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy,
                              CGEventType type,
                              CGEventRef _Nullable event,
                              void* _Nonnull refcon) noexcept {
  if (type == kCGEventTapDisabledByTimeout) {
    CGEventTapEnable(event_tap.get(),
                     true);
    return event;
  }

  if (type == kCGEventRightMouseDown) {
    exit(0);
  }

  if (event) {
    CGEventSetFlags(event,
                    static_cast<CGEventFlags>(kCGEventFlagMaskNonCoalesced | kCGEventFlagMaskCommand));
  }
  return event;
}
} // namespace

int main() {
  // Observe all mouse events
  auto mask = CGEventMaskBit(kCGEventLeftMouseDown) |
              CGEventMaskBit(kCGEventLeftMouseUp) |
              CGEventMaskBit(kCGEventRightMouseDown) |
              CGEventMaskBit(kCGEventRightMouseUp) |
              CGEventMaskBit(kCGEventMouseMoved) |
              CGEventMaskBit(kCGEventLeftMouseDragged) |
              CGEventMaskBit(kCGEventRightMouseDragged) |
              CGEventMaskBit(kCGEventScrollWheel) |
              CGEventMaskBit(kCGEventTabletPointer) |
              CGEventMaskBit(kCGEventTabletProximity) |
              CGEventMaskBit(kCGEventOtherMouseDown) |
              CGEventMaskBit(kCGEventOtherMouseUp) |
              CGEventMaskBit(kCGEventOtherMouseDragged);

  event_tap = pqrs::cf::adopt_cf_ptr(CGEventTapCreate(kCGHIDEventTap,
                                                      kCGHeadInsertEventTap,
                                                      kCGEventTapOptionDefault,
                                                      mask,
                                                      callback,
                                                      nullptr));
  if (event_tap) {
    if (auto run_loop_source = pqrs::cf::adopt_cf_ptr(CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                                                    event_tap.get(),
                                                                                    0))) {
      CFRunLoopAddSource(CFRunLoopGetMain(),
                         run_loop_source.get(),
                         kCFRunLoopCommonModes);
      CGEventTapEnable(event_tap.get(),
                       true);

      std::cout << std::endl;
      std::cout << std::endl;
      std::cout << "Set mouse event flags to command." << std::endl;
      std::cout << "You can exit by right click." << std::endl;
      std::cout << std::endl;
      std::cout << std::endl;
    }
  }

  CFRunLoopRun();

  return 0;
}
