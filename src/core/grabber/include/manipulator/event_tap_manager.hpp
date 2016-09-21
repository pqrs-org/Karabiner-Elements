#pragma once

#include "logger.hpp"
#include "modifier_flag_manager.hpp"
#include <CoreGraphics/CoreGraphics.h>

namespace manipulator {
class event_tap_manager final {
public:
  event_tap_manager(const event_tap_manager&) = delete;

  event_tap_manager(modifier_flag_manager& modifier_flag_manager) : modifier_flag_manager_(modifier_flag_manager),
                                                                    event_tap_(nullptr),
                                                                    run_loop_source_(nullptr) {
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

    event_tap_ = CGEventTapCreate(kCGHIDEventTap,
                                  kCGHeadInsertEventTap,
                                  kCGEventTapOptionDefault,
                                  mask,
                                  event_tap_manager::static_callback,
                                  this);

    if (event_tap_) {
      run_loop_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_, 0);
      if (run_loop_source_) {
        CFRunLoopAddSource(CFRunLoopGetMain(), run_loop_source_, kCFRunLoopCommonModes);
        CGEventTapEnable(event_tap_, true);

        logger::get_logger().info("event_tap_manager grabbed mouse events");
      }
    }
  }

  ~event_tap_manager(void) {
    if (event_tap_) {
      CGEventTapEnable(event_tap_, false);

      if (run_loop_source_) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), run_loop_source_, kCFRunLoopCommonModes);
        CFRelease(run_loop_source_);
        run_loop_source_ = nullptr;
      }

      CFRelease(event_tap_);
      event_tap_ = nullptr;
    }

    logger::get_logger().info("event_tap_manager ungrabbed mouse events");
  }

private:
  static CGEventRef _Nullable static_callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event, void* _Nonnull refcon) {
    auto self = static_cast<event_tap_manager*>(refcon);
    if (self) {
      return self->callback(proxy, type, event);
    }
    return nullptr;
  }

  CGEventRef _Nullable callback(CGEventTapProxy _Nullable proxy, CGEventType type, CGEventRef _Nullable event) {
    if (event) {
      CGEventSetFlags(event, modifier_flag_manager_.get_cg_event_flags(CGEventGetFlags(event), krbn::key_code::vk_none));
    }
    return event;
  }

  const modifier_flag_manager& modifier_flag_manager_;

  CFMachPortRef _Nullable event_tap_;
  CFRunLoopSourceRef _Nullable run_loop_source_;
};
}
