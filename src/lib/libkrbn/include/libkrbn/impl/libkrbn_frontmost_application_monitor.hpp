#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include <pqrs/osx/frontmost_application_monitor.hpp>

class libkrbn_frontmost_application_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_frontmost_application_monitor(const libkrbn_frontmost_application_monitor&) = delete;

  libkrbn_frontmost_application_monitor(void)
      : dispatcher_client() {
    monitor_ = std::make_unique<pqrs::osx::frontmost_application_monitor::monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    monitor_->frontmost_application_changed.connect([this](auto&& application_ptr) {
      application_ = application_ptr;

      for (const auto& c : callback_manager_.get_callbacks()) {
        c();
      }
    });

    monitor_->async_start();

    // Do not wait `changed` callback here.
  }

  ~libkrbn_frontmost_application_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
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
