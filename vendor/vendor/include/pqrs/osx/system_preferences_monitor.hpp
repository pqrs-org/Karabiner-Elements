#pragma once

// pqrs::osx::system_preferences_monitor v1.4.0

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/system_preferences.hpp>

namespace pqrs::osx {
class system_preferences_monitor final : public dispatcher::extra::dispatcher_client {
private:
  // Keep the guard first so member initialization failures also detach.
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_exception_guard_{*this};

public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(pqrs::not_null_shared_ptr_t<system_preferences::properties> value)> system_preferences_changed;

  // Methods

  system_preferences_monitor(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher),
                                                                                      timer_(*this) {
    dispatcher_client_constructor_exception_guard_.initialize();
  }

  ~system_preferences_monitor() override {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start(std::chrono::milliseconds check_interval) {
    timer_.start(
        [this] {
          auto p = make_current_properties();

          if (!last_properties_ || *last_properties_ != *p) {
            emit_system_preferences_changed(p);
          }
        },
        check_interval);
  }

  // A method for forcibly retrieving the current value in case of missed events, such as during user account switches.
  void async_trigger_system_preferences_changed() {
    enqueue_to_dispatcher([this] {
      emit_system_preferences_changed(make_current_properties());
    });
  }

private:
  [[nodiscard]] pqrs::not_null_shared_ptr_t<system_preferences::properties> make_current_properties() const {
    pqrs::not_null_shared_ptr_t<system_preferences::properties> p = std::make_shared<system_preferences::properties>();
    p->update();
    return p;
  }

  void emit_system_preferences_changed(pqrs::not_null_shared_ptr_t<system_preferences::properties> properties) {
    last_properties_ = properties;

    enqueue_to_dispatcher([this, properties] {
      system_preferences_changed(properties);
    });
  }

  std::shared_ptr<system_preferences::properties> last_properties_;
  // Construct after potentially throwing members; destruction requires detach.
  dispatcher::extra::timer timer_;
};
} // namespace pqrs::osx
