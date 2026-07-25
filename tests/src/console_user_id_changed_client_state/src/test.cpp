#include "../../../../src/core/console_user_server/include/console_user_server/console_user_id_changed_client_state.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using state = krbn::console_user_server::console_user_id_changed_client_state;

  "typical flow emits bound after request success"_test = [] {
    state s;
    s.connected();

    // A connected client exposes the latest console state as a request.
    s.console_user_id_changed(true);
    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());
    expect(request->on_console);

    // Request success only registers this client's state. Since no server_bound
    // has been received yet, there is no action and this returns std::nullopt.
    s.console_user_id_changed_request_succeeded(*request);
    expect(!s.take_core_service_daemon_server_bound_action());

    // Once the daemon receiver is bound, the action contains its UID so that the
    // caller can start the daemon client.
    s.core_service_daemon_server_bound(uid_t(501));
    auto action = s.take_core_service_daemon_server_bound_action();
    expect(action.has_value());
    expect(action->uid == std::optional<uid_t>(501));
  };

  "bound before this client is accepted is not emitted"_test = [] {
    state s;
    s.connected();

    // A broadcast caused by another client cannot be emitted before this client
    // has registered its console state, so this returns std::nullopt.
    s.core_service_daemon_server_bound(uid_t(501));
    expect(!s.take_core_service_daemon_server_bound_action());

    // Starting a new request discards the stored pre-acceptance broadcast.
    s.console_user_id_changed(true);
    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());
    s.console_user_id_changed_request_succeeded(*request);

    // The first server_bound is ignored because it was received before this
    // client's state was accepted, so this returns std::nullopt.
    expect(!s.take_core_service_daemon_server_bound_action());
  };

  "bound before request success is emitted after success"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(true);

    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());

    // A server_bound received while the request is in flight is held, so this
    // returns std::nullopt until the request succeeds.
    s.core_service_daemon_server_bound(uid_t(501));
    expect(!s.take_core_service_daemon_server_bound_action());

    // Accepting the request releases the held notification with UID 501.
    s.console_user_id_changed_request_succeeded(*request);
    auto action = s.take_core_service_daemon_server_bound_action();
    expect(action.has_value());
    expect(action->uid == std::optional<uid_t>(501));

    // Taking the action consumes it, so a second take returns std::nullopt.
    expect(!s.take_core_service_daemon_server_bound_action());
  };

  "request is delivered only while connected"_test = [] {
    state s;

    // While disconnected, the latest console state remains pending and taking
    // a request returns std::nullopt.
    s.console_user_id_changed(true);
    expect(!s.take_console_user_id_changed_request());

    // Connecting makes the pending state available. Taking it marks the request
    // as in flight, so a second take returns std::nullopt.
    s.connected();
    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());
    expect(request->on_console);
    expect(!s.take_console_user_id_changed_request());
  };

  "new console user state supersedes an in-flight request"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(false);

    auto stale_request = s.take_console_user_id_changed_request();
    expect(stale_request.has_value());
    expect(!stale_request->on_console);

    // Queue a newer state and receive a bound notification before the stale
    // request completes.
    s.console_user_id_changed(true);
    s.core_service_daemon_server_bound(uid_t(501));

    // Acknowledging the stale request does not accept the latest state, so the
    // held server_bound cannot be emitted and this returns std::nullopt.
    s.console_user_id_changed_request_succeeded(*stale_request);
    expect(!s.take_core_service_daemon_server_bound_action());

    // Acknowledging the latest request accepts the current state and makes the
    // held server_bound available as an action with UID 501.
    auto latest_request = s.take_console_user_id_changed_request();
    expect(latest_request.has_value());
    expect(latest_request->on_console);
    s.console_user_id_changed_request_succeeded(*latest_request);

    auto action = s.take_core_service_daemon_server_bound_action();
    expect(action.has_value());
    expect(action->uid == std::optional<uid_t>(501));
  };

  "new console user state discards an old bound notification"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(true);

    auto first_request = s.take_console_user_id_changed_request();
    expect(first_request.has_value());

    // Hold a bound notification associated with the first requested state.
    s.core_service_daemon_server_bound(uid_t(501));

    // A newer console state invalidates that notification.
    s.console_user_id_changed(false);
    s.console_user_id_changed_request_succeeded(*first_request);

    // Even after the latest request succeeds, there is no replacement
    // server_bound, so taking an action returns std::nullopt.
    auto second_request = s.take_console_user_id_changed_request();
    expect(second_request.has_value());
    s.console_user_id_changed_request_succeeded(*second_request);
    expect(!s.take_core_service_daemon_server_bound_action());
  };

  "disconnect clears bound state and keeps the latest request"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(true);

    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());
    s.core_service_daemon_server_bound(uid_t(501));

    // Losing the connection clears connection-specific state, including the held
    // server_bound, but preserves the latest console state for retry.
    s.connection_not_ready();

    // A request cannot be taken while disconnected, and the cleared
    // server_bound cannot produce an action; both calls return std::nullopt.
    expect(!s.take_console_user_id_changed_request());
    expect(!s.take_core_service_daemon_server_bound_action());

    // Reconnecting makes the preserved console state available for
    // retransmission, but does not restore the cleared server_bound.
    s.connected();
    auto retried_request = s.take_console_user_id_changed_request();
    expect(retried_request.has_value());
    expect(retried_request->on_console);
    s.console_user_id_changed_request_succeeded(*retried_request);
    expect(!s.take_core_service_daemon_server_bound_action());
  };

  "null uid is a valid bound action"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(false);

    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());
    s.console_user_id_changed_request_succeeded(*request);

    // A null UID means that the daemon server is bound for the no-console-user
    // state; it does not mean that no action is available.
    s.core_service_daemon_server_bound(std::nullopt);

    auto action = s.take_core_service_daemon_server_bound_action();
    expect(action.has_value());

    // The outer optional contains an action, while the action's UID is nullopt.
    expect(!action->uid);
  };

  "latest bound notification supersedes an older notification"_test = [] {
    state s;
    s.connected();
    s.console_user_id_changed(true);

    auto request = s.take_console_user_id_changed_request();
    expect(request.has_value());

    // Multiple notifications may arrive while request acceptance is pending.
    // The later UID 501 replaces UID 502, so the action contains UID 501.
    s.core_service_daemon_server_bound(uid_t(502));
    s.core_service_daemon_server_bound(uid_t(501));
    s.console_user_id_changed_request_succeeded(*request);

    auto action = s.take_core_service_daemon_server_bound_action();
    expect(action.has_value());
    expect(action->uid == std::optional<uid_t>(501));
  };

  return 0;
}
