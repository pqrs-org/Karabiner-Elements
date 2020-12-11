#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("modifier_flag json") {
  REQUIRE(nlohmann::json(krbn::modifier_flag::zero) == nlohmann::json("zero"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::caps_lock) == nlohmann::json("caps_lock"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::left_control) == nlohmann::json("left_control"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::left_shift) == nlohmann::json("left_shift"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::left_option) == nlohmann::json("left_option"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::left_command) == nlohmann::json("left_command"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::right_control) == nlohmann::json("right_control"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::right_shift) == nlohmann::json("right_shift"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::right_option) == nlohmann::json("right_option"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::right_command) == nlohmann::json("right_command"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::fn) == nlohmann::json("fn"));
  REQUIRE(nlohmann::json(krbn::modifier_flag::end_) == nlohmann::json());
}
