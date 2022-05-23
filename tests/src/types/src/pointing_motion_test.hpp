#include "types.hpp"
#include <boost/ut.hpp>

void run_pointing_motion_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "pointing_motion"_test = [] {
    {
      krbn::pointing_motion pointing_motion;
      expect(pointing_motion.get_x() == 0);
      expect(pointing_motion.get_y() == 0);
      expect(pointing_motion.get_vertical_wheel() == 0);
      expect(pointing_motion.get_horizontal_wheel() == 0);

      nlohmann::json json;
      json["x"] = 0;
      json["y"] = 0;
      json["vertical_wheel"] = 0;
      json["horizontal_wheel"] = 0;
      expect(nlohmann::json(pointing_motion) == json);
    }

    {
      krbn::pointing_motion pointing_motion(1, 2, 3, 4);
      expect(pointing_motion.get_x() == 1);
      expect(pointing_motion.get_y() == 2);
      expect(pointing_motion.get_vertical_wheel() == 3);
      expect(pointing_motion.get_horizontal_wheel() == 4);
    }
  };

  "pointing_motion::setter,getter"_test = [] {
    {
      krbn::pointing_motion pointing_motion;

      pointing_motion.set_x(1);
      pointing_motion.set_y(-1);
      pointing_motion.set_vertical_wheel(2);
      pointing_motion.set_horizontal_wheel(-2);

      expect(pointing_motion.get_x() == 1);
      expect(pointing_motion.get_y() == -1);
      expect(pointing_motion.get_vertical_wheel() == 2);
      expect(pointing_motion.get_horizontal_wheel() == -2);

      nlohmann::json json;
      json["x"] = 1;
      json["y"] = -1;
      json["vertical_wheel"] = 2;
      json["horizontal_wheel"] = -2;
      expect(nlohmann::json(pointing_motion) == json);
    }
  };

  "pointing_motion::from_json"_test = [] {
    // from_json
    {
      nlohmann::json json;
      json["x"] = 10;
      json["y"] = -10;
      json["vertical_wheel"] = 20;
      json["horizontal_wheel"] = -20;

      krbn::pointing_motion pointing_motion(json);
      expect(pointing_motion.get_x() == 10);
      expect(pointing_motion.get_y() == -10);
      expect(pointing_motion.get_vertical_wheel() == 20);
      expect(pointing_motion.get_horizontal_wheel() == -20);
    }
  };

  "pointing_motion::is_zero"_test = [] {
    // is_zero
    {
      krbn::pointing_motion pointing_motion;
      expect(pointing_motion.is_zero());
    }
    {
      krbn::pointing_motion pointing_motion;
      pointing_motion.set_x(1);
      expect(!pointing_motion.is_zero());
    }
    {
      krbn::pointing_motion pointing_motion;
      pointing_motion.set_y(1);
      expect(!pointing_motion.is_zero());
    }
    {
      krbn::pointing_motion pointing_motion;
      pointing_motion.set_vertical_wheel(1);
      expect(!pointing_motion.is_zero());
    }
    {
      krbn::pointing_motion pointing_motion;
      pointing_motion.set_horizontal_wheel(1);
      expect(!pointing_motion.is_zero());
    }
  };

  "pointing_motion::clear"_test = [] {
    // clear
    {
      krbn::pointing_motion pointing_motion;
      pointing_motion.set_x(1);
      pointing_motion.set_y(1);
      pointing_motion.set_vertical_wheel(1);
      pointing_motion.set_horizontal_wheel(1);
      pointing_motion.clear();
      expect(pointing_motion.is_zero());
    }
  };
}
