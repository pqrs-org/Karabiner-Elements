#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "monitor/configuration_monitor.hpp"

namespace {
class test_configuration_monitor final {
public:
  test_configuration_monitor(void) : count_(0) {
    configuration_monitor_ = std::make_unique<krbn::configuration_monitor>("target/user.json",
                                                                           "target/system.json");

    configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
      ++count_;
      if (auto c = weak_core_configuration.lock()) {
        last_core_configuration_ = c;
      }
    });

    configuration_monitor_->async_start();

    wait();
  }

  size_t get_count(void) const {
    return count_;
  }

  std::shared_ptr<const krbn::core_configuration::core_configuration> get_last_core_configuration(void) const {
    return last_core_configuration_;
  }

  std::string get_selected_profile_name(void) const {
    return last_core_configuration_->get_selected_profile().get_name();
  }

  void wait(void) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

private:
  std::unique_ptr<krbn::configuration_monitor> configuration_monitor_;
  size_t count_;
  std::shared_ptr<const krbn::core_configuration::core_configuration> last_core_configuration_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("configuration_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '{}' > target/user.json");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_selected_profile_name() == "Default profile");

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_selected_profile_name() == "user1");

    // ============================================================
    // Update system.json (ignored since user.json exists.)
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"system1\",\"selected\":true}]}' > target/system.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_selected_profile_name() == "user1");

    // ============================================================
    // Remove user.json (load system.json)
    // ============================================================

    system("rm target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_selected_profile_name() == "system1");

    // ============================================================
    // Remove system.json (ignored)
    // ============================================================

    std::cout << "rm target/system.json" << std::endl;
    system("rm target/system.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_selected_profile_name() == "system1");

    // ============================================================
    // Update system.json
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"system2\",\"selected\":true}]}' > target/system.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 4);
    REQUIRE(monitor.get_selected_profile_name() == "system2");

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"user2\",\"selected\":true}]}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 5);
    REQUIRE(monitor.get_selected_profile_name() == "user2");
  }

  // ============================================================
  // There are not user.json and system.json at start
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_selected_profile_name() == "user1");
  }

  // ============================================================
  // There are both user.json and system.json at start
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '{\"profiles\":[{\"name\":\"system1\",\"selected\":true}]}' > target/system.json");
    system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_selected_profile_name() == "user1");
  }

  // ============================================================
  // Broken json
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '[' > target/user.json");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_selected_profile_name() == "Default profile");

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_selected_profile_name() == "user1");

    // ============================================================
    // Break user.json (ignored)
    // ============================================================

    system("echo '[' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_selected_profile_name() == "user1");
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
