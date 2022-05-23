#include "types.hpp"
#include <boost/ut.hpp>

void run_mouse_key_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "mouse_key"_test = [] {
    {
      krbn::mouse_key mouse_key(10, 20, 30, 40, 1.5);
      expect(mouse_key.get_x() == 10);
      expect(mouse_key.get_y() == 20);
      expect(mouse_key.get_vertical_wheel() == 30);
      expect(mouse_key.get_horizontal_wheel() == 40);
      expect(mouse_key.get_speed_multiplier() == 1.5_d);

      auto json = nlohmann::json::object({
          {"x", 10},
          {"y", 20},
          {"vertical_wheel", 30},
          {"horizontal_wheel", 40},
          {"speed_multiplier", 1.5},
      });

      expect(nlohmann::json(mouse_key) == json);
    }
    {
      auto json = nlohmann::json::object({
          {"x", 10},
          {"y", 20},
          {"vertical_wheel", 30},
          {"horizontal_wheel", 40},
          {"speed_multiplier", 0.5},
      });

      auto mouse_key = json.get<krbn::mouse_key>();

      expect(mouse_key.get_x() == 10);
      expect(mouse_key.get_y() == 20);
      expect(mouse_key.get_vertical_wheel() == 30);
      expect(mouse_key.get_horizontal_wheel() == 40);
      expect(mouse_key.get_speed_multiplier() == 0.5_d);
    }
    {
      krbn::mouse_key mouse_key1(10, 20, 30, 40, 1.5);
      krbn::mouse_key mouse_key2(1, 2, 3, 4, 2.0);
      krbn::mouse_key mouse_key = mouse_key1 + mouse_key2;
      expect(mouse_key.get_x() == 11);
      expect(mouse_key.get_y() == 22);
      expect(mouse_key.get_vertical_wheel() == 33);
      expect(mouse_key.get_horizontal_wheel() == 44);
      expect(mouse_key.get_speed_multiplier() == 3.0_d);
    }
    {
      expect(krbn::mouse_key(0, 0, 0, 0, 1.0).is_zero());
      expect(!krbn::mouse_key(1, 0, 0, 0, 1.0).is_zero());
      expect(!krbn::mouse_key(0, 2, 0, 0, 1.0).is_zero());
      expect(!krbn::mouse_key(0, 0, 3, 0, 1.0).is_zero());
      expect(!krbn::mouse_key(0, 0, 0, 4, 1.0).is_zero());
    }
  };

  "mouse_key operators"_test = [] {
    krbn::mouse_key mouse_key1(10, 20, 30, 40, 1.0);
    krbn::mouse_key mouse_key2 = mouse_key1;
    krbn::mouse_key mouse_key3(1, 2, 3, 4, 2.0);
    expect(mouse_key1 == mouse_key2);
    expect(mouse_key1 != mouse_key3);
  };
}
