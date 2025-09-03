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

  "exprtk_utility::eval undefined variable"_test = [] {
    auto actual = krbn::exprtk_utility::eval("x + 2",
                                             {});
    expect(2.0_d == actual);
  };

  "exprtk_utility::eval error"_test = [] {
    auto actual = krbn::exprtk_utility::eval("cos(",
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

  "exprtk_utility::compile"_test = [] {
    auto expression = krbn::exprtk_utility::compile("cos(radian) * delta_magnitude",
                                                    {
                                                        {"radian", 0.0},
                                                        {"delta_magnitude", 1.0},
                                                    });
    expect(1.0_d == expression->value());

    expect(false == expression->set_variables({
                        {"radian", 0.78539816339},
                        {"delta_magnitude", 3.5},
                    }));

    expect(2.47487_d == expression->value());
  };

  "expression_wrapper::set_variable"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_variable1 + example_variable2",
                                                    {
                                                        {"example_variable1", 1.0},
                                                        {"example_variable2", 2.0},
                                                    });
    expect(3.0_d == expression->value());

    // No recompilation occurs if the variables already exist.
    expect(false == expression->set_variable("example_variable1", 10.0));
    expect(12.0_d == expression->value());

    // A recompilation is triggered for new variable.
    expect(true == expression->set_variable("example_variable3", 3.0));
    expect(12.0_d == expression->value());
  };

  "expression_wrapper::set_variables"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_variable1 + example_variable2",
                                                    {
                                                        {"example_variable1", 1.0},
                                                        {"example_variable2", 2.0},
                                                    });
    expect(3.0_d == expression->value());

    // No recompilation occurs if the variables already exist.
    expect(false == expression->set_variables({
                        {"example_variable1", 11.0},
                        {"example_variable2", 12.0},
                    }));
    expect(23.0_d == expression->value());

    // Variables not included in the set_variables arguments keep their current values.
    expect(false == expression->set_variables({
                        {"example_variable1", 21.0},
                        // {"example_variable2", 12.0},
                    }));
    expect(33.0_d == expression->value());

    // A recompilation is triggered for new variable.
    expect(true == expression->set_variables({
                       {"example_variable1", 21.0},
                       {"example_variable2", 22.0},
                       {"example_variable3", 23.0},
                   }));
    expect(43.0_d == expression->value());
  };

  "expression_wrapper::set_variables new_variables"_test = [] {
    auto expression = krbn::exprtk_utility::compile("x * 2",
                                                    {});
    expect(0.0_d == expression->value());

    // Any undefined variables are defined during the previous compilation pass.
    expect(false == expression->set_variables({
                        {"x", 2.0},
                    }));
    expect(4.0_d == expression->value());
  };

  "expression_wrapper::set_variable with predefined constants"_test = [] {
    auto expression = krbn::exprtk_utility::compile("pi * 2",
                                                    {});
    expect(6.28_d == expression->value());

    // Calling set_variable on a defined constant does not change its value.
    expect(true == expression->set_variable("pi", 3.0));
    expect(6.28_d == expression->value());
  };

  return 0;
}
