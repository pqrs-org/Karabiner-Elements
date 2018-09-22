#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "dispatcher.hpp"

TEST_CASE("object_id") {
  {
    krbn::dispatcher::object_id object_id1(krbn::dispatcher::object_id::make_new_object_id());
    krbn::dispatcher::object_id object_id2(krbn::dispatcher::object_id::make_new_object_id());

    REQUIRE(object_id1.get() == 1);
    REQUIRE(object_id2.get() == 2);
    REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 2);
  }

  REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 0);

  {
    krbn::dispatcher::object_id object_id3(krbn::dispatcher::object_id::make_new_object_id());
    krbn::dispatcher::object_id object_id4(krbn::dispatcher::object_id::make_new_object_id());

    REQUIRE(object_id3.get() == 3);
    REQUIRE(object_id4.get() == 4);
    REQUIRE(krbn::dispatcher::object_id::active_object_id_count() == 2);
  }
}

TEST_CASE("dispatcher") {
  std::cout << "dispatcher" << std::endl;

  {
    for (int i = 0; i < 10000; ++i) {
      krbn::dispatcher::dispatcher d;
      d.terminate();
    }
  }

  {
    size_t count = 0;

    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    REQUIRE(d.is_dispatcher_thread() == false);

    for (int i = 0; i < 10000; ++i) {
      d.enqueue(
          object_id,
          [&d, &count, i] {
            ++count;
            if (i % 1000 == 0) {
              std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            REQUIRE(d.is_dispatcher_thread() == true);
          });
    }

    REQUIRE(count < 10000);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    REQUIRE(count == 10000);

    d.detach(object_id);
    d.terminate();
  }
}

TEST_CASE("dispatcher.preserve_the_order_of_entries") {
  std::cout << "dispatcher.preserve_the_order_of_entries" << std::endl;

  {
    std::string text;

    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          text += "a";
        });
    d.enqueue(
        object_id,
        [&] {
          text += "b";
        });
    d.enqueue(
        object_id,
        [&] {
          text += "c";
        });

    krbn::thread_utility::wait w;
    d.enqueue(
        object_id,
        [&w] {
          w.notify();
        });
    w.wait_notice();

    d.detach(object_id);
    d.terminate();

    REQUIRE(text == "abc");
  }
}

TEST_CASE("dispatcher.run_enqueued_functions_in_the_destructor") {
  std::cout << "dispatcher.run_enqueued_functions_in_the_destructor" << std::endl;

  {
    size_t count = 0;

    {
      krbn::dispatcher::dispatcher d;

      auto object_id = krbn::dispatcher::object_id::make_new_object_id();
      d.attach(object_id);

      for (int i = 0; i < 10000; ++i) {
        d.enqueue(
            object_id,
            [&count, i] {
              ++count;
              if (i % 1000 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              }
            });
      }

      d.terminate();
    }

    REQUIRE(count == 10000);
  }

  // Ignore `enqueue` after `terminate`.

  {
    size_t count = 0;

    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    d.terminate();

    d.enqueue(
        object_id,
        [&count] {
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(count == 0);
  }
}

TEST_CASE("dispatcher.detach") {
  std::cout << "dispatcher.detach" << std::endl;

  {
    size_t count = 0;

    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });

    d.enqueue(
        object_id,
        [&] {
          ++count;
        });

    // `detach` cancel enqueued functions.

    d.detach(object_id);

    REQUIRE(count == 0);

    d.terminate();
  }
}

TEST_CASE("dispatcher.wait_current_running_function_in_detach") {
  std::cout << "dispatcher.wait_current_running_function_in_detach" << std::endl;

  {
    size_t count = 0;

    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&count] {
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    d.detach(object_id);

    REQUIRE(count == 1);

    d.terminate();
  }

  // Call `detach` in the dispatcher thread.

  {
    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          d.detach(object_id);
        });

    d.terminate();
  }
}

TEST_CASE("dispatcher.terminate") {
  std::cout << "dispatcher.terminate" << std::endl;

  // Call `terminate` in the dispatcher thread.

  {
    krbn::dispatcher::dispatcher d;

    auto object_id = krbn::dispatcher::object_id::make_new_object_id();
    d.attach(object_id);

    krbn::thread_utility::wait w;

    d.enqueue(
        object_id,
        [&] {
          d.terminate();
          w.notify();
        });

    w.wait_notice();
  }
}
