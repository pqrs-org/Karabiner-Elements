#include "exprtk_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "exprtk_utility::eval"_test = [] {
    auto actual = krbn::exprtk_utility::eval("cos(radian) * magnitude",
                                             {
                                                 {"radian", 0.78539816339},
                                                 {"magnitude", 3.5},
                                             });
    expect(2.47487_d == actual);
  };

  "exprtk_utility::compile"_test = [] {
    double radian = 0.0;
    double magnitude = 1.0;

    auto expression = krbn::exprtk_utility::compile("cos(radian) * magnitude",
                                                    {},
                                                    {
                                                        {"radian", radian},
                                                        {"magnitude", magnitude},
                                                    });
    expect(1.0_d == expression.value());

    radian = 0.78539816339;
    magnitude = 3.5;
    expect(2.47487_d == expression.value());
  };

  return 0;
}
