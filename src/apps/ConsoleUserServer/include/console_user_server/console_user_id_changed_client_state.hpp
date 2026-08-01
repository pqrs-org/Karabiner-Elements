#pragma once

#include <optional>
#include <sys/types.h>

namespace krbn::console_user_server {
class console_user_id_changed_client_state final {
public:
  struct console_user_id_changed_request final {
    bool on_console;

    bool operator==(const console_user_id_changed_request&) const = default;
  };

  // The wrapper distinguishes a bound server whose UID is nullopt from the
  // absence of a notification to emit.
  struct core_service_daemon_server_bound_action final {
    std::optional<uid_t> uid;

    bool operator==(const core_service_daemon_server_bound_action&) const = default;
  };

  void connected() {
    connection_ready_ = true;
    console_user_id_changed_request_in_flight_ = false;
    console_user_id_changed_accepted_ = false;
    pending_core_service_daemon_server_bound_ = std::nullopt;
  }

  void connection_not_ready() {
    connection_ready_ = false;
    console_user_id_changed_request_in_flight_ = false;
    console_user_id_changed_accepted_ = false;
    pending_core_service_daemon_server_bound_ = std::nullopt;
  }

  void stop() {
    connection_not_ready();
    pending_console_user_id_changed_request_ = std::nullopt;
  }

  void console_user_id_changed(bool on_console) {
    pending_console_user_id_changed_request_ = console_user_id_changed_request{
        .on_console = on_console,
    };
    console_user_id_changed_accepted_ = false;
    pending_core_service_daemon_server_bound_ = std::nullopt;
  }

  [[nodiscard]] std::optional<console_user_id_changed_request> take_console_user_id_changed_request() {
    if (!connection_ready_ ||
        !pending_console_user_id_changed_request_ ||
        console_user_id_changed_request_in_flight_) {
      return std::nullopt;
    }

    console_user_id_changed_request_in_flight_ = true;
    return pending_console_user_id_changed_request_;
  }

  void console_user_id_changed_request_succeeded(const console_user_id_changed_request& request) {
    console_user_id_changed_request_in_flight_ = false;

    if (pending_console_user_id_changed_request_ == request) {
      pending_console_user_id_changed_request_ = std::nullopt;
      console_user_id_changed_accepted_ = true;
    }
  }

  void core_service_daemon_server_bound(std::optional<uid_t> uid) {
    pending_core_service_daemon_server_bound_ = core_service_daemon_server_bound_action{
        .uid = uid,
    };
  }

  [[nodiscard]] std::optional<core_service_daemon_server_bound_action> take_core_service_daemon_server_bound_action() {
    // A bound notification may be broadcast because another client updated its state.
    // Do not expose it until this client's latest console_user_id_changed request has
    // been acknowledged and its peer state is registered in the daemon.
    if (!console_user_id_changed_accepted_ ||
        !pending_core_service_daemon_server_bound_) {
      return std::nullopt;
    }

    auto action = pending_core_service_daemon_server_bound_;
    pending_core_service_daemon_server_bound_ = std::nullopt;
    return action;
  }

private:
  std::optional<console_user_id_changed_request> pending_console_user_id_changed_request_;
  std::optional<core_service_daemon_server_bound_action> pending_core_service_daemon_server_bound_;
  bool console_user_id_changed_request_in_flight_ = false;
  bool console_user_id_changed_accepted_ = false;
  bool connection_ready_ = false;
};
} // namespace krbn::console_user_server
