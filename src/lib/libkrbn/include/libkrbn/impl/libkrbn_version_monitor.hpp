#pragma once

#include "libkrbn/impl/libkrbn_callback_manager.hpp"
#include "libkrbn/libkrbn.h"
#include "monitor/version_monitor.hpp"

class libkrbn_version_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_version_monitor(const libkrbn_version_monitor&) = delete;

  libkrbn_version_monitor(void)
      : dispatcher_client(),
        callback_manager_(std::make_unique<libkrbn_callback_manager<libkrbn_version_updated>>()) {
    monitor_ = std::make_unique<krbn::version_monitor>(krbn::constants::get_version_file_path());

    monitor_->changed.connect([this](auto&& version) {
      version_ = version;

      callback_manager_->trigger();
    });

    monitor_->async_start();

    // Do not wait `changed` callback here.
  }

  ~libkrbn_version_monitor(void) {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  const std::string& get_version(void) const {
    return version_;
  }

  void register_libkrbn_version_updated_callback(libkrbn_version_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_->register_callback(callback);
    });
  }

  void unregister_libkrbn_version_updated_callback(libkrbn_version_updated callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_->unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::version_monitor> monitor_;
  std::string version_;
  std::unique_ptr<libkrbn_callback_manager<libkrbn_version_updated>> callback_manager_;
};
