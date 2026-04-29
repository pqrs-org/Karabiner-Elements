#include "../../../../src/core/console_user_server/include/console_user_server/settings_window_guidance_manager.hpp"
#include <boost/ut.hpp>
#include <pqrs/thread_wait.hpp>

namespace {
struct core_service_daemon_state_initialization_parameters final {
  bool iohid_listen_event_allowed;
  bool accessibility_process_trusted;
  std::optional<bool> driver_activated = std::nullopt;
};

krbn::settings_window_guidance_context make_guidance_context(std::optional<bool> enabled,
                                                             std::optional<bool> running) {
  auto context = krbn::settings_window_guidance_context();
  context.set_core_daemons_enabled(enabled);
  context.set_core_agents_enabled(enabled);
  context.set_core_daemons_running(running);
  context.set_core_agents_running(running);
  return context;
}

krbn::core_service_daemon_state make_core_service_daemon_state(const core_service_daemon_state_initialization_parameters& parameters) {
  auto permission_check_result = krbn::core_service_permission_check_result();
  permission_check_result.set_iohid_listen_event_allowed(parameters.iohid_listen_event_allowed);
  permission_check_result.set_accessibility_process_trusted(parameters.accessibility_process_trusted);

  auto core_service_daemon_state = krbn::core_service_daemon_state();
  core_service_daemon_state.set_bundle_permission_check_result(permission_check_result);
  core_service_daemon_state.set_driver_activated(parameters.driver_activated);

  return core_service_daemon_state;
}

struct manager_test_context final {
  struct initialization_parameters final {
    std::optional<bool> enabled;
    std::optional<bool> running;
    krbn::console_user_server::settings_window_guidance_manager::launch_settings_handler launch_settings_handler = [] {};
  };

  manager_test_context(const initialization_parameters& parameters)
      : current_context(make_guidance_context(parameters.enabled,
                                              parameters.running)),
        manager(dispatcher, [this] { return current_context; }, parameters.launch_settings_handler) {
    time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));
  }

  void wait_until(std::chrono::milliseconds ms) {
    auto wait = pqrs::make_thread_wait();
    auto when = pqrs::dispatcher::time_point(ms + std::chrono::milliseconds(1));

    time_source->set_now(when);
    boost::ut::expect(manager.enqueue_to_dispatcher(
        [wait] {
          wait->notify();
        },
        when));
    wait->wait_notice();
  }

  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
  std::shared_ptr<pqrs::dispatcher::dispatcher> dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
  krbn::settings_window_guidance_context current_context;
  krbn::console_user_server::settings_window_guidance_manager manager;
};
} // namespace

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "settings_window_guidance_manager services_not_running"_test = [] {
    auto launch_count = 0;
    auto c = manager_test_context(
        manager_test_context::initialization_parameters{
            .enabled = true,
            .running = false,
            .launch_settings_handler = [&launch_count] {
              ++launch_count;
            },
        });

    c.manager.async_start();

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(9000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(12000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::services_not_running);
    expect(launch_count == 1_i);
  };

  "settings_window_guidance_manager services_not_running starts counting when services become enabled"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = false,
        .running = false,
    });
    c.manager.async_start();

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::services);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(30000));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::services);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.current_context.set_core_daemons_enabled(true);
    c.current_context.set_core_agents_enabled(true);

    c.wait_until(std::chrono::milliseconds(33000));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::none);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(39000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(45000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::services_not_running);
  };

  "settings_window_guidance_manager services_not_running ignores short downtime"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.current_context.set_core_daemons_running(false);
    c.current_context.set_core_agents_running(false);

    c.wait_until(std::chrono::milliseconds(5000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.current_context.set_core_daemons_running(true);
    c.current_context.set_core_agents_running(true);

    c.wait_until(std::chrono::milliseconds(12000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);
  };

  "settings_window_guidance_manager setup stays none when services state becomes nullopt"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::none);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.current_context.set_core_daemons_enabled(std::nullopt);
    c.current_context.set_core_agents_enabled(std::nullopt);
    c.current_context.set_core_daemons_running(std::nullopt);
    c.current_context.set_core_agents_running(std::nullopt);

    c.wait_until(std::chrono::milliseconds(3000));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::none);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);
  };

  "settings_window_guidance_manager accessibility setup"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    c.manager.async_update_core_service_daemon_state(make_core_service_daemon_state(core_service_daemon_state_initialization_parameters{
        .iohid_listen_event_allowed = false,
        .accessibility_process_trusted = false,
    }));

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::accessibility);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);
  };

  "settings_window_guidance_manager input_monitoring setup"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    c.manager.async_update_core_service_daemon_state(make_core_service_daemon_state(core_service_daemon_state_initialization_parameters{
        .iohid_listen_event_allowed = false,
        .accessibility_process_trusted = true,
    }));

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::input_monitoring);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);
  };

  "settings_window_guidance_manager driver_extension setup"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    c.manager.async_update_core_service_daemon_state(make_core_service_daemon_state(core_service_daemon_state_initialization_parameters{
        .iohid_listen_event_allowed = true,
        .accessibility_process_trusted = true,
        .driver_activated = false,
    }));

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_setup() == krbn::settings_window_guidance_setup::driver_extension);
    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);
  };

  "settings_window_guidance_manager doctor alert"_test = [] {
    auto c = manager_test_context(manager_test_context::initialization_parameters{
        .enabled = true,
        .running = true,
    });
    c.manager.async_start();
    c.wait_until(std::chrono::milliseconds(0));

    auto core_service_daemon_state = krbn::core_service_daemon_state();
    core_service_daemon_state.set_karabiner_json_parse_error_message("parse error");
    c.manager.async_update_core_service_daemon_state(core_service_daemon_state);

    c.wait_until(std::chrono::milliseconds(0));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::none);

    c.wait_until(std::chrono::milliseconds(3000));

    expect(c.manager.get_guidance_state().get_current_alert() == krbn::settings_window_guidance_alert::doctor);
  };

  return 0;
}
