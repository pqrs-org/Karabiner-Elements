#include "thread_utility.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <iostream>

namespace {
CFMachPortRef _Nullable event_tap;

CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event, void* _Nonnull refcon) {
  if (type == kCGEventTapDisabledByTimeout) {
    CGEventTapEnable(event_tap, true);
    return event;
  }

  if (type == kCGEventRightMouseDown) {
    exit(0);
  }

  if (event) {
    CGEventSetFlags(event, static_cast<CGEventFlags>(kCGEventFlagMaskNonCoalesced | kCGEventFlagMaskCommand));
  }
  return event;
}
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

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

  event_tap = CGEventTapCreate(kCGHIDEventTap,
                               kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault,
                               mask,
                               callback,
                               nullptr);
  if (event_tap) {
    if (auto run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0)) {
      CFRunLoopAddSource(CFRunLoopGetMain(), run_loop_source, kCFRunLoopCommonModes);
      CGEventTapEnable(event_tap, true);

      std::cout << std::endl;
      std::cout << std::endl;
      std::cout << "Set mouse event flags to command." << std::endl;
      std::cout << "You can exit by right click." << std::endl;
      std::cout << std::endl;
      std::cout << std::endl;

      CFRelease(run_loop_source);
    }
  }

  CFRunLoopRun();

  return 0;
}
