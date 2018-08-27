#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("timer") {
  std::cout << "timer" << std::endl;

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(100),
        false,
        [&] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    timer.cancel();

    REQUIRE(count == 1);
  }

  // Wait

  {
    size_t count = 0;
    size_t wait_count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    auto thread = std::thread([&] {
      for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        ++wait_count;
      }
    });

    timer.wait();

    REQUIRE(count == 1);
    REQUIRE(wait_count > 1);

    thread.join();
  }

  // Cancel (1)

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    timer.cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(count == 0);
  }

  // Cancel (2)

  {
    size_t count = 0;

    {
      krbn::thread_utility::timer timer(
          std::chrono::milliseconds(50000),
          false,
          [&] {
            ++count;
          });
    }
  }

  // Cancel while wait

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(500),
        false,
        [&] {
          ++count;
        });

    auto thread = std::thread([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      timer.cancel();
    });

    timer.wait();

    REQUIRE(count == 0);

    thread.join();
  }
}

TEST_CASE("timer.repeats") {
  std::cout << "timer.repeats" << std::endl;

  // Cancel while wait

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        std::chrono::milliseconds(100),
        true,
        [&] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    timer.cancel();
    timer.wait();

    REQUIRE(count >= 2);
    REQUIRE(count <= 10);
  }

  {
    size_t count = 0;

    // Call `cancel` in `~timer`.

    {
      krbn::thread_utility::timer timer(
          std::chrono::milliseconds(100),
          true,
          [&] {
            ++count;
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    REQUIRE(count >= 2);
    REQUIRE(count <= 10);
  }

  {
    size_t count = 0;

    // `unset_repeats`

    {
      krbn::thread_utility::timer timer(
          std::chrono::milliseconds(100),
          true,
          [&] {
            ++count;
            if (count == 2) {
              timer.unset_repeats();
            }
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    REQUIRE(count == 2);
  }
}

TEST_CASE("timer.repeats.interval") {
  std::cout << "timer.repeats.interval" << std::endl;

  {
    size_t count = 0;

    krbn::thread_utility::timer timer(
        [](auto&& count) {
          if (count == 0) {
            return std::chrono::milliseconds(0);
          } else {
            return std::chrono::milliseconds(500);
          }
        },
        true,
        [&] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(count == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(count > 1);
  }
}

TEST_CASE("queue") {
  std::cout << "queue" << std::endl;

  {
    size_t count = 0;

    krbn::thread_utility::queue queue;

    for (int i = 0; i < 10000; ++i) {
      queue.push_back([&count, i] {
        ++count;
        if (i % 1000 == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
      });
    }

    REQUIRE(count < 10000);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(count == 10000);
  }

  // Run queued functions in the destructor.

  {
    size_t count = 0;

    {
      krbn::thread_utility::queue queue;

      for (int i = 0; i < 10000; ++i) {
        queue.push_back([&count, i] {
          ++count;
          if (i % 1000 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
          }
        });
      }
    }

    REQUIRE(count == 10000);
  }
}

namespace {
void queue_recursive_function(krbn::thread_utility::queue& queue,
                              size_t& count) {
  ++count;
  if (count < 5) {
    queue.push_back([&queue, &count] {
      queue_recursive_function(queue, count);
    });
  } else if (count == 5) {
    queue.push_back([] {
      std::cout << "queue_recursive_function finished" << std::endl;
    });
  }
}
} // namespace

TEST_CASE("queue.recursive") {
  std::cout << "queue.recursive" << std::endl;

  // Call `push_back` in queue's thread.

  {
    size_t count = 0;

    {
      krbn::thread_utility::queue queue;
      queue.push_back([&] {
        queue_recursive_function(queue, count);
      });
    }

    REQUIRE(count == 5);
  }
}
