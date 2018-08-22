#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "configuration_monitor.hpp"
#include "thread_utility.hpp"

namespace {
class test_configuration_monitor final {
public:
  test_configuration_monitor(void) : count_(0) {
    configuration_monitor_ = std::make_unique<krbn::configuration_monitor>("target/user.json",
                                                                           "target/system.json");

    configuration_monitor_->core_configuration_updated.connect([this](auto&& core_configuration) {
      ++count_;
      last_core_configuration_ = core_configuration;
    });

    configuration_monitor_->async_start();

    wait();
  }

  size_t get_count(void) const {
    return count_;
  }

  std::shared_ptr<const krbn::core_configuration> get_last_core_configuration(void) const {
    return last_core_configuration_;
  }

  void wait(void) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

private:
  std::unique_ptr<krbn::configuration_monitor> configuration_monitor_;
  size_t count_;
  std::shared_ptr<const krbn::core_configuration> last_core_configuration_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("configuration_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '{}' > target/user.json");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);

    // ============================================================
    // Update system.json (ignored since user.json exists.)
    // ============================================================

    system("echo '{\"global\":{\"show_in_menu_bar\":true}}' > target/system.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);

    // ============================================================
    // Remove user.json (ignored file removal)
    // ============================================================

    system("rm target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);

    // ============================================================
    // Update system.json
    // ============================================================

    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/system.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 4);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"global\":{\"show_in_menu_bar\":true}}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 5);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);
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

    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);
  }

  // ============================================================
  // There are both user.json and system.json at start
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/system.json");
    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/user.json");

    test_configuration_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);
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
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);

    // ============================================================
    // Update user.json
    // ============================================================

    system("echo '{\"global\":{\"show_in_menu_bar\":false}}' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);

    // ============================================================
    // Break user.json (ignored)
    // ============================================================

    system("echo '[' > target/user.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == false);
  }
}
