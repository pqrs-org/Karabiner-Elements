#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/local_datagram.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>
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

  receiver(void) : dispatcher_client() {
    input_source_selector_ = std::make_unique<pqrs::osx::input_source_selector::selector>(weak_dispatcher_);
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_ = nullptr;
      input_source_selector_ = nullptr;
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
        if (buffer) {
          if (buffer->empty()) {
            return;
          }

          try {
            nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
            switch (json.at("operation_type").get<operation_type>()) {
              case operation_type::select_input_source: {
                using specifiers_t = std::vector<pqrs::osx::input_source_selector::specifier>;
                auto specifiers = json.at("input_source_specifiers").get<specifiers_t>();
                input_source_selector_->async_select(std::make_shared<specifiers_t>(specifiers));
                break;
              }

              case operation_type::shell_command_execution: {
                auto shell_command = json.at("shell_command").get<std::string>();
                auto background_shell_command = pqrs::shell::make_background_command_string(shell_command);
                system(background_shell_command.c_str());
                break;
              }

              case operation_type::set_notification_message: {
                auto notification_message = json.at("notification_message").get<std::string>();
                json_writer::async_save_to_file(
                    nlohmann::json::object({
                        {"body", notification_message},
                    }),
                    constants::get_user_notification_message_file_path(),
                    0700,
                    0600);
                break;
              }

              default:
                break;
            }
            return;
          } catch (std::exception& e) {
            logger::get_logger()->error("Received data is corrupted: {0}", e.what());
          }
        }
      });

      server_->async_start();

      logger::get_logger()->info("receiver is initialized");
    });
  }

private:
  std::unique_ptr<pqrs::local_datagram::server> server_;
  std::unique_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
};
} // namespace krbn
