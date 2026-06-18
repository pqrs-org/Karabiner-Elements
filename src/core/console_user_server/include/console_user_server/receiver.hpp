#pragma once

// `krbn::console_user_server::receiver` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "constants.hpp"
#include "filesystem_utility.hpp"
#include "send_user_command_handler.hpp"
#include "services_utility.hpp"
#include "settings_window_guidance_manager.hpp"
#include "shell_command_handler.hpp"
#include "software_function_handler.hpp"
#include "types.hpp"
#include "update_utility.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/input_source_selector.hpp>
#include <pqrs/osx/input_source_selector/extra/nlohmann_json.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <random>

namespace krbn::console_user_server {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  receiver(const receiver&) = delete;

  receiver(std::weak_ptr<settings_window_guidance_manager> weak_settings_window_guidance_manager,
           std::weak_ptr<software_function_handler> weak_software_function_handler)
      : dispatcher_client(),
        weak_settings_window_guidance_manager_(weak_settings_window_guidance_manager),
        weak_software_function_handler_(weak_software_function_handler),
        input_source_selector_(std::make_unique<pqrs::osx::input_source_selector::selector>(weak_dispatcher_)),
        shell_command_handler_(std::make_unique<shell_command_handler>()),
        send_user_command_handler_(std::make_unique<send_user_command_handler>()) {
    prepare_console_user_server_socket_directory();

    auto options = pqrs::unix_domain_stream::server_options(
        {
            .max_message_size = constants::unix_domain_stream_max_message_size,
        },
        {
            .bind_retry_interval = std::chrono::milliseconds(1000),
            .socket_path_health_check_interval = std::chrono::milliseconds(3000),
        });

    auto socket_file_path = console_user_server_socket_file_path();

    server_ = std::make_unique<pqrs::unix_domain_stream::server>(
        weak_dispatcher_,
        socket_file_path,
        options,
        [](const auto& peer_credentials) {
          auto result = get_shared_codesign_manager()->same_team_id(peer_credentials.pid);
          if (!result) {
            // During an update, retrieving the Team ID may fail, causing an error once.
            // Since this can occur during normal use, treat it as debug rather than warn.
            logger::get_logger()->debug("receiver: peer is not code-signed with same Team ID");
          }
          return result;
        });

    server_->bound.connect([socket_file_path] {
      logger::get_logger()->info("receiver: bound");
    });

    server_->bind_failed.connect([this](auto&& error_code) {
      logger::get_logger()->error("receiver: bind_failed: {0}",
                                  error_code.message());

      // If the socket directory is deleted for any reason,
      // bind_failed will be triggered, so recreate the directory each time.
      prepare_console_user_server_socket_directory();
    });

    server_->closed.connect([] {
      logger::get_logger()->info("receiver: closed");
    });

    server_->peer_connected.connect([](auto, auto&&) {
      // Do nothing
    });

    server_->peer_closed.connect([](auto) {
      // Do nothing
    });

    server_->peer_error_occurred.connect([](auto peer_id, auto&& error_code) {
      logger::get_logger()->debug("receiver: peer_error_occurred ({0}): {1}", peer_id, error_code.message());
    });

    server_->received.connect([](auto, auto&&) {
      // Do nothing
    });

    server_->request_received.connect([this](auto peer_id, auto request_id, auto&& buffer) {
      handle_request(peer_id,
                     request_id,
                     buffer);
    });

    server_->async_start();

    logger::get_logger()->info("receiver is initialized");
  }

  ~receiver() override {
    detach_from_dispatcher([this] {
      input_source_selector_ = nullptr;
      shell_command_handler_ = nullptr;
      send_user_command_handler_ = nullptr;

      server_ = nullptr;
    });

    logger::get_logger()->info("receiver is terminated");
  }

private:
  std::filesystem::path console_user_server_socket_file_path() const {
    return constants::get_console_user_server_socket_file_path(geteuid());
  }

  void prepare_console_user_server_socket_directory() const {
    filesystem_utility::create_base_directories(geteuid());
  }

  void async_respond(pqrs::unix_domain_stream::peer_id peer_id,
                     pqrs::unix_domain_stream::request_id request_id,
                     nlohmann::json json) {
    if (server_) {
      server_->async_respond(peer_id,
                             request_id,
                             nlohmann::json::to_msgpack(json));
    }
  }

  void async_respond_none(pqrs::unix_domain_stream::peer_id peer_id,
                          pqrs::unix_domain_stream::request_id request_id) {
    async_respond(peer_id,
                  request_id,
                  nlohmann::json{
                      {"operation_type", operation_type::none},
                  });
  }

  void handle_request(pqrs::unix_domain_stream::peer_id peer_id,
                      pqrs::unix_domain_stream::request_id request_id,
                      pqrs::not_null_shared_ptr_t<std::vector<uint8_t>> buffer) {
    if (buffer->empty()) {
      server_->async_close_peer(peer_id);
      return;
    }

    try {
      nlohmann::json json = nlohmann::json::from_msgpack(*buffer);
      switch (json.at("operation_type").get<operation_type>()) {
        case operation_type::get_user_core_configuration_file_path:
          async_respond(peer_id,
                        request_id,
                        nlohmann::json{
                            {"operation_type", operation_type::user_core_configuration_file_path},
                            {"user_core_configuration_file_path", constants::get_user_core_configuration_file_path()},
                        });
          break;

        case operation_type::get_settings_window_guidance:
          if (auto m = weak_settings_window_guidance_manager_.lock()) {
            async_respond(peer_id,
                          request_id,
                          nlohmann::json{
                              {"operation_type", operation_type::settings_window_guidance},
                              {"settings_window_guidance", m->get_guidance_state()}});
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::get_frontmost_application_history:
          if (auto h = weak_software_function_handler_.lock()) {
            h->async_invoke_with_frontmost_application_history(
                [this, peer_id, request_id](auto&& frontmost_application_history) {
                  async_respond(peer_id,
                                request_id,
                                nlohmann::json{
                                    {"operation_type", operation_type::frontmost_application_history},
                                    {"frontmost_application_history", frontmost_application_history}});
                });
          } else {
            async_respond_none(peer_id,
                               request_id);
          }
          break;

        case operation_type::core_service_daemon_state:
          if (auto m = weak_settings_window_guidance_manager_.lock()) {
            m->async_update_core_service_daemon_state(
                json.at("core_service_daemon_state").get<core_service_daemon_state>());
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::check_for_updates:
          set_check_for_updates_enabled(json.at("enabled").get<bool>());
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::register_menu_agent:
          services_utility::register_menu_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::unregister_menu_agent:
          services_utility::unregister_menu_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::register_multitouch_extension_agent:
          services_utility::register_multitouch_extension_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::unregister_multitouch_extension_agent:
          services_utility::unregister_multitouch_extension_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::register_notification_window_agent:
          services_utility::register_notification_window_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::unregister_notification_window_agent:
          services_utility::unregister_notification_window_agent();
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::frontmost_application_changed:
          if (auto h = weak_software_function_handler_.lock()) {
            auto app = json.at("frontmost_application").get<application>();
            h->add_frontmost_application_history(app);
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::focused_ui_element_changed:
          if (auto h = weak_software_function_handler_.lock()) {
            auto element = json.at("focused_ui_element").get<focused_ui_element>();
            h->set_focused_ui_element(element);
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::select_input_source:
          if (input_source_selector_) {
            using specifiers_t = std::vector<pqrs::osx::input_source_selector::specifier>;
            auto specifiers = json.at("input_source_specifiers").get<specifiers_t>();
            input_source_selector_->async_select(std::make_shared<specifiers_t>(specifiers));
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::shell_command_execution:
          if (shell_command_handler_) {
            auto shell_command = json.at("shell_command").get<std::string>();
            shell_command_handler_->run(shell_command);
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::send_user_command:
          if (send_user_command_handler_) {
            auto user_command = json.at("user_command").get<nlohmann::json>();
            send_user_command_handler_->run(user_command);
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        case operation_type::software_function:
          if (auto h = weak_software_function_handler_.lock()) {
            h->execute_software_function(json.at("software_function").get<software_function>());
          }
          async_respond_none(peer_id,
                             request_id);
          break;

        default:
          server_->async_close_peer(peer_id);
          break;
      }
    } catch (std::exception& e) {
      logger::get_logger()->error("received data is corrupted");
      server_->async_close_peer(peer_id);
    }
  }

  void set_check_for_updates_enabled(bool enabled) {
    if (check_for_updates_enabled_ == enabled) {
      return;
    }

    check_for_updates_enabled_ = enabled;
    ++check_for_updates_generation_;

    if (enabled) {
      logger::get_logger()->info("Check for updates is enabled; waiting 30 seconds before checking for updates.");

      // Note:
      //
      // During the updates, Karabiner-Updater.app and console_user_server binaries are overwritten asynchronous.
      // And console_user_server will be restarted via version check.
      // If console_user_server is restarted before Karabiner-Updater.app overwritten,
      // checking for updates runs with the old version of Karabiner-Updater.app,
      // and the update dialog is shown again even though the update was just completed.
      //
      // Wait before checking for updates to avoid it.

      schedule_check_for_updates(check_for_updates_generation_,
                                 std::chrono::seconds(30));
    }
  }

  void schedule_check_for_updates(uint64_t generation,
                                  std::chrono::milliseconds delay) {
    enqueue_to_dispatcher(
        [this, generation] {
          if (!check_for_updates_enabled_ ||
              generation != check_for_updates_generation_) {
            return;
          }

          logger::get_logger()->info("Check for updates...");
          update_utility::check_for_updates_in_background();

          schedule_check_for_updates(generation,
                                     make_next_check_for_updates_interval());
        },
        when_now() + delay);
  }

  static std::chrono::milliseconds make_next_check_for_updates_interval() {
    static std::random_device random_device;
    static std::mt19937 engine(random_device());
    static std::uniform_int_distribution<int> jitter_minutes(0, 60);

    return std::chrono::hours(24) +
           std::chrono::minutes(jitter_minutes(engine));
  }

  std::weak_ptr<settings_window_guidance_manager> weak_settings_window_guidance_manager_;
  std::weak_ptr<software_function_handler> weak_software_function_handler_;
  std::unique_ptr<pqrs::osx::input_source_selector::selector> input_source_selector_;
  std::unique_ptr<shell_command_handler> shell_command_handler_;
  std::unique_ptr<send_user_command_handler> send_user_command_handler_;
  std::unique_ptr<pqrs::unix_domain_stream::server> server_;
  bool check_for_updates_enabled_ = false;
  uint64_t check_for_updates_generation_ = 0;
};
} // namespace krbn::console_user_server
