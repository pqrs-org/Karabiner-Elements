#include "dispatcher_utility.hpp"
#include "monitor/process_codesign_monitor.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>
#include <condition_variable>
#include <mutex>

namespace {
class team_id_provider final {
public:
  team_id_provider(std::optional<pqrs::osx::codesign::team_id> team_id) : team_id_(team_id) {
  }

  std::optional<pqrs::osx::codesign::team_id> get() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = team_id_;
    ++call_count_;
    condition_variable_.notify_all();
    return result;
  }

  void set(std::optional<pqrs::osx::codesign::team_id> team_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    team_id_ = team_id;
  }

  bool wait_for_call_count(size_t call_count) {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_variable_.wait_for(lock,
                                        std::chrono::seconds(5),
                                        [this, call_count] {
                                          return call_count_ >= call_count;
                                        });
  }

private:
  std::mutex mutex_;
  std::condition_variable condition_variable_;
  std::optional<pqrs::osx::codesign::team_id> team_id_;
  size_t call_count_ = 0;
};

class notification final {
public:
  void notify() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++count_;
    condition_variable_.notify_all();
  }

  bool wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_variable_.wait_for(lock,
                                        std::chrono::seconds(5),
                                        [this] {
                                          return count_ > 0;
                                        });
  }

  size_t count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable condition_variable_;
  size_t count_ = 0;
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  "three consecutive failures"_test = [] {
    auto expected_team_id = pqrs::osx::codesign::team_id("TEAMID");
    team_id_provider provider(expected_team_id);
    notification invalidated;

    {
      krbn::process_codesign_monitor monitor(
          [&] {
            return provider.get();
          },
          std::chrono::hours(1),
          3);

      monitor.invalidated.connect([&] {
        invalidated.notify();
      });

      provider.set(std::nullopt);
      monitor.async_manual_check();
      monitor.async_manual_check();
      expect(provider.wait_for_call_count(3));

      // Entering the next provider call means that the preceding checks have completed.
      provider.set(expected_team_id);
      monitor.async_manual_check();
      expect(provider.wait_for_call_count(4));
      expect(invalidated.count() == 0_u);

      // The successful check above resets the consecutive failure count.
      provider.set(std::nullopt);
      monitor.async_manual_check();
      monitor.async_manual_check();
      expect(provider.wait_for_call_count(6));

      provider.set(expected_team_id);
      monitor.async_manual_check();
      expect(provider.wait_for_call_count(7));
      expect(invalidated.count() == 0_u);

      provider.set(std::nullopt);
      monitor.async_manual_check();
      monitor.async_manual_check();
      monitor.async_manual_check();
      expect(invalidated.wait());

      // Notify only once, even if another check is requested.
      monitor.async_manual_check();
    }

    expect(invalidated.count() == 1_u);
  };

  "unsigned process"_test = [] {
    krbn::process_codesign_monitor monitor(
        [] {
          return std::optional<pqrs::osx::codesign::team_id>();
        },
        std::chrono::milliseconds(10),
        3);

    expect(!monitor.active());
  };

  "timer"_test = [] {
    auto expected_team_id = pqrs::osx::codesign::team_id("TEAMID");
    team_id_provider provider(expected_team_id);
    notification invalidated;

    krbn::process_codesign_monitor monitor(
        [&] {
          return provider.get();
        },
        std::chrono::milliseconds(10),
        3);

    monitor.invalidated.connect([&] {
      invalidated.notify();
    });

    provider.set(std::nullopt);
    monitor.async_start();
    expect(invalidated.wait());
    expect(invalidated.count() == 1_u);
  };

  return 0;
}
