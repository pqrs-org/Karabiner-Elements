#pragma once

#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include <pqrs/osx/file_monitor.hpp>

class libkrbn_file_monitors final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  class monitor final {
  public:
    monitor(const std::string& file_path) {
      std::vector<std::string> targets = {
          file_path,
      };
      monitor_ = std::make_unique<pqrs::osx::file_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                           targets);

      monitor_->file_changed.connect([this](auto&& changed_file_path,
                                            auto&& changed_file_body) {
        for (const auto& c : callback_manager_.get_callbacks()) {
          c();
        }
      });

      monitor_->async_start();

      // Do not wait for the `changed` callback here to avoid a deadlock, as this method is called in the shared dispatcher thread.
    }

    ~monitor(void) {
      monitor_ = nullptr;
    }

    void register_callback(libkrbn_file_updated callback) {
      callback_manager_.register_callback(callback);
    }

    void unregister_callback(libkrbn_file_updated callback) {
      callback_manager_.unregister_callback(callback);
    }

    size_t callbacks_empty(void) const {
      return callback_manager_.get_callbacks().empty();
    }

  private:
    std::unique_ptr<pqrs::osx::file_monitor> monitor_;
    libkrbn_callback_manager<libkrbn_file_updated> callback_manager_;
  };

  libkrbn_file_monitors(const libkrbn_file_monitors&) = delete;

  libkrbn_file_monitors(void)
      : dispatcher_client() {
  }

  ~libkrbn_file_monitors(void) {
    detach_from_dispatcher([this] {
      monitors_.clear();
    });
  }

  void register_libkrbn_file_updated_callback(const std::string& file_path,
                                              libkrbn_file_updated callback) {
    enqueue_to_dispatcher([this, file_path, callback] {
      auto it = monitors_.find(file_path);
      if (it == std::end(monitors_)) {
        auto pair = monitors_.insert({file_path, std::make_shared<monitor>(file_path)});
        it = pair.first;
      }

      it->second->register_callback(callback);
    });
  }

  void unregister_libkrbn_file_updated_callback(const std::string& file_path,
                                                libkrbn_file_updated callback) {
    enqueue_to_dispatcher([this, file_path, callback] {
      auto it = monitors_.find(file_path);
      if (it != std::end(monitors_)) {
        it->second->unregister_callback(callback);

        // Remove the monitor if there are no remaining callbacks.
        if (it->second->callbacks_empty()) {
          monitors_.erase(it);
        }
      }
    });
  }

private:
  std::unordered_map<std::string, std::shared_ptr<monitor>> monitors_;
};
