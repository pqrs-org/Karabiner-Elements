#pragma once

#include <asio.hpp>
#include <csignal>
#include <functional>
#include <thread>
#include <utility>

namespace krbn {
class termination_signal_monitor final {
public:
  using handler = std::function<void(int)>;

  termination_signal_monitor(const termination_signal_monitor&) = delete;

  explicit termination_signal_monitor(handler handler)
      : signals_(io_context_,
                 SIGINT,
                 SIGTERM),
        handler_(std::move(handler)) {
    signals_.async_wait([this](const auto& error_code, int signal_number) {
      if (error_code) {
        return;
      }

      // Restore the default handlers after the first signal so that a second
      // termination signal can force termination if graceful shutdown gets stuck.
      asio::error_code clear_error_code;
      signals_.clear(clear_error_code);

      handler_(signal_number);
    });

    thread_ = std::thread([this] {
      io_context_.run();
    });
  }

  ~termination_signal_monitor() {
    asio::error_code error_code;
    signals_.cancel(error_code);
    io_context_.stop();

    if (thread_.joinable()) {
      thread_.join();
    }
  }

private:
  asio::io_context io_context_;
  asio::signal_set signals_;
  handler handler_;
  std::thread thread_;
};
} // namespace krbn
