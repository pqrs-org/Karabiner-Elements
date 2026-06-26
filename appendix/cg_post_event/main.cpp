#include <CoreGraphics/CoreGraphics.h>
#include <chrono>
#include <pqrs/cf/cf_ptr.hpp>
#include <thread>

int main(int argc, const char* argv[]) {
  if (auto source = pqrs::cf::adopt_cf_ptr(CGEventSourceCreate(kCGEventSourceStateHIDSystemState))) {
    while (true) {
      // shift key
      {
        if (auto ev = pqrs::cf::adopt_cf_ptr(CGEventCreateKeyboardEvent(source.get(),
                                                                        (CGKeyCode)56,
                                                                        true))) {
          CGEventSetFlags(ev.get(),
                          static_cast<CGEventFlags>(kCGEventFlagMaskShift | CGEventGetFlags(ev.get())));
          CGEventPost(kCGHIDEventTap,
                      ev.get());
        }
      }

      // We have to put wait between continous CGEventPost.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // z key
      {
        if (auto ev = pqrs::cf::adopt_cf_ptr(CGEventCreateKeyboardEvent(source.get(),
                                                                        (CGKeyCode)6,
                                                                        true))) {
          CGEventSetFlags(ev.get(),
                          static_cast<CGEventFlags>(kCGEventFlagMaskShift | CGEventGetFlags(ev.get())));
          CGEventPost(kCGHIDEventTap,
                      ev.get());
        }
      }

      // We have to put wait between continous CGEventPost.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      {
        if (auto ev = pqrs::cf::adopt_cf_ptr(CGEventCreateKeyboardEvent(source.get(),
                                                                        (CGKeyCode)6,
                                                                        false))) {
          CGEventSetFlags(ev.get(),
                          static_cast<CGEventFlags>(kCGEventFlagMaskShift | CGEventGetFlags(ev.get())));
          CGEventPost(kCGHIDEventTap,
                      ev.get());
        }
      }

      // We have to put wait between continous CGEventPost.
      std::this_thread::sleep_for(std::chrono::seconds(3));

      // shift key
      {
        if (auto ev = pqrs::cf::adopt_cf_ptr(CGEventCreateKeyboardEvent(source.get(),
                                                                        (CGKeyCode)56,
                                                                        false))) {
          CGEventPost(kCGHIDEventTap,
                      ev.get());
        }
      }

      // We have to put wait between continous CGEventPost.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

#if 0
    // comment out for -Wunreachable-code
#endif
  }

  return 0;
}
