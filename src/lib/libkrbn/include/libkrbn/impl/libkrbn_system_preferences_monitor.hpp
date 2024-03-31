#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include <pqrs/osx/system_preferences_monitor.hpp>

class libkrbn_system_preferences_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_system_preferences_monitor(const libkrbn_system_preferences_monitor&) = delete;

  libkrbn_system_preferences_monitor(void)
      : dispatcher_client() {
    monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    auto wait = pqrs::make_thread_wait();

    monitor_->system_preferences_changed.connect([this, wait](auto&& properties_ptr) {
      weak_system_preferences_properties_ = properties_ptr;

      for (const auto& c : callback_manager_.get_callbacks()) {
        c();
      }

      wait->notify();
    });

    monitor_->async_start(std::chrono::milliseconds(3000));

    wait->wait_notice();
  }

  ~libkrbn_system_preferences_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  std::weak_ptr<pqrs::osx::system_preferences::properties> get_weak_system_preferences_properties(void) const {
    return weak_system_preferences_properties_;
  }

  void register_libkrbn_system_preferences_updated_callback(libkrbn_system_preferences_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_system_preferences_updated_callback(libkrbn_system_preferences_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<pqrs::osx::system_preferences_monitor> monitor_;
  std::weak_ptr<pqrs::osx::system_preferences::properties> weak_system_preferences_properties_;
  libkrbn_callback_manager<libkrbn_system_preferences_updated> callback_manager_;
};
