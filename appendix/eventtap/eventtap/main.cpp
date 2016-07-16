#include <CoreGraphics/CoreGraphics.h>
#include <iostream>

namespace {
CFMachPortRef eventtap_;

CGEventRef callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
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
}

int main(int argc, const char* argv[]) {
  eventtap_ = CGEventTapCreate(kCGHIDEventTap,
                               kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault,
                               CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
                               callback,
                               NULL);

  auto run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventtap_, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
  CGEventTapEnable(eventtap_, 1);
  CFRelease(run_loop_source);

  CFRunLoopRun();

  return 0;
}
