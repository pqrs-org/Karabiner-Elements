#pragma once

#include "core_service_daemon_client.hpp"
#include "process_lifecycle_manager.hpp"
#include "termination_signal_monitor.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <pqrs/thread_wait.hpp>

namespace krbn::cli::watch_multitouch_extension_variables {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  explicit components_manager(int interval)
      : dispatcher_client(),
        interval_(interval),
        client_(std::make_unique<core_service_daemon_client>()),
        timer_(*this) {
    client_->connected.connect([this] {
      timer_.start(
          [this] {
            client_->async_get_multitouch_extension_variables();
          },
          std::chrono::milliseconds(interval_));
    });

    client_->connect_failed.connect([this](auto&&) {
      timer_.stop();
    });

    client_->closed.connect([this] {
      timer_.stop();
    });

    client_->received.connect([this](auto&& operation_type,
                                     auto&& json) {
      try {
        switch (operation_type) {
          case operation_type::multitouch_extension_variables: {
            auto string = json.at("multitouch_extension_variables").dump();
            if (output_json_string_ != string) {
              output_json_string_ = string;
              std::cout << string << std::endl;
            }
            break;
          }

          default:
            break;
        }
      } catch (std::exception& e) {
        std::cerr << "watch-multitouch-extension-variables error:" << std::endl
                  << e.what() << std::endl;
      }
    });
  }

  ~components_manager() override {
    detach_from_dispatcher([this] {
      timer_.stop();
      client_ = nullptr;
    });
  }

  void async_start() {
    client_->async_start();
  }

private:
  int interval_;
  std::string output_json_string_;
  std::unique_ptr<core_service_daemon_client> client_;
  pqrs::dispatcher::extra::timer timer_;
};
inline int run(int interval) {
  auto termination_wait = pqrs::make_thread_wait();
  std::atomic_int termination_signal{0};

  process_lifecycle_manager::initialize_shared_instance(
      process_lifecycle_manager::configuration{
          .components_manager_maker =
              [interval] {
                return std::make_unique<components_manager>(interval);
              },
          .termination_completion_handler = [termination_wait] { termination_wait->notify(); },
      });

  termination_signal_monitor signal_monitor([&termination_signal](int signal_number) {
    termination_signal = signal_number;
    process_lifecycle_manager::async_request_termination();
  });

  process_lifecycle_manager::async_start();

  termination_wait->wait_notice();
  process_lifecycle_manager::terminate_shared_instance();

  return 128 + termination_signal;
}
} // namespace krbn::cli::watch_multitouch_extension_variables
