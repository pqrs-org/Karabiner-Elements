#include "types.hpp"
#include <boost/ut.hpp>

void run_modifier_flag_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "modifier_flag json"_test = [] {
    expect(nlohmann::json(krbn::modifier_flag::zero) == nlohmann::json("zero"));
    expect(nlohmann::json(krbn::modifier_flag::caps_lock) == nlohmann::json("caps_lock"));
    expect(nlohmann::json(krbn::modifier_flag::left_control) == nlohmann::json("left_control"));
    expect(nlohmann::json(krbn::modifier_flag::left_shift) == nlohmann::json("left_shift"));
    expect(nlohmann::json(krbn::modifier_flag::left_option) == nlohmann::json("left_option"));
    expect(nlohmann::json(krbn::modifier_flag::left_command) == nlohmann::json("left_command"));
    expect(nlohmann::json(krbn::modifier_flag::right_control) == nlohmann::json("right_control"));
    expect(nlohmann::json(krbn::modifier_flag::right_shift) == nlohmann::json("right_shift"));
    expect(nlohmann::json(krbn::modifier_flag::right_option) == nlohmann::json("right_option"));
    expect(nlohmann::json(krbn::modifier_flag::right_command) == nlohmann::json("right_command"));
    expect(nlohmann::json(krbn::modifier_flag::fn) == nlohmann::json("fn"));
    expect(nlohmann::json(krbn::modifier_flag::end_) == nlohmann::json());
  };
}
