#include "../../../src/share/notification_message_manager.hpp"
#include <boost/ut.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/thread_wait.hpp>

namespace {
class manager_test_context final {
public:
  manager_test_context()
      : manager_(pqrs::make_weak(dispatcher_)) {
    time_source_->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));
  }

  krbn::notification_message_manager& get_manager() {
    return manager_;
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

  void wait_until(std::chrono::milliseconds ms) {
    auto wait = pqrs::make_thread_wait();
    auto when = pqrs::dispatcher::time_point(ms);

    time_source_->set_now(when);
    boost::ut::expect(manager_.enqueue_to_dispatcher(
        [wait] {
          wait->notify();
        },
        when));
    wait->wait_notice();
  }

private:
  pqrs::not_null_shared_ptr_t<pqrs::dispatcher::pseudo_time_source> time_source_ = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
  pqrs::not_null_shared_ptr_t<pqrs::dispatcher::dispatcher> dispatcher_ = std::make_shared<pqrs::dispatcher::dispatcher>(time_source_.get());
  krbn::notification_message_manager manager_;
};

krbn::notification_message make_notification_message(const std::string& text,
                                                     std::chrono::milliseconds duration_milliseconds) {
  auto message = krbn::notification_message();
  message.set_id("test");
  message.set_text(text);
  message.set_duration_milliseconds(duration_milliseconds);
  return message;
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "notification_message_manager duration_milliseconds"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_notification_message(
        make_notification_message("first", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_full_message() == "first");

    c.wait_until(std::chrono::milliseconds(40));
    expect(c.get_manager().get_full_message() == "first");

    c.wait_until(std::chrono::milliseconds(60));
    expect(c.get_manager().get_full_message().empty());
  };

  "notification_message_manager duration reset"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_notification_message(
        make_notification_message("first", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    c.wait_until(std::chrono::milliseconds(40));
    c.get_manager().async_set_notification_message(
        make_notification_message("second", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    expect(c.get_manager().get_full_message() == "second");

    c.wait_until(std::chrono::milliseconds(80));
    expect(c.get_manager().get_full_message() == "second");

    c.wait_until(std::chrono::milliseconds(100));
    expect(c.get_manager().get_full_message().empty());
  };

  "notification_message_manager duration cancel"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_notification_message(
        make_notification_message("first", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    c.wait_until(std::chrono::milliseconds(40));
    c.get_manager().async_set_notification_message(
        make_notification_message("second", std::chrono::milliseconds(0)));
    c.flush_immediate_dispatcher_jobs();

    c.wait_until(std::chrono::milliseconds(60));
    expect(c.get_manager().get_full_message() == "second");
  };

  "notification_message_manager duration reset after clear"_test = [] {
    auto c = manager_test_context();

    c.get_manager().async_set_notification_message(
        make_notification_message("first", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    c.wait_until(std::chrono::milliseconds(20));
    c.get_manager().async_set_notification_message(
        make_notification_message("", std::chrono::milliseconds(0)));
    c.flush_immediate_dispatcher_jobs();
    expect(c.get_manager().get_full_message().empty());

    c.wait_until(std::chrono::milliseconds(30));
    c.get_manager().async_set_notification_message(
        make_notification_message("second", std::chrono::milliseconds(50)));
    c.flush_immediate_dispatcher_jobs();

    c.wait_until(std::chrono::milliseconds(60));
    expect(c.get_manager().get_full_message() == "second");

    c.wait_until(std::chrono::milliseconds(80));
    expect(c.get_manager().get_full_message().empty());
  };

  return 0;
}
