#pragma once

#include "core_service_daemon_client.hpp"
#include "json_utility.hpp"
#include "process_lifecycle_manager.hpp"
#include "termination_signal_monitor.hpp"
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/thread_wait.hpp>
#include <stdexcept>

namespace krbn::cli::set_variables_from_stdin {
class runner final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  runner(const runner&) = delete;

  explicit runner(bool verbose)
      : dispatcher_client(),
        verbose_(verbose),
        input_completion_wait_(pqrs::make_thread_wait()) {
  }

  ~runner() override {
    detach_from_dispatcher();
  }

  int run(std::istream& input);

private:
  class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
  public:
    components_manager(const components_manager&) = delete;

    explicit components_manager(runner& runner)
        : dispatcher_client(),
          runner_(runner),
          client_(std::make_shared<core_service_daemon_client>()),
          process_pending_task_(*this) {
      runner_.set_components_manager(this);

      client_->connected.connect([this] {
        connected_ = true;
        schedule_process_pending_task(std::chrono::milliseconds(0));
      });

      client_->connect_failed.connect([this](auto&&) {
        connected_ = false;
        request_failed_ = true;

        // If EOF was enqueued before the connection failure was reported,
        // process it now so that the command does not wait for a reconnect.
        schedule_process_pending_task(std::chrono::milliseconds(0));
      });

      client_->closed.connect([this] {
        connected_ = false;
        request_failed_ = true;

        // If EOF was enqueued before the disconnection was reported, process
        // it now so that the command does not wait for a reconnect.
        schedule_process_pending_task(std::chrono::milliseconds(0));
      });
    }

    ~components_manager() override {
      detach_from_dispatcher([this] {
        runner_.set_components_manager(nullptr);
        request_variables_.reset();
        request_in_flight_ = false;
        request_failed_ = false;
        connected_ = false;
        callback_lifetime_.reset();
        client_ = nullptr;
      });
    }

    void async_start() {
      client_->async_start();
    }

    void schedule_process_pending_task(std::chrono::milliseconds delay) {
      process_pending_task_.debounce_after(
          [this] {
            process_pending_task();
          },
          delay);
    }

  private:
    void process_pending_task() {
      // This method is called only on the shared dispatcher thread.
      if (request_failed_ &&
          runner_.abort_pending_variables_if_input_complete()) {
        request_variables_.reset();
        request_failed_ = false;
        return;
      }

      if (!connected_) {
        return;
      }

      if (request_in_flight_) {
        return;
      }

      if (!request_variables_) {
        if (runner_.complete_input_if_front()) {
          return;
        }

        request_variables_ = runner_.get_pending_variables();
        request_failed_ = false;
      }

      if (!request_variables_) {
        return;
      }

      request_in_flight_ = true;
      auto variables = *request_variables_;
      auto weak_callback_lifetime = std::weak_ptr(callback_lifetime_);
      client_->async_set_variables_with_completion_handler(
          variables,
          [this, variables, weak_callback_lifetime](const auto& error_code) {
            // The completion handler and components destruction are serialized
            // on the shared dispatcher thread. The token prevents a handler
            // retained by the transport from accessing a destroyed manager.
            if (!weak_callback_lifetime.lock()) {
              return;
            }

            request_in_flight_ = false;

            if (!error_code) {
              runner_.output_variables_if_verbose(variables);
              runner_.complete_pending_variables();
              request_variables_.reset();
              request_failed_ = false;
              schedule_process_pending_task(std::chrono::milliseconds(0));
              return;
            }

            request_failed_ = true;

            // Once stdin has reached EOF, do not keep the command alive just
            // to retry a request that has already failed once.
            if (runner_.abort_pending_variables_if_input_complete()) {
              request_variables_.reset();
              return;
            }

            schedule_process_pending_task(retry_interval);
          });
    }

    static constexpr auto retry_interval = std::chrono::seconds(1);

    runner& runner_;
    std::shared_ptr<core_service_daemon_client> client_;
    pqrs::dispatcher::extra::debounced_task process_pending_task_;
    std::optional<nlohmann::json> request_variables_;
    bool request_in_flight_{false};
    bool request_failed_{false};
    bool connected_{false};
    std::shared_ptr<int> callback_lifetime_{std::make_shared<int>(0)};
  };

  void set_components_manager(components_manager* manager) {
    // All accesses to components_manager_ are made on the shared dispatcher
    // thread, where lifecycle creation, destruction, and task enqueueing are
    // serialized.
    components_manager_ = manager;
  }

  std::optional<nlohmann::json> get_pending_variables() const {
    std::lock_guard<std::mutex> lock(pending_variables_mutex_);

    if (pending_variables_.empty() ||
        pending_variables_.front().is_null()) {
      return std::nullopt;
    }

    return pending_variables_.front();
  }

  void add_pending_variables(const nlohmann::json& variables) {
    std::lock_guard<std::mutex> lock(pending_variables_mutex_);

    pending_variables_.push_back(variables);
  }

  void output_variables_if_verbose(const nlohmann::json& variables) const {
    if (verbose_) {
      std::cout << variables.dump() << std::endl;
    }
  }

  void complete_pending_variables() {
    std::lock_guard<std::mutex> lock(pending_variables_mutex_);

    if (!pending_variables_.empty()) {
      pending_variables_.pop_front();
    }
  }

  bool complete_input_if_front() {
    bool completed = false;
    {
      std::lock_guard<std::mutex> lock(pending_variables_mutex_);

      if (!pending_variables_.empty() &&
          pending_variables_.front().is_null()) {
        pending_variables_.pop_front();
        completed = true;
      }
    }

    if (completed) {
      input_completion_wait_->notify();
    }

    return completed;
  }

  bool abort_pending_variables_if_input_complete() {
    bool completed = false;
    {
      std::lock_guard<std::mutex> lock(pending_variables_mutex_);

      // The completion entry is enqueued only after the stdin loop ends, so it
      // is always the final entry and no further variables can follow it.
      if (!pending_variables_.empty() &&
          pending_variables_.back().is_null()) {
        pending_variables_.clear();
        completed = true;
      }
    }

    if (completed) {
      input_completion_wait_->notify();
    }

    return completed;
  }

  void enqueue_task() {
    enqueue_to_dispatcher([this] {
      if (components_manager_) {
        components_manager_->schedule_process_pending_task(std::chrono::milliseconds(0));
      }
    });
  }

  void enqueue_set_variables(const nlohmann::json& variables) {
    add_pending_variables(variables);
    enqueue_task();
  }

  void enqueue_input_completion() {
    {
      std::lock_guard<std::mutex> lock(pending_variables_mutex_);

      // Input values are always JSON objects, so null can be reserved as the
      // EOF completion entry.
      pending_variables_.push_back(nullptr);
    }

    enqueue_task();
  }

  mutable std::mutex pending_variables_mutex_;
  std::deque<nlohmann::json> pending_variables_;
  bool verbose_;
  std::shared_ptr<pqrs::thread_wait> input_completion_wait_;
  components_manager* components_manager_{nullptr};
};

inline int runner::run(std::istream& input) {
  auto termination_wait = pqrs::make_thread_wait();
  std::atomic_int termination_signal{0};

  process_lifecycle_manager::initialize_shared_instance(
      process_lifecycle_manager::configuration{
          .components_manager_maker =
              [this] {
                return std::make_unique<components_manager>(*this);
              },
          .termination_completion_handler =
              [termination_wait] {
                termination_wait->notify();
              },
      });

  termination_signal_monitor signal_monitor([this, &termination_signal](int signal_number) {
    termination_signal = signal_number;
    input_completion_wait_->notify();
    process_lifecycle_manager::async_request_termination();
  });

  process_lifecycle_manager::async_start();

  size_t line_number = 0;
  std::string line;
  while (termination_signal == 0) {
    if (!std::getline(input, line)) {
      break;
    }

    ++line_number;

    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }

    try {
      auto variables = json_utility::parse_jsonc(line);
      if (!variables.is_object()) {
        throw std::invalid_argument("json must be an object");
      }

      // Validate values here so that a malformed line cannot make the
      // CoreService reject the connection before subsequent lines are read.
      for (const auto& item : variables.items()) {
        static_cast<void>(item.value().get<manipulator_environment_variable_value>());
      }

      enqueue_set_variables(variables);
    } catch (std::exception& e) {
      std::cerr << "set-variables-from-stdin error at line "
                << line_number << ": " << e.what() << std::endl;
    }
  }

  if (termination_signal == 0 &&
      input.bad()) {
    std::cerr << "set-variables-from-stdin error: failed to read stdin" << std::endl;
  }

  if (termination_signal == 0) {
    enqueue_input_completion();
    input_completion_wait_->wait_notice();

    if (termination_signal == 0) {
      process_lifecycle_manager::async_request_termination();
    }
  }

  termination_wait->wait_notice();
  process_lifecycle_manager::terminate_shared_instance();

  if (termination_signal != 0) {
    return 128 + termination_signal;
  }

  return input.bad() ? 1 : 0;
}

} // namespace krbn::cli::set_variables_from_stdin
