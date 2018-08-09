#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "grabber_alerts_monitor.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("grabber_alerts_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");

    std::string file_path = "target/karabiner_grabber_alerts.json";
    krbn::grabber_alerts_monitor grabber_alerts_monitor;

    boost::optional<std::string> last_alerts;

    grabber_alerts_monitor.alerts_changed.connect([&](auto&& alerts) {
      last_alerts = alerts.dump();
    });

    grabber_alerts_monitor.start(file_path);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!last_alerts);

    // ========================================
    // Empty alerts
    // ========================================

    {
      last_alerts = boost::none;

      auto json = nlohmann::json::object({{"alerts", nlohmann::json::array()}});

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_alerts);
      REQUIRE(*last_alerts == json["alerts"].dump());
    }

    // ========================================
    // Alerts
    // ========================================

    {
      last_alerts = boost::none;

      auto json = nlohmann::json::object({{"alerts", nlohmann::json::array({"example"})}});

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_alerts);
      REQUIRE(*last_alerts == json["alerts"].dump());
    }

    // ========================================
    // Broken json
    // ========================================

    {
      last_alerts = boost::none;

      system(fmt::format("echo '[' > {0}", file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(!last_alerts);
    }
  }
}
