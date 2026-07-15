#include "dispatcher_utility.hpp"
#include "monitor/process_codesign_monitor.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace {
class notification final {
public:
  void notify() {
    std::lock_guard<std::mutex> lock(mutex_);
    notified_ = true;
    condition_variable_.notify_all();
  }

  bool wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_variable_.wait_for(lock,
                                        std::chrono::seconds(10),
                                        [this] {
                                          return notified_;
                                        });
  }

private:
  std::mutex mutex_;
  std::condition_variable condition_variable_;
  bool notified_ = false;
};

class child_process final {
public:
  child_process() = default;

  child_process(const child_process&) = delete;

  ~child_process() {
    if (pid_ > 0) {
      kill(pid_, SIGTERM);
      waitpid(pid_, nullptr, 0);
    }
  }

  bool start(const std::filesystem::path& executable_path) {
    int pipe_file_descriptors[2];
    if (pipe(pipe_file_descriptors) != 0) {
      return false;
    }

    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_addclose(&file_actions, pipe_file_descriptors[0]);
    posix_spawn_file_actions_adddup2(&file_actions, pipe_file_descriptors[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&file_actions, pipe_file_descriptors[1]);

    auto executable_path_string = executable_path.string();
    char* arguments[] = {
        executable_path_string.data(),
        nullptr,
    };

    auto error = posix_spawn(&pid_,
                             executable_path_string.c_str(),
                             &file_actions,
                             nullptr,
                             arguments,
                             environ);

    posix_spawn_file_actions_destroy(&file_actions);
    close(pipe_file_descriptors[1]);

    char ready = 0;
    auto ready_size = read(pipe_file_descriptors[0], &ready, sizeof(ready));
    close(pipe_file_descriptors[0]);

    if (error != 0 || ready_size != 1 || ready != 'A') {
      if (pid_ > 0) {
        kill(pid_, SIGTERM);
        waitpid(pid_, nullptr, 0);
        pid_ = 0;
      }
      return false;
    }

    return true;
  }

  [[nodiscard]] pid_t pid() const {
    return pid_;
  }

private:
  pid_t pid_ = 0;
};

class temporary_directory final {
public:
  temporary_directory(const std::filesystem::path& base_directory_path) {
    std::filesystem::create_directories(base_directory_path);

    auto path = base_directory_path / "karabiner_process_codesign_monitor.XXXXXX";
    auto path_string = path.string();
    if (auto p = mkdtemp(path_string.data())) {
      path_ = p;
    }
  }

  ~temporary_directory() {
    std::error_code error_code;
    std::filesystem::remove_all(path_, error_code);
  }

  [[nodiscard]] const std::filesystem::path& path() const {
    return path_;
  }

private:
  std::filesystem::path path_;
};

} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  "replace a running app on disk"_test = [] {
    auto fixtures_directory_path = std::filesystem::path(PROCESS_CODESIGN_TEST_FIXTURES_DIR);
    auto fixture_a_path = fixtures_directory_path / "process_codesign_test_helper_a.app";
    auto fixture_b_path = fixtures_directory_path / "process_codesign_test_helper_b.app";

    expect(std::filesystem::exists(fixture_a_path)) << "Run `make prepare-fixtures` to create fixture A.";
    expect(std::filesystem::exists(fixture_b_path)) << "Run `make prepare-fixtures` to create fixture B.";
    if (!std::filesystem::exists(fixture_a_path) ||
        !std::filesystem::exists(fixture_b_path)) {
      return;
    }

    auto fixture_a_team_id = pqrs::osx::codesign::get_signing_information_of_file(fixture_a_path).get_verified_team_id();
    auto fixture_b_team_id = pqrs::osx::codesign::get_signing_information_of_file(fixture_b_path).get_verified_team_id();

    expect(fixture_a_team_id.has_value()) << "Fixture A must have a non-ad-hoc code signature.";
    expect(fixture_b_team_id.has_value()) << "Fixture B must have a non-ad-hoc code signature.";
    expect(fixture_a_team_id == fixture_b_team_id) << "Both fixtures must have the same Team ID.";
    if (!fixture_a_team_id || fixture_a_team_id != fixture_b_team_id) {
      return;
    }

    temporary_directory temporary_directory(fixtures_directory_path.parent_path() / "tmp");
    auto app_path = temporary_directory.path() / "ProcessCodesignTestHelper.app";
    auto executable_path = app_path / "Contents/MacOS/process_codesign_test_helper";
    std::filesystem::copy(fixture_a_path,
                          app_path,
                          std::filesystem::copy_options::recursive);

    child_process child_process;
    expect(child_process.start(executable_path));
    if (child_process.pid() == 0) {
      return;
    }

    notification invalidated;
    krbn::process_codesign_monitor monitor(
        [&child_process] {
          return pqrs::osx::codesign::get_signing_information_of_process(child_process.pid()).get_verified_team_id();
        },
        std::chrono::milliseconds(100),
        3);

    expect(monitor.active());
    monitor.invalidated.connect([&invalidated] {
      invalidated.notify();
    });
    monitor.async_start();

    std::filesystem::remove_all(app_path);
    std::filesystem::copy(fixture_b_path,
                          app_path,
                          std::filesystem::copy_options::recursive);

    expect(invalidated.wait());
  };

  return 0;
}
