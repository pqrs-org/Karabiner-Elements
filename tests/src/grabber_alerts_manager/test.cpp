#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "grabber_alerts_manager.hpp"
#include "thread_utility.hpp"
#include <unistd.h>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("set_alert") {
  using namespace std::string_literals;

  std::vector<std::string> file_names{
      "grabber_alerts_manager0.json"s,
      "grabber_alerts_manager1.json"s,
      "grabber_alerts_manager2.json"s,
  };

  for (const auto& file_name : file_names) {
    auto tmp_file_path = "tmp/"s + file_name;
    unlink(tmp_file_path.c_str());
  }

  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager0.json");
  krbn::grabber_alerts_manager::save_to_file();

  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager1.json");
  krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, true);

  krbn::grabber_alerts_manager::enable_json_output("tmp/grabber_alerts_manager2.json");
  krbn::grabber_alerts_manager::set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, false);

  krbn::async_sequential_file_writer::get_instance().wait();

  for (const auto& file_name : file_names) {
    std::ifstream expected_stream("expected/"s + file_name);
    std::ifstream actual_stream("tmp/"s + file_name);

    nlohmann::json expected;
    expected_stream >> expected;

    nlohmann::json actual;
    actual_stream >> actual;

    REQUIRE(expected == actual);
  }
}
