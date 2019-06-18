#include <catch2/catch.hpp>

#include "monitor/kextd_state_monitor.hpp"

TEST_CASE("kextd_alerts_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");

    std::string file_path = "target/karabiner_kextd_alerts.json";
    krbn::kextd_state_monitor monitor(file_path);

    std::optional<kern_return_t> last_kext_load_result;

    monitor.kext_load_result_changed.connect([&](auto&& result) {
      last_kext_load_result = result;
    });

    monitor.async_start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_kext_load_result == std::nullopt);

    // ========================================
    // Empty json
    // ========================================

    {
      last_kext_load_result = std::nullopt;

      auto json = nlohmann::json::object();

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_kext_load_result == std::nullopt);
    }

    // ========================================
    // kext_load_result
    // ========================================

    {
      last_kext_load_result = std::nullopt;

      auto json = nlohmann::json::object({
          {"kext_load_result", kOSKextReturnSystemPolicy},
      });

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_kext_load_result != std::nullopt);
      REQUIRE(*last_kext_load_result == kOSKextReturnSystemPolicy);
    }

    // ========================================
    // Broken json
    // ========================================

    {
      last_kext_load_result = std::nullopt;

      system(fmt::format("echo '[' > {0}", file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(!last_kext_load_result);
    }
  }
}
