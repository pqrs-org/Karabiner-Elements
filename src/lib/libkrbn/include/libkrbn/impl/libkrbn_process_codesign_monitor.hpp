#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include "monitor/process_codesign_monitor.hpp"
#include <pqrs/dispatcher.hpp>

class libkrbn_process_codesign_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_process_codesign_monitor(const libkrbn_process_codesign_monitor&) = delete;

  libkrbn_process_codesign_monitor() {
    monitor_ = std::make_unique<krbn::process_codesign_monitor>();

    monitor_->invalidated.connect([this] {
      for (const auto& c : callback_manager_.get_callbacks()) {
        c();
      }
    });

    monitor_->async_start();
  }

  ~libkrbn_process_codesign_monitor() override {
    detach_from_dispatcher([this] {
      monitor_ = nullptr;
    });
  }

  void register_libkrbn_process_codesign_invalidated_callback(libkrbn_process_codesign_invalidated_t callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_process_codesign_invalidated_callback(libkrbn_process_codesign_invalidated_t callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  std::unique_ptr<krbn::process_codesign_monitor> monitor_;
  libkrbn_callback_manager<libkrbn_process_codesign_invalidated_t> callback_manager_;
};
