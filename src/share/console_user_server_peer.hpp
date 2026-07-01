#pragma once

#include "types.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <pqrs/dispatcher/extra/dispatcher_client.hpp>
#include <pqrs/unix_domain_stream.hpp>
#include <string>
#include <utility>
#include <vector>

namespace krbn {
class console_user_server_peer final : public pqrs::dispatcher::extra::dispatcher_client,
                                       public std::enable_shared_from_this<console_user_server_peer> {
public:
  console_user_server_peer(const console_user_server_peer&) = delete;

  console_user_server_peer(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                           std::weak_ptr<pqrs::unix_domain_stream::server> weak_server,
                           pqrs::unix_domain_stream::peer_id peer_id)
      : dispatcher_client(std::move(weak_dispatcher)),
        weak_server_(weak_server),
        peer_id_(peer_id) {
  }

  ~console_user_server_peer() override {
    detach_from_dispatcher();
  }

  pqrs::unix_domain_stream::peer_id get_peer_id() const {
    return peer_id_;
  }

  void async_core_service_daemon_state(const core_service_daemon_state& core_service_daemon_state) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::core_service_daemon_state},
        {"core_service_daemon_state", core_service_daemon_state},
    });
  }

  void async_check_for_updates(bool enabled) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::check_for_updates},
        {"enabled", enabled},
    });
  }

  void async_register_menu_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::register_menu_agent},
    });
  }

  void async_unregister_menu_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::unregister_menu_agent},
    });
  }

  void async_register_multitouch_extension_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::register_multitouch_extension_agent},
    });
  }

  void async_unregister_multitouch_extension_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::unregister_multitouch_extension_agent},
    });
  }

  void async_register_notification_window_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::register_notification_window_agent},
    });
  }

  void async_unregister_notification_window_agent() {
    async_send(nlohmann::json{
        {"operation_type", operation_type::unregister_notification_window_agent},
    });
  }

  void async_frontmost_application_changed(const application& application) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::frontmost_application_changed},
        {"frontmost_application", application},
    });
  }

  void async_focused_ui_element_changed(const focused_ui_element& focused_ui_element) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::focused_ui_element_changed},
        {"focused_ui_element", focused_ui_element},
    });
  }

  void async_shell_command_execution(const std::string& shell_command) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::shell_command_execution},
        {"shell_command", shell_command},
    });
  }

  void async_send_user_command(const nlohmann::json& user_command) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::send_user_command},
        {"user_command", user_command},
    });
  }

  void async_select_input_source(const std::vector<pqrs::osx::input_source_selector::specifier>& input_source_specifiers) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::select_input_source},
        {"input_source_specifiers", input_source_specifiers},
    });
  }

  void async_software_function(const software_function& software_function) {
    async_send(nlohmann::json{
        {"operation_type", operation_type::software_function},
        {"software_function", software_function},
    });
  }

private:
  void async_send(nlohmann::json&& json) {
    auto weak_peer = weak_from_this();

    enqueue_to_dispatcher([weak_peer, json = std::move(json)] mutable {
      if (auto peer = weak_peer.lock()) {
        if (auto server = peer->weak_server_.lock()) {
          server->async_send(peer->peer_id_,
                             nlohmann::json::to_msgpack(json));
        }
      }
    });
  }

  std::weak_ptr<pqrs::unix_domain_stream::server> weak_server_;
  pqrs::unix_domain_stream::peer_id peer_id_;
};
} // namespace krbn
