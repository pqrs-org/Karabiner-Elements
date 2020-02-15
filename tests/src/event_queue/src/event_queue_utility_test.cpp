#include <catch2/catch.hpp>

#include "test.hpp"

TEST_CASE("utility::make_queue") {
  std::vector<pqrs::osx::iokit_hid_value> hid_values;

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(1000),
                                                     1,
                                                     *krbn::make_hid_usage_page(krbn::key_code::spacebar),
                                                     *krbn::make_hid_usage(krbn::key_code::spacebar)));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(2000),
                                                     0,
                                                     *krbn::make_hid_usage_page(krbn::key_code::spacebar),
                                                     *krbn::make_hid_usage(krbn::key_code::spacebar)));

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(3000),
                                                     1,
                                                     *krbn::make_hid_usage_page(krbn::consumer_key_code::mute),
                                                     *krbn::make_hid_usage(krbn::consumer_key_code::mute)));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(4000),
                                                     0,
                                                     *krbn::make_hid_usage_page(krbn::consumer_key_code::mute),
                                                     *krbn::make_hid_usage(krbn::consumer_key_code::mute)));

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(5000),
                                                     10,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_x));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(5000),
                                                     20,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_y));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(5000),
                                                     30,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_wheel));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(5000),
                                                     40,
                                                     pqrs::osx::iokit_hid_usage_page_consumer,
                                                     pqrs::osx::iokit_hid_usage_consumer_ac_pan));

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(6000),
                                                     -10,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_x));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(6000),
                                                     -20,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_y));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(6000),
                                                     -30,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_wheel));
  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(6000),
                                                     -40,
                                                     pqrs::osx::iokit_hid_usage_page_consumer,
                                                     pqrs::osx::iokit_hid_usage_consumer_ac_pan));

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(7000),
                                                     10,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_x));

  hid_values.emplace_back(pqrs::osx::iokit_hid_value(krbn::absolute_time_point(8000),
                                                     0,
                                                     pqrs::osx::iokit_hid_usage_page_generic_desktop,
                                                     pqrs::osx::iokit_hid_usage_generic_desktop_x));

  auto queue = krbn::event_queue::utility::make_queue(krbn::device_id(1),
                                                      hid_values);
  REQUIRE(queue->get_entries().size() == 8);

  {
    auto& e = queue->get_entries()[0];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(1000));
    REQUIRE(e.get_event().get_key_code() == krbn::key_code::spacebar);
    REQUIRE(e.get_event_type() == krbn::event_type::key_down);
  }
  {
    auto& e = queue->get_entries()[1];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(2000));
    REQUIRE(e.get_event().get_key_code() == krbn::key_code::spacebar);
    REQUIRE(e.get_event_type() == krbn::event_type::key_up);
  }
  {
    auto& e = queue->get_entries()[2];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(3000));
    REQUIRE(e.get_event().get_consumer_key_code() == krbn::consumer_key_code::mute);
    REQUIRE(e.get_event_type() == krbn::event_type::key_down);
  }
  {
    auto& e = queue->get_entries()[3];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(4000));
    REQUIRE(e.get_event().get_consumer_key_code() == krbn::consumer_key_code::mute);
    REQUIRE(e.get_event_type() == krbn::event_type::key_up);
  }
  {
    auto& e = queue->get_entries()[4];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(5000));
    REQUIRE(e.get_event().get_pointing_motion() == krbn::pointing_motion(10, 20, 30, 40));
    REQUIRE(e.get_event_type() == krbn::event_type::single);
  }
  {
    auto& e = queue->get_entries()[5];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(6000));
    REQUIRE(e.get_event().get_pointing_motion() == krbn::pointing_motion(-10, -20, -30, -40));
    REQUIRE(e.get_event_type() == krbn::event_type::single);
  }
  {
    auto& e = queue->get_entries()[6];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(7000));
    REQUIRE(e.get_event().get_pointing_motion() == krbn::pointing_motion(10, 0, 0, 0));
    REQUIRE(e.get_event_type() == krbn::event_type::single);
  }
  {
    auto& e = queue->get_entries()[7];
    REQUIRE(e.get_device_id() == krbn::device_id(1));
    REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(8000));
    REQUIRE(e.get_event().get_pointing_motion() == krbn::pointing_motion(0, 0, 0, 0));
    REQUIRE(e.get_event_type() == krbn::event_type::single);
  }
}

TEST_CASE("utility::insert_device_keys_and_pointing_buttons_are_released_event") {
  krbn::event_queue::event a_event(krbn::key_code::a);
  krbn::event_queue::event b_event(krbn::key_code::b);
  krbn::event_queue::event mute_event(krbn::consumer_key_code::mute);
  krbn::event_queue::event button2_event(krbn::pointing_button::button2);

  {
    // Normal

    auto event_queue = std::make_shared<krbn::event_queue::queue>();
    auto pressed_keys_manager = std::make_shared<krbn::pressed_keys_manager>();

    ENQUEUE_EVENT((*event_queue), 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT((*event_queue), 1, 200, a_event, key_up, a_event);
    ENQUEUE_EVENT((*event_queue), 1, 300, b_event, key_down, b_event);
    ENQUEUE_EVENT((*event_queue), 1, 400, b_event, key_up, b_event);
    ENQUEUE_EVENT((*event_queue), 1, 500, mute_event, key_down, mute_event);
    ENQUEUE_EVENT((*event_queue), 1, 600, mute_event, key_up, mute_event);
    ENQUEUE_EVENT((*event_queue), 1, 700, button2_event, key_down, button2_event);
    ENQUEUE_EVENT((*event_queue), 1, 800, button2_event, key_up, button2_event);

    event_queue = krbn::event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue,
                                                                                                         krbn::device_id(1),
                                                                                                         pressed_keys_manager);

    REQUIRE(event_queue->get_entries().size() == 12); // (2 + 1) * 4

    {
      auto& e = event_queue->get_entries()[0];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_down);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(100));
    }
    {
      auto& e = event_queue->get_entries()[1];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_up);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(200));
    }
    {
      auto& e = event_queue->get_entries()[2];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::device_keys_and_pointing_buttons_are_released);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(200));
    }
    {
      auto& e = event_queue->get_entries()[3];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_down);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(300));
    }

    REQUIRE(pressed_keys_manager->empty());
  }

  {
    // Different device_id

    auto event_queue = std::make_shared<krbn::event_queue::queue>();
    auto pressed_keys_manager = std::make_shared<krbn::pressed_keys_manager>();

    ENQUEUE_EVENT((*event_queue), 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT((*event_queue), 2, 200, a_event, key_up, a_event);

    event_queue = krbn::event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue,
                                                                                                         krbn::device_id(1),
                                                                                                         pressed_keys_manager);

    REQUIRE(event_queue->get_entries().size() == 2);
    REQUIRE(!pressed_keys_manager->empty());
  }

  {
    // Multiple key_down

    auto event_queue = std::make_shared<krbn::event_queue::queue>();
    auto pressed_keys_manager = std::make_shared<krbn::pressed_keys_manager>();

    ENQUEUE_EVENT((*event_queue), 1, 100, a_event, key_down, a_event);
    ENQUEUE_EVENT((*event_queue), 1, 200, b_event, key_down, b_event);
    ENQUEUE_EVENT((*event_queue), 1, 300, b_event, key_up, b_event);
    ENQUEUE_EVENT((*event_queue), 1, 400, a_event, key_up, a_event);

    event_queue = krbn::event_queue::utility::insert_device_keys_and_pointing_buttons_are_released_event(event_queue,
                                                                                                         krbn::device_id(1),
                                                                                                         pressed_keys_manager);

    REQUIRE(event_queue->get_entries().size() == 5);

    {
      auto& e = event_queue->get_entries()[0];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_down);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(100));
    }
    {
      auto& e = event_queue->get_entries()[1];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_down);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(200));
    }
    {
      auto& e = event_queue->get_entries()[2];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_up);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(300));
    }
    {
      auto& e = event_queue->get_entries()[3];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::key_code);
      REQUIRE(e.get_event_type() == krbn::event_type::key_up);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(400));
    }
    {
      auto& e = event_queue->get_entries()[4];
      REQUIRE(e.get_event().get_type() == krbn::event_queue::event::type::device_keys_and_pointing_buttons_are_released);
      REQUIRE(e.get_event_time_stamp().get_time_stamp() == krbn::absolute_time_point(400));
    }

    REQUIRE(pressed_keys_manager->empty());
  }
}
