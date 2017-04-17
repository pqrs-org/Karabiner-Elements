#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulated_keys.hpp"
#include "thread_utility.hpp"

TEST_CASE("manipulated_keys") {
  krbn::manipulator::manipulated_keys manipulated_keys;

  krbn::manipulator::manipulated_keys::manipulated_key manipulated_key_1_10_20(krbn::device_id(1),
                                                                               krbn::key_code(10),
                                                                               krbn::key_code(20));
  krbn::manipulator::manipulated_keys::manipulated_key manipulated_key_1_10_30(krbn::device_id(1),
                                                                               krbn::key_code(10),
                                                                               krbn::key_code(30));
  krbn::manipulator::manipulated_keys::manipulated_key manipulated_key_2_10_40(krbn::device_id(2),
                                                                               krbn::key_code(10),
                                                                               krbn::key_code(40));

  manipulated_keys.emplace_back(krbn::device_id(1),
                                krbn::key_code(10),
                                krbn::key_code(20));

  REQUIRE(manipulated_keys.get_manipulated_keys() == std::vector<krbn::manipulator::manipulated_keys::manipulated_key>({
                                                         manipulated_key_1_10_20,
                                                     }));

  REQUIRE(*(manipulated_keys.find(krbn::device_id(1), krbn::key_code(10))) == krbn::key_code(20));
  if (manipulated_keys.find(krbn::device_id(1), krbn::key_code(11)) != boost::none) {
    REQUIRE(false);
  }

  manipulated_keys.erase(krbn::device_id(1), krbn::key_code(10));
  REQUIRE(manipulated_keys.get_manipulated_keys() == std::vector<krbn::manipulator::manipulated_keys::manipulated_key>({}));

  // Same device_id and key_code
  manipulated_keys.emplace_back(krbn::device_id(2),
                                krbn::key_code(10),
                                krbn::key_code(40));
  manipulated_keys.emplace_back(krbn::device_id(1),
                                krbn::key_code(10),
                                krbn::key_code(20));
  manipulated_keys.emplace_back(krbn::device_id(1),
                                krbn::key_code(10),
                                krbn::key_code(30));

  REQUIRE(manipulated_keys.get_manipulated_keys() == std::vector<krbn::manipulator::manipulated_keys::manipulated_key>({
                                                         manipulated_key_2_10_40,
                                                         manipulated_key_1_10_20,
                                                         manipulated_key_1_10_30,
                                                     }));

  REQUIRE(*(manipulated_keys.find(krbn::device_id(1), krbn::key_code(10))) == krbn::key_code(20));
  if (manipulated_keys.find(krbn::device_id(1), krbn::key_code(11)) != boost::none) {
    REQUIRE(false);
  }

  manipulated_keys.erase(krbn::device_id(1), krbn::key_code(10));
  REQUIRE(manipulated_keys.get_manipulated_keys() == std::vector<krbn::manipulator::manipulated_keys::manipulated_key>({
                                                         manipulated_key_2_10_40,
                                                     }));
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
