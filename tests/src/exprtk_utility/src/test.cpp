#include "exprtk_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "exprtk_utility::eval"_test = [] {
    auto actual = krbn::exprtk_utility::eval("cos(radian) * delta_magnitude",
                                             {
                                                 {"radian", 0.78539816339},
                                                 {"delta_magnitude", 3.5},
                                             });
    expect(2.47487_d == actual);
  };

  "exprtk_utility::compile"_test = [] {
    double radian = 0.0;
    double delta_magnitude = 1.0;

    auto expression = krbn::exprtk_utility::compile("cos(radian) * delta_magnitude",
                                                    {},
                                                    {
                                                        {"radian", radian},
                                                        {"delta_magnitude", delta_magnitude},
                                                    });
    expect(1.0_d == expression.value());

    radian = 0.78539816339;
    delta_magnitude = 3.5;
    expect(2.47487_d == expression.value());
  };

  "exprtk_utility::eval error"_test = [] {
    auto actual = krbn::exprtk_utility::eval("cos(unknown)",
                                             {
                                                 {"radian", 0.78539816339},
                                                 {"delta_magnitude", 3.5},
                                             });
    expect(std::isnan(actual));
  };

  "max_loop_iterations"_test = [] {
    auto actual = krbn::exprtk_utility::eval("var x := 0; while (x < 1) { x -= 1; }; x;",
                                             {});
    expect(std::isnan(actual));
  };

  return 0;
}
