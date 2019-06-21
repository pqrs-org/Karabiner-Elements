#include "logger.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <iostream>

namespace {
CFMachPortRef eventtap_;

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
  std::cout << "CGEventGetFlags 0x" << std::hex << CGEventGetFlags(event) << std::dec << std::endl;

  switch (type) {
    case kCGEventTapDisabledByTimeout:
      krbn::logger::get_logger()->info("Re-enable event_tap_ by kCGEventTapDisabledByTimeout");
      CGEventTapEnable(eventtap_, true);
      break;

    case kCGEventKeyDown:
      std::cout << "kCGEventKeyDown" << std::endl;
      break;

    case kCGEventKeyUp:
      std::cout << "kCGEventKeyUp" << std::endl;

    default:
      std::cout << "callback:" << type << std::endl;
      break;
  }
  return event;
}
} // namespace

int main(int argc, const char* argv[]) {
  if (geteuid() != 0) {
    krbn::logger::get_logger()->error("eventtap requires root privilege to use kCGHIDEventTap.");
    return 0;
  }

  if (auto source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState)) {
    std::cout << "CGEventSourceKeyboardType:" << CGEventSourceGetKeyboardType(source) << std::endl;
    CFRelease(source);
  }

  auto mask = CGEventMaskBit(kCGEventFlagsChanged) |
              CGEventMaskBit(kCGEventKeyDown) |
              CGEventMaskBit(kCGEventKeyUp) |
              CGEventMaskBit(kCGEventLeftMouseDown) |
              CGEventMaskBit(kCGEventLeftMouseUp) |
              CGEventMaskBit(kCGEventRightMouseDown) |
              CGEventMaskBit(kCGEventRightMouseUp) |
              CGEventMaskBit(kCGEventMouseMoved) |
              CGEventMaskBit(kCGEventLeftMouseDragged) |
              CGEventMaskBit(kCGEventRightMouseDragged) |
              CGEventMaskBit(kCGEventScrollWheel) |
              CGEventMaskBit(kCGEventOtherMouseDown) |
              CGEventMaskBit(kCGEventOtherMouseUp) |
              CGEventMaskBit(kCGEventOtherMouseDragged);

  eventtap_ = CGEventTapCreate(kCGHIDEventTap,
                               kCGTailAppendEventTap,
                               kCGEventTapOptionListenOnly,
                               mask,
                               callback,
                               nullptr);

  auto run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventtap_, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
  CGEventTapEnable(eventtap_, true);
  CFRelease(run_loop_source);

  CFRunLoopRun();

  return 0;
}
