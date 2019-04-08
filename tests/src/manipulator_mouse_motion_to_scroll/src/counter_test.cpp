#include <catch2/catch.hpp>
#include <iostream>

#include "manipulator/manipulators/mouse_motion_to_scroll/counter.hpp"

using namespace krbn::manipulator::manipulators::mouse_motion_to_scroll;

class counter_test final : pqrs::dispatcher::extra::dispatcher_client {
public:
  counter_test(std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source,
               std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
               const counter_parameters& parameters) : dispatcher_client(weak_dispatcher),
                                                       time_source_(time_source),
                                                       counter_(weak_dispatcher,
                                                                parameters),
                                                       last_ms_(0) {
    counter_.scroll_event_arrived.connect([](auto&& pointing_motion) {
      std::cout << nlohmann::json(pointing_motion) << std::endl;
    });
  }

  ~counter_test(void) {
    detach_from_dispatcher();
  }

  void update(int x, int y, std::chrono::milliseconds duration) {
    counter_.update(krbn::pointing_motion(x, y, 0, 0),
                    krbn::absolute_time_point(0) +
                        pqrs::osx::chrono::make_absolute_time_duration(duration));
  }

  void set_now(int ms) {
    while (last_ms_ < ms) {
      last_ms_ += 10;
      // std::cout << "now:" << last_ms_ << std::endl;
      auto now = pqrs::dispatcher::time_point(std::chrono::milliseconds(last_ms_));
      auto wait = pqrs::make_thread_wait();

      enqueue_to_dispatcher(
          [wait] {
            wait->notify();
          },
          now);

      time_source_->set_now(now);

      wait->wait_notice();
    }
  }

private:
  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source_;
  counter counter_;
  int last_ms_;
};

TEST_CASE("counter::update") {
  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  {
    counter_parameters parameters;
    counter_test counter_test(time_source,
                              dispatcher,
                              parameters);

    counter_test.update(5, 0, std::chrono::milliseconds(0));

    counter_test.set_now(50);
    counter_test.update(0, 10, std::chrono::milliseconds(50));

    counter_test.set_now(100);
    counter_test.update(0, 5, std::chrono::milliseconds(100));

    counter_test.set_now(150);
    counter_test.update(0, 5, std::chrono::milliseconds(150));

    counter_test.set_now(200);
    counter_test.update(0, 2, std::chrono::milliseconds(200));

    counter_test.set_now(250);
    counter_test.update(0, 2, std::chrono::milliseconds(250));

    counter_test.set_now(300);
    counter_test.update(0, 1, std::chrono::milliseconds(300));

    counter_test.set_now(600);
    counter_test.update(5, 1, std::chrono::milliseconds(600));

    counter_test.set_now(650);
    counter_test.update(10, 1, std::chrono::milliseconds(650));

    counter_test.set_now(700);
    counter_test.update(5, 1, std::chrono::milliseconds(700));

    for (int i = 1000; i < 1150; i += 10) {
      counter_test.set_now(i);
      counter_test.update(0, 20, std::chrono::milliseconds(i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    for (int i = 4000; i < 4150; i += 10) {
      counter_test.set_now(i);
      counter_test.update(0, 20, std::chrono::milliseconds(i));
    }
    for (int i = 4200; i < 6000; i += 50) {
      counter_test.set_now(i);
      counter_test.update(0, 5, std::chrono::milliseconds(i));
    }

    counter_test.set_now(4500);
    counter_test.update(0, -1, std::chrono::milliseconds(2500));

    counter_test.set_now(5000);
  }

  dispatcher->terminate();
}
