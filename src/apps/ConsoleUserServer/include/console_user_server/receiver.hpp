#pragma once

// `krbn::console_user_server::receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "send_user_command_handler.hpp"
#include "settings_window_guidance_manager.hpp"
#include "shell_command_handler.hpp"
#include "software_function_handler.hpp"
#include "types.hpp"
#include "update_check_scheduler.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>
#include <pqrs/osx/xpc.hpp>

namespace krbn::console_user_server {
namespace xpc = pqrs::osx::xpc;

class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(const xpc::listener_recovery_request&)> listener_recovery_required;

  // Methods

  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<settings_window_guidance_manager> weak_settings_window_guidance_manager,
           std::weak_ptr<software_function_handler> weak_software_function_handler,
           std::weak_ptr<update_check_scheduler> weak_update_check_scheduler)
      : weak_settings_window_guidance_manager_(weak_settings_window_guidance_manager),
        weak_software_function_handler_(weak_software_function_handler),
        weak_update_check_scheduler_(weak_update_check_scheduler),
        input_source_selector_(std::make_unique<pqrs::osx::input_source_selector::selector>(weak_dispatcher_)),
        shell_command_handler_(std::make_unique<shell_command_handler>()),
        send_user_command_handler_(std::make_unique<send_user_command_handler>()) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
          server_ = std::make_shared<xpc::listener>(
              weak_dispatcher_,
              xpc::listener_options({
                  .common_parameters = {
                      .signing_requirement = get_shared_codesign_manager()->xpc_signing_requirement({
                          "org.pqrs.Karabiner-CLI",
                          "org.pqrs.Karabiner-Elements.Settings",
                          "org.pqrs.Karabiner-EventViewer",
                      }),
                      .max_message_size = constants::xpc_max_message_size,
                  },
                  .listener_parameters = {
                      .service_name = constants::get_console_user_server_mach_service_name(),
                  },
              }));

          server_->listener_started.connect([] {
            logger::get_logger()->debug("receiver: listener_started");
          });

          server_->listener_recovery_required.connect([this](const auto& request) {
            listener_recovery_required(request);
          });

          server_->peer_ready.connect([](auto) {
            // Do nothing
          });

          server_->peer_interrupted.connect([](auto) {
            // Do nothing
          });

          server_->peer_invalidated.connect([](auto peer_id,
                                               const auto& error_code) {
            logger::get_logger()->debug("receiver: peer_invalidated ({0}): {1}",
                                        peer_id,
                                        error_code.message());
          });

          server_->message_received.connect([](auto, auto&&) {
            // Do nothing
          });

          server_->request_received.connect([this](auto peer_id,
                                                   auto reply,
                                                   auto&& buffer) {
            handle_request(peer_id,
                           reply,
                           buffer);
          });

          server_->async_start();

          logger::get_logger()->debug("receiver is initialized");
        },
        [this] {
          cleanup();
        });
  }

  ~receiver() override {
    detach_from_dispatcher([this] {
      cleanup();
    });

    logger::get_logger()->debug("receiver is terminated");
  }

  void handle_core_service_daemon_message(operation_type operation_type_value,
                                          const nlohmann::json& json) {
    try {
      switch (operation_type_value) {
        case operation_type::core_service_daemon_state:
          if (auto m = weak_settings_window_guidance_manager_.lock()) {
            m->async_update_core_service_daemon_state(
                json.at("core_service_daemon_state").get<core_service_daemon_state>());
          }
          break;

        case operation_type::check_for_updates:
          if (auto scheduler = weak_update_check_scheduler_.lock()) {
            scheduler->async_set_enabled(json.at("enabled").get<bool>());
          }
          break;

        case operation_type::frontmost_application_changed:
          if (auto h = weak_software_function_handler_.lock()) {
            auto app = json.at("frontmost_application").get<application>();
            h->add_frontmost_application_history(app);
          }
          break;

        case operation_type::focused_ui_element_changed:
          if (auto h = weak_software_function_handler_.lock()) {
            auto element = json.at("focused_ui_element").get<focused_ui_element>();
            h->set_focused_ui_element(element);
          }
          break;

        case operation_type::select_input_source:
          if (input_source_selector_) {
            using specifiers_t = std::vector<pqrs::osx::input_source_selector::specifier>;
            input_source_selector_->async_select(json.at("input_source_specifiers").get<specifiers_t>());
          }
          break;

        case operation_type::shell_command_execution:
          if (shell_command_handler_) {
            auto shell_command = json.at("shell_command").get<std::string>();
            shell_command_handler_->run(shell_command);
          }
          break;

        case operation_type::send_user_command:
          if (send_user_command_handler_) {
            auto user_command = json.at("user_command").get<nlohmann::json>();
            send_user_command_handler_->run(user_command);
          }
          break;

        case operation_type::software_function:
          if (auto h = weak_software_function_handler_.lock()) {
            h->execute_software_function(json.at("software_function").get<software_function>());
          }
          break;

        default:
          break;
      }
    } catch (std::exception& e) {
      logger::get_logger()->error("receiver: core_service daemon message is corrupted");
    }
  }

private:
  void cleanup() {
    input_source_selector_ = nullptr;
    shell_command_handler_ = nullptr;
    send_user_command_handler_ = nullptr;

    server_ = nullptr;
  }

  void async_respond(xpc::reply_token reply,
                     nlohmann::json json) {
    if (server_) {
      server_->async_reply(reply,
                           std::make_shared<const std::vector<uint8_t>>(nlohmann::json::to_msgpack(json)));
    }
  }

  void async_respond_none(xpc::reply_token reply) {
    async_respond(reply,
                  nlohmann::json{
                      {"operation_type", operation_type::none},
                  });
  }

  void handle_request(xpc::peer_id peer_id,
                      xpc::reply_token reply,
                      std::shared_ptr<const std::vector<uint8_t>> buffer) {
    if (buffer->empty()) {
      server_->async_cancel_peer(peer_id);
      return;
    }

    try {
      nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
      if (json.at("uid").get<uid_t>() != geteuid()) {
        server_->async_cancel_peer(peer_id);
        return;
      }

      switch (json.at("operation_type").get<operation_type>()) {
        case operation_type::get_settings_window_guidance:
          if (auto m = weak_settings_window_guidance_manager_.lock()) {
            async_respond(reply,
                          nlohmann::json{
                              {"operation_type", operation_type::settings_window_guidance},
                              {"settings_window_guidance", m->get_guidance_state()}});
          } else {
            async_respond_none(reply);
          }
          break;

        case operation_type::get_frontmost_application_history:
          if (auto h = weak_software_function_handler_.lock()) {
            h->async_invoke_with_frontmost_application_history(
                [this, reply](auto&& frontmost_application_history) {
                  async_respond(reply,
                                nlohmann::json{
                                    {"operation_type", operation_type::frontmost_application_history},
                                    {"frontmost_application_history", frontmost_application_history}});
                });
          } else {
            async_respond_none(reply);
          }
          break;

        default:
          server_->async_cancel_peer(peer_id);
          break;
      }
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
      server_->async_cancel_peer(peer_id);
    }
  }

  std::weak_ptr<settings_window_guidance_manager> weak_settings_window_guidance_manager_;
  std::weak_ptr<software_function_handler> weak_software_function_handler_;
  std::weak_ptr<update_check_scheduler> weak_update_check_scheduler_;
  std::unique_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
  std::unique_ptr<shell_command_handler> shell_command_handler_;
  std::unique_ptr<send_user_command_handler> send_user_command_handler_;
  std::shared_ptr<xpc::listener> server_;
};
} // namespace krbn::console_user_server
