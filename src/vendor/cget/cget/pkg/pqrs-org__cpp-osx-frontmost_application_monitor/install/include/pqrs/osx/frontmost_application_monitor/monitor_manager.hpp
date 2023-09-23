#pragma once

// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "application.hpp"
#include "impl/impl.h"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace osx {
namespace frontmost_application_monitor {
class monitor;

// Manage valid frontmost_application_monitor::monitor instances with monitor_manager because
// the callbacks implemented in PQRSOSXFrontmostApplicationMonitor.swift are not cleanly consistent with the lifetime of monitor.
class monitor_manager final {
public:
  static std::shared_ptr<monitor_manager> get_global_monitor_manager(void) {
    if (!monitor_manager_) {
      monitor_manager_ = std::make_shared<monitor_manager>();
    }

    return monitor_manager_;
  }

  monitor_manager(void) {
  }

  bool contains(const monitor* monitor_pointer) const {
    return monitor_pointers_.contains(monitor_pointer);
  }

  void insert(const monitor* monitor_pointer) {
    monitor_pointers_.insert(monitor_pointer);
  }

  void erase(const monitor* monitor_pointer) {
    monitor_pointers_.erase(monitor_pointer);
  }

private:
  static inline std::shared_ptr<monitor_manager> monitor_manager_;

  std::unordered_set<const monitor*> monitor_pointers_;
};
} // namespace frontmost_application_monitor
} // namespace osx
} // namespace pqrs
