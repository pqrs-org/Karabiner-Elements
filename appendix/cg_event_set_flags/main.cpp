#include <CoreGraphics/CoreGraphics.h>
#include <iostream>
#include <memory>
#include <pqrs/osx/cg_event_tap.hpp>

namespace {
std::unique_ptr<pqrs::osx::cg_event_tap> event_tap;

CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy,
                              CGEventType type,
                              CGEventRef _Nullable event,
                              void* _Nonnull refcon) noexcept {
  if (type == kCGEventTapDisabledByTimeout) {
    if (event_tap &&
        !event_tap->enable()) {
      std::cerr << "Failed to re-enable event tap." << std::endl;
    }
    return event;
  }

  if (type == kCGEventRightMouseDown) {
    CFRunLoopStop(CFRunLoopGetMain());
    return event;
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

  event_tap = std::make_unique<pqrs::osx::cg_event_tap>(
      pqrs::cf::adopt_cf_ptr(CGEventTapCreate(kCGHIDEventTap,
                                              kCGHeadInsertEventTap,
                                              kCGEventTapOptionDefault,
                                              mask,
                                              callback,
                                              nullptr)));
  if (event_tap->valid() &&
      event_tap->attach_to_run_loop(pqrs::cf::cf_ptr(CFRunLoopGetMain())) &&
      event_tap->enable()) {
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "Set mouse event flags to command." << std::endl;
    std::cout << "You can exit by right click." << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
  }

  CFRunLoopRun();

  return 0;
}
