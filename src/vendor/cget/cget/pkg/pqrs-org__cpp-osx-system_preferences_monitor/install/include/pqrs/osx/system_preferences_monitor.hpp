#pragma once

// pqrs::osx::system_preferences_monitor v1.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/system_preferences.hpp>

namespace pqrs {
namespace osx {
class system_preferences_monitor final : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(std::shared_ptr<system_preferences::properties> value)> system_preferences_changed;

  // Methods

  system_preferences_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                                      timer_(*this) {
  }

  virtual ~system_preferences_monitor(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start(std::chrono::milliseconds check_interval) {
    timer_.start(
        [this] {
          auto p = std::make_shared<system_preferences::properties>();
          p->update();

          if (!last_properties_ || *last_properties_ != *p) {
            last_properties_ = p;

            enqueue_to_dispatcher([this, p] {
              system_preferences_changed(p);
            });
          }
        },
        check_interval);
  }

private:
  dispatcher::extra::timer timer_;
  std::shared_ptr<system_preferences::properties> last_properties_;
};
} // namespace osx
} // namespace pqrs
