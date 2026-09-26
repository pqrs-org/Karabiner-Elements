#include "../../../src/share/caps_lock_led_override_manager.hpp"
#include <boost/ut.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/thread_wait.hpp>
#include <vector>

namespace {
class manager_test_context final {
public:
  manager_test_context()
      : manager_(pqrs::make_weak(dispatcher_)) {
    time_source_->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

    manager_.override_changed.connect([this](auto&& value) {
      changes_.push_back(value);
    });
  }

  krbn::caps_lock_led_override_manager& get_manager() {
    return manager_;
  }

  const std::vector<std::optional<krbn::led_state>>& get_changes() const {
    return changes_;
  }

  void flush_immediate_dispatcher_jobs(std::size_t rounds = 4) {
    for (std::size_t i = 0; i < rounds; ++i) {
      auto wait = pqrs::make_thread_wait();

      manager_.enqueue_to_dispatcher([wait] {
        wait->notify();
      });

      wait->wait_notice();
    }
  }

private:
  pqrs::not_null_shared_ptr_t<pqrs::dispatcher::pseudo_time_source> time_source_ = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
  pqrs::not_null_shared_ptr_t<pqrs::dispatcher::dispatcher> dispatcher_ = std::make_shared<pqrs::dispatcher::dispatcher>(time_source_.get());
  krbn::caps_lock_led_override_manager manager_;
  std::vector<std::optional<krbn::led_state>> changes_;
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "caps_lock_led_override_manager initial state"_test = [] {
    auto c = manager_test_context();

    expect(c.get_manager().get_override() == std::nullopt);
    expect(c.get_changes().empty());
  };

  "caps_lock_led_override_manager on and off"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_override(krbn::caps_lock_led_value::on);
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_override() == krbn::led_state::on);
    expect(c.get_changes().size() == 1_ul);
    expect(c.get_changes().back() == krbn::led_state::on);

    c.get_manager().async_set_override(krbn::caps_lock_led_value::off);
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_override() == krbn::led_state::off);
    expect(c.get_changes().size() == 2_ul);
    expect(c.get_changes().back() == krbn::led_state::off);
  };

  "caps_lock_led_override_manager auto removes the override"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_override(krbn::caps_lock_led_value::on);
    c.get_manager().async_set_override(krbn::caps_lock_led_value::automatic);
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_override() == std::nullopt);
    expect(c.get_changes().size() == 2_ul);
    expect(c.get_changes().back() == std::nullopt);
  };

  "caps_lock_led_override_manager async_clear_override"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_override(krbn::caps_lock_led_value::off);
    c.get_manager().async_clear_override();
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_override() == std::nullopt);
    expect(c.get_changes().size() == 2_ul);
    expect(c.get_changes().back() == std::nullopt);
  };

  "caps_lock_led_override_manager does not signal unchanged values"_test = [] {
    auto c = manager_test_context();

    // `auto` while no override is set must not emit a signal.
    c.get_manager().async_set_override(krbn::caps_lock_led_value::automatic);
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_changes().empty());

    c.get_manager().async_set_override(krbn::caps_lock_led_value::on);
    c.get_manager().async_set_override(krbn::caps_lock_led_value::on);
    c.get_manager().async_set_override(krbn::caps_lock_led_value::on);
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_override() == krbn::led_state::on);
    expect(c.get_changes().size() == 1_ul);
  };

  return 0;
}
