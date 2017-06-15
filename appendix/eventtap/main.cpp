#include "../include/logger.hpp"
#include "thread_utility.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <iostream>
#include <spdlog/spdlog.h>

namespace {
CFMachPortRef eventtap_;

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
  std::cout << "CGEventGetFlags 0x" << std::hex << CGEventGetFlags(event) << std::dec << std::endl;

  switch (type) {
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
  krbn::thread_utility::register_main_thread();

  if (getuid() != 0) {
    logger::get_logger().error("eventtap requires root privilege to use kCGHIDEventTap.");
    return 0;
  }

  if (auto source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState)) {
    std::cout << "CGEventSourceKeyboardType:" << CGEventSourceGetKeyboardType(source) << std::endl;
    CFRelease(source);
  }

  eventtap_ = CGEventTapCreate(kCGHIDEventTap,
                               kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault,
                               CGEventMaskBit(kCGEventLeftMouseDown) |
                                   CGEventMaskBit(kCGEventLeftMouseUp) |
                                   CGEventMaskBit(kCGEventRightMouseDown) |
                                   CGEventMaskBit(kCGEventRightMouseUp) |
                                   CGEventMaskBit(kCGEventMouseMoved) |
                                   CGEventMaskBit(kCGEventLeftMouseDragged) |
                                   CGEventMaskBit(kCGEventRightMouseDragged) |
                                   CGEventMaskBit(kCGEventKeyDown) |
                                   CGEventMaskBit(kCGEventKeyUp),
                               callback,
                               nullptr);

  auto run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventtap_, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
  CGEventTapEnable(eventtap_, true);
  CFRelease(run_loop_source);

  CFRunLoopRun();

  return 0;
}
