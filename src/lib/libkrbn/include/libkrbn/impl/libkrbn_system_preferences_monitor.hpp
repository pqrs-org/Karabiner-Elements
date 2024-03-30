#pragma once

#include "libkrbn/libkrbn.h"
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

      for (const auto& c : libkrbn_system_preferences_updated_callbacks_) {
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
      libkrbn_system_preferences_updated_callbacks_.push_back(callback);
    });
  }

  void unregister_libkrbn_system_preferences_updated_callback(libkrbn_system_preferences_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      libkrbn_system_preferences_updated_callbacks_.erase(std::remove_if(std::begin(libkrbn_system_preferences_updated_callbacks_),
                                                                         std::end(libkrbn_system_preferences_updated_callbacks_),
                                                                         [&](auto& c) {
                                                                           return c == callback;
                                                                         }),
                                                          std::end(libkrbn_system_preferences_updated_callbacks_));
    });
  }

private:
  std::unique_ptr<pqrs::osx::system_preferences_monitor> monitor_;
  std::weak_ptr<pqrs::osx::system_preferences::properties> weak_system_preferences_properties_;
  std::vector<libkrbn_system_preferences_updated> libkrbn_system_preferences_updated_callbacks_;
};
