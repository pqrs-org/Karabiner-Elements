#pragma once

// pqrs::osx::cg_event_tap v1.0.0

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <pqrs/cf/cf_ptr.hpp>
#include <utility>

namespace pqrs::osx {
class cg_event_tap final {
public:
  cg_event_tap(const cg_event_tap&) = delete;
  cg_event_tap& operator=(const cg_event_tap&) = delete;

  explicit cg_event_tap(pqrs::cf::cf_ptr<CFMachPortRef>&& port)
      : port_(std::move(port)) {
  }

  ~cg_event_tap() {
    invalidate();
  }

  [[nodiscard]] bool valid() const {
    return static_cast<bool>(port_);
  }

  [[nodiscard]] bool attach_to_run_loop(pqrs::cf::cf_ptr<CFRunLoopRef> run_loop) {
    if (!port_ ||
        !run_loop ||
        run_loop_source_) {
      return false;
    }

    auto source = pqrs::cf::adopt_cf_ptr(CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                                       port_.get(),
                                                                       0));
    if (!source) {
      return false;
    }

    CFRunLoopAddSource(run_loop.get(),
                       source.get(),
                       kCFRunLoopCommonModes);

    run_loop_ = std::move(run_loop);
    run_loop_source_ = std::move(source);

    return true;
  }

  [[nodiscard]] bool set_enabled(bool enabled) const {
    if (!port_) {
      return false;
    }

    CGEventTapEnable(port_.get(), enabled);
    return CGEventTapIsEnabled(port_.get()) == enabled;
  }

  void invalidate() {
    if (!port_) {
      return;
    }

    if (run_loop_ &&
        run_loop_source_) {
      CFRunLoopRemoveSource(run_loop_.get(),
                            run_loop_source_.get(),
                            kCFRunLoopCommonModes);
    }

    // Invalidating the CFMachPort explicitly tears down the EventTap connection.
    // It must be done before releasing the run loop source and the port.
    CFMachPortInvalidate(port_.get());

    run_loop_source_ = nullptr;
    run_loop_ = nullptr;
    port_ = nullptr;
  }

private:
  pqrs::cf::cf_ptr<CFMachPortRef> port_;
  pqrs::cf::cf_ptr<CFRunLoopRef> run_loop_;
  pqrs::cf::cf_ptr<CFRunLoopSourceRef> run_loop_source_;
};
} // namespace pqrs::osx
