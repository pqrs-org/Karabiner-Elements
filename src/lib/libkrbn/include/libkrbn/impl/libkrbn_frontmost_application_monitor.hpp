#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include <pqrs/osx/frontmost_application_monitor.hpp>

class libkrbn_frontmost_application_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_frontmost_application_monitor(const libkrbn_frontmost_application_monitor&) = delete;

  libkrbn_frontmost_application_monitor(void)
      : dispatcher_client() {
    pqrs::osx::frontmost_application_monitor::monitor::initialize_shared_monitor(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    if (auto m = pqrs::osx::frontmost_application_monitor::monitor::get_shared_monitor().lock()) {
      m->frontmost_application_changed.connect([this](auto&& application_ptr) {
        application_ = application_ptr;

        for (const auto& c : callback_manager_.get_callbacks()) {
          c();
        }
      });

      m->trigger();
    }

    // Do not wait `changed` callback here.
  }

  ~libkrbn_frontmost_application_monitor(void) {
    pqrs::osx::frontmost_application_monitor::monitor::terminate_shared_monitor();

    detach_from_dispatcher();
  }

  std::shared_ptr<pqrs::osx::frontmost_application_monitor::application> get_application(void) const {
    return application_;
  }

  void register_libkrbn_frontmost_application_changed_callback(libkrbn_frontmost_application_changed callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_frontmost_application_changed_callback(libkrbn_frontmost_application_changed callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<pqrs::osx::frontmost_application_monitor::monitor> monitor_;
  std::shared_ptr<pqrs::osx::frontmost_application_monitor::application> application_;
  libkrbn_callback_manager<libkrbn_frontmost_application_changed> callback_manager_;
};
