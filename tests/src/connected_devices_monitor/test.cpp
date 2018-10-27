#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "dispatcher_utility.hpp"
#include "monitor/connected_devices_monitor.hpp"

namespace {
class test_connected_devices_monitor final {
public:
  test_connected_devices_monitor(void) : count_(0) {
    connected_devices_monitor_ = std::make_unique<krbn::connected_devices_monitor>("target/devices.json");

    connected_devices_monitor_->connected_devices_updated.connect([this](auto&& weak_connected_devices) {
      ++count_;
      if (auto d = weak_connected_devices.lock()) {
        last_connected_devices_ = d;
      }
    });

    connected_devices_monitor_->async_start();

    wait();
  }

  size_t get_count(void) const {
    return count_;
  }

  std::shared_ptr<const krbn::connected_devices::connected_devices> get_last_connected_devices(void) const {
    return last_connected_devices_;
  }

  void wait(void) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

private:
  std::unique_ptr<krbn::connected_devices_monitor> connected_devices_monitor_;
  size_t count_;
  std::shared_ptr<const krbn::connected_devices::connected_devices> last_connected_devices_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("connected_devices_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '[]' > target/devices.json");

    test_connected_devices_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 0);

    // ============================================================
    // Update devices.json
    // ============================================================

    system("echo '[{\"descriptions\":{\"manufacturer\":\"test1\"}}]' > target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test1");

    // ============================================================
    // Remove devices.json (ignored file removal)
    // ============================================================

    system("rm target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test1");

    // ============================================================
    // Update devices.json
    // ============================================================

    system("echo '[{\"descriptions\":{\"manufacturer\":\"test2\"}}]' > target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test2");
  }

  // ============================================================
  // There are not user.json and system.json at start
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");

    test_connected_devices_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 0);

    // ============================================================
    // Update devices.json
    // ============================================================

    system("echo '[{\"descriptions\":{\"manufacturer\":\"test2\"}}]' > target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test2");
  }

  // ============================================================
  // Broken json
  // ============================================================

  {
    system("rm -rf target");
    system("mkdir -p target");
    system("echo '[' > target/devices.json");

    test_connected_devices_monitor monitor;

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 0);

    // ============================================================
    // Update devices.json
    // ============================================================

    system("echo '[{\"descriptions\":{\"manufacturer\":\"test1\"}}]' > target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test1");

    // ============================================================
    // Break user.json (ignored)
    // ============================================================

    system("echo '[' > target/devices.json");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_connected_devices()->get_devices().size() == 1);
    REQUIRE(monitor.get_last_connected_devices()->get_devices()[0].get_descriptions().get_manufacturer() == "test1");
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
