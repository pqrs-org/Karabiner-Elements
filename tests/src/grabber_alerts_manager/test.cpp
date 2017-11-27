#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "grabber_alerts_manager.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("set_alert") {
  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager0.json");
  krbn::grabber_alerts_manager::save_to_file();

  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager1.json");
  krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, true);

  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager2.json");
  krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, false);
}
