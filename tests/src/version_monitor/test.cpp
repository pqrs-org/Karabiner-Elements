#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "monitor/version_monitor.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("version_monitor") {
  system("rm -rf target");
  system("mkdir -p target");
  system("echo 1.0.0 > target/version");

  {
    krbn::version_monitor version_monitor("target/version");

    std::string last_changed_version;

    version_monitor.changed.connect([&](auto&& version) {
      last_changed_version = version;
    });

    version_monitor.async_start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version.empty());

    // ========================================
    // Update version
    // ========================================

    last_changed_version.clear();

    system("echo 1.1.0 > target/version");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version == "1.1.0");

    // ========================================
    // The callback is not called if the file contents is not changed.
    // ========================================

    last_changed_version.clear();

    system("touch target/version");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version.empty());

    // ========================================
    // `manual_check` does not invoke callback if the version file is not actually changed.
    // ========================================

    last_changed_version.clear();

    version_monitor.async_manual_check();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version.empty());

    // ========================================
    // Self update is ignored by `kFSEventStreamCreateFlagIgnoreSelf`
    // ========================================

    last_changed_version.clear();

    {
      std::ofstream("target/version") << "1.2.0";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version.empty());

    // ========================================
    // `manual_check`
    // ========================================

    last_changed_version.clear();

    version_monitor.async_manual_check();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version == "1.2.0");

    // ========================================
    // Update version again
    // ========================================

    last_changed_version.clear();

    system("echo 1.3.0 > target/version");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(last_changed_version == "1.3.0");
  }
}
