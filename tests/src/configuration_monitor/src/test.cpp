#include "dispatcher_utility.hpp"
#include "monitor/configuration_monitor.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>
#include <future>

namespace {
class test_configuration_monitor final {
public:
  test_configuration_monitor(std::optional<std::string> user_core_configuration_file_path = "target/user.json") {
    configuration_monitor_ = std::make_unique<krbn::configuration_monitor>(user_core_configuration_file_path,
                                                                           geteuid(),
                                                                           krbn::core_configuration::error_handling::loose,
                                                                           "target/system.json");

    configuration_monitor_->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
      ++count_;
      if (auto c = weak_core_configuration.lock()) {
        last_core_configuration_ = c;
      }
    });

    configuration_monitor_->load_state_changed.connect([this](auto load_state) {
      last_load_state_ = load_state;
    });

    configuration_monitor_->async_start();

    wait();
  }

  [[nodiscard]] size_t get_count() const {
    return count_;
  }

  [[nodiscard]] std::shared_ptr<const krbn::core_configuration::core_configuration> get_last_core_configuration() const {
    return last_core_configuration_;
  }

  [[nodiscard]] std::optional<krbn::core_configuration::core_configuration::load_state> get_last_load_state() const {
    return last_load_state_;
  }

  [[nodiscard]] std::string get_selected_profile_name() const {
    return last_core_configuration_->get_selected_profile().get_name();
  }

  void wait(std::chrono::milliseconds duration = std::chrono::milliseconds(500)) const {
    std::this_thread::sleep_for(duration);
  }

private:
  std::unique_ptr<krbn::configuration_monitor> configuration_monitor_;
  size_t count_{0};
  std::shared_ptr<const krbn::core_configuration::core_configuration> last_core_configuration_;
  std::optional<krbn::core_configuration::core_configuration::load_state> last_load_state_;
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  "configuration_monitor retries unchanged files after access is restored"_test = [] {
    // root bypasses discretionary read permissions.
    if (geteuid() == 0) {
      return;
    }
    std::filesystem::create_directories("target");
    const auto path = std::filesystem::path("target/retry.json");
    std::ofstream(path) << "{}";
    expect(chmod(path.c_str(), 0000) == 0);
    {
      krbn::configuration_monitor monitor(path.string(), geteuid(),
                                          krbn::core_configuration::error_handling::loose,
                                          "target/unused-system.json");
      std::promise<void> failed;
      std::promise<void> loaded;
      std::atomic<size_t> updates{0};
      monitor.load_state_changed.connect([&](auto state) {
        if (state == krbn::core_configuration::core_configuration::load_state::permission_error) {
          failed.set_value();
        }
      });
      monitor.core_configuration_updated.connect([&](auto configuration) {
        if (++updates == 1) {
          expect(!configuration.expired());
          loaded.set_value();
        }
      });
      // Do not start file watching: recovery must come from the explicit retry,
      // without depending on a filesystem event or a change to the JSON contents.
      monitor.async_retry_failed_load();
      expect(failed.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
      expect(updates.load() == 0_u);
      expect(chmod(path.c_str(), 0600) == 0);
      monitor.async_retry_failed_load();
      expect(loaded.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);

      // Further retries must not publish a replacement for an already loaded configuration.
      monitor.async_retry_failed_load();
      std::promise<void> drained;
      monitor.enqueue_to_dispatcher([&] { drained.set_value(); });
      expect(drained.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
      expect(updates.load() == 1_u);
    }
    std::filesystem::remove(path);
  };

  "configuration_monitor rejects FIFO and oversized files"_test = [] {
    std::filesystem::create_directories("target");
    std::filesystem::remove("target/user.json");
    std::filesystem::remove("target/system.json");
    expect(mkfifo("target/user.json", 0600) == 0);
    {
      test_configuration_monitor monitor;
      expect(monitor.get_count() == 0);
      expect(monitor.get_last_load_state() == krbn::core_configuration::core_configuration::load_state::other_error);
    }
    std::filesystem::remove("target/user.json");
    {
      std::ofstream output("target/user.json");
      output << "{}";
    }
    std::filesystem::resize_file("target/user.json", krbn::core_configuration::core_configuration::max_configuration_file_size + 1);
    {
      test_configuration_monitor monitor;
      expect(monitor.get_count() == 0);
      expect(monitor.get_last_load_state() == krbn::core_configuration::core_configuration::load_state::other_error);
    }
    std::filesystem::remove("target/user.json");
  };

  "configuration_monitor"_test = [] {
    {
      system("rm -rf target");
      system("mkdir -p target");
      system("echo '{}' > target/user.json");

      test_configuration_monitor monitor;

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "Default profile");

      // ============================================================
      // Update user.json
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 2);
      expect(monitor.get_selected_profile_name() == "user1");

      // ============================================================
      // Update system.json (ignored since user.json exists.)
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"system1\",\"selected\":true}]}' > target/system.json");

      monitor.wait();

      expect(monitor.get_count() == 2);
      expect(monitor.get_selected_profile_name() == "user1");

      // ============================================================
      // Remove user.json (load system.json)
      // ============================================================

      system("rm target/user.json");

      monitor.wait(std::chrono::milliseconds(1500));

      expect(monitor.get_count() == 3);
      expect(monitor.get_selected_profile_name() == "system1");

      // ============================================================
      // Remove system.json (load default configuration)
      // ============================================================

      std::cout << "rm target/system.json" << std::endl;
      system("rm target/system.json");

      monitor.wait(std::chrono::milliseconds(1500));

      expect(monitor.get_count() == 4);
      expect(monitor.get_selected_profile_name() == "Default profile");

      // ============================================================
      // Update system.json
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"system2\",\"selected\":true}]}' > target/system.json");

      monitor.wait();

      expect(monitor.get_count() == 5);
      expect(monitor.get_selected_profile_name() == "system2");

      // ============================================================
      // Update user.json
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"user2\",\"selected\":true}]}' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 6);
      expect(monitor.get_selected_profile_name() == "user2");
    }

    // ============================================================
    // There are not user.json and system.json at start
    // ============================================================

    {
      system("rm -rf target");
      system("mkdir -p target");

      test_configuration_monitor monitor;

      expect(monitor.get_count() == 1);
      expect(monitor.get_last_core_configuration()->get_global_configuration().get_show_in_menu_bar() == true);

      // ============================================================
      // Update user.json
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 2);
      expect(monitor.get_selected_profile_name() == "user1");
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

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "user1");
    }

    // ============================================================
    // Monitor only system.json
    // ============================================================

    {
      system("rm -rf target");
      system("mkdir -p target");
      system("echo '{\"profiles\":[{\"name\":\"system1\",\"selected\":true}]}' > target/system.json");

      test_configuration_monitor monitor(std::nullopt);

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "system1");

      // user.json is not monitored in system-only mode.
      system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "system1");

      system("rm target/system.json");

      monitor.wait(std::chrono::milliseconds(1500));

      expect(monitor.get_count() == 2);
      expect(monitor.get_selected_profile_name() == "Default profile");
    }

    // ============================================================
    // Broken json
    // ============================================================

    {
      system("rm -rf target");
      system("mkdir -p target");
      system("echo '[' > target/user.json");

      test_configuration_monitor monitor;

      expect(monitor.get_count() == 0);
      expect(monitor.get_last_core_configuration() == nullptr);
      expect(monitor.get_last_load_state() == krbn::core_configuration::core_configuration::load_state::json_error);

      // ============================================================
      // Update user.json
      // ============================================================

      system("echo '{\"profiles\":[{\"name\":\"user1\",\"selected\":true}]}' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "user1");
      expect(monitor.get_last_load_state() == krbn::core_configuration::core_configuration::load_state::loaded);

      // ============================================================
      // Break user.json (ignored)
      // ============================================================

      system("echo '[' > target/user.json");

      monitor.wait();

      expect(monitor.get_count() == 1);
      expect(monitor.get_selected_profile_name() == "user1");
      expect(monitor.get_last_load_state() == krbn::core_configuration::core_configuration::load_state::json_error);
    }
  };

  return 0;
}
