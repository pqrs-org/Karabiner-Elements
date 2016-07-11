#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <iostream>

#include "event_grabber.hpp"

int main(int argc, const char* argv[]) {
  event_grabber observer;
  CFRunLoopRun();
  return 0;
}
