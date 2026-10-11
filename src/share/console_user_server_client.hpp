#pragma once

// `krbn::console_user_server_client` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "xpc_client.hpp"
#include <utility>

namespace krbn {
class console_user_server_client final : public xpc_client {
public:
  console_user_server_client(const console_user_server_client&) = delete;

  console_user_server_client()
      : xpc_client({
            .client_name = "console_user_server_client",
            .service_name = constants::get_console_user_server_mach_service_name(),
            .signing_identifier = "org.pqrs.Karabiner-Console-User-Server",
        }) {
  }

  // settings_window -> console_user_server
  void async_get_settings_window_guidance() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_settings_window_guidance},
      };

      async_request(std::move(json));
    });
  }

  // event_viewer -> console_user_server
  void async_get_frontmost_application_history() const {
    enqueue_to_dispatcher([this] {
      nlohmann::json json{
          {"operation_type", operation_type::get_frontmost_application_history},
      };

      async_request(std::move(json));
    });
  }
};
} // namespace krbn
