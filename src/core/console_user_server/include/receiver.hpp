#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "input_source_manager.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/shell.hpp>
#include <vector>

namespace krbn {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(void)> closed;

  // Methods

  receiver(const receiver&) = delete;

  receiver(void) : dispatcher_client(),
                   last_select_input_source_time_stamp_(0) {
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
    });

    logger::get_logger()->info("receiver is terminated");
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (server_) {
        return;
      }

      auto uid = getuid();
      auto socket_file_path = console_user_server_client::make_console_user_server_socket_file_path(uid);

      unlink(socket_file_path.c_str());

      size_t buffer_size = 32 * 1024;
      server_ = std::make_unique<pqrs::local_datagram::server>(weak_dispatcher_,
                                                               socket_file_path,
                                                               buffer_size);
      server_->set_server_check_interval(std::chrono::milliseconds(3000));
      server_->set_reconnect_interval(std::chrono::milliseconds(1000));

      server_->bound.connect([this] {
        enqueue_to_dispatcher([this] {
          bound();
        });
      });

      server_->bind_failed.connect([this](auto&& error_code) {
        enqueue_to_dispatcher([this, error_code] {
          bind_failed(error_code);
        });
      });

      server_->closed.connect([this] {
        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      server_->received.connect([this](auto&& buffer) {
        if (auto type = types::find_operation_type(*buffer)) {
          switch (*type) {
            case operation_type::shell_command_execution:
              if (buffer->size() != sizeof(operation_type_shell_command_execution_struct)) {
                logger::get_logger()->error("invalid size for operation_type::shell_command_execution");
              } else {
                auto p = reinterpret_cast<operation_type_shell_command_execution_struct*>(&((*buffer)[0]));

                // Ensure shell_command is null-terminated string even if corrupted data is sent.
                p->shell_command[sizeof(p->shell_command) - 1] = '\0';

                auto background_shell_command = pqrs::shell::make_background_command_string(p->shell_command);
                system(background_shell_command.c_str());
              }
              break;

            case operation_type::select_input_source:
              if (buffer->size() != sizeof(operation_type_select_input_source_struct)) {
                logger::get_logger()->error("invalid size for operation_type::select_input_source");
              } else {
                auto p = reinterpret_cast<operation_type_select_input_source_struct*>(&((*buffer)[0]));

                // Ensure input_source_selector's strings are null-terminated string even if corrupted data is sent.
                p->language[sizeof(p->language) - 1] = '\0';
                p->input_source_id[sizeof(p->input_source_id) - 1] = '\0';
                p->input_mode_id[sizeof(p->input_mode_id) - 1] = '\0';

                auto time_stamp = p->time_stamp;
                std::optional<std::string> language(std::string(p->language));
                std::optional<std::string> input_source_id(std::string(p->input_source_id));
                std::optional<std::string> input_mode_id(std::string(p->input_mode_id));
                if (language && language->empty()) {
                  language = std::nullopt;
                }
                if (input_source_id && input_source_id->empty()) {
                  input_source_id = std::nullopt;
                }
                if (input_mode_id && input_mode_id->empty()) {
                  input_mode_id = std::nullopt;
                }

                input_source_selector input_source_selector(language,
                                                            input_source_id,
                                                            input_mode_id);

                if (last_select_input_source_time_stamp_ == time_stamp) {
                  return;
                }
                if (input_source_manager_.select(input_source_selector)) {
                  last_select_input_source_time_stamp_ = time_stamp;
                }
              }
              break;

            default:
              break;
          }
        }
      });

      server_->async_start();

      logger::get_logger()->info("receiver is initialized");
    });
  }

private:
  std::unique_ptr<pqrs::local_datagram::server> server_;
  input_source_manager input_source_manager_;
  absolute_time_point last_select_input_source_time_stamp_;
};
} // namespace krbn
