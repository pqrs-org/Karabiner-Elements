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
                                             },
                                             {});
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
    expect(1.0_d == expression->value());

    radian = 0.78539816339;
    delta_magnitude = 3.5;
    expect(2.47487_d == expression->value());
  };

  "exprtk_utility::eval error"_test = [] {
    auto actual = krbn::exprtk_utility::eval("cos(unknown)",
                                             {
                                                 {"radian", 0.78539816339},
                                                 {"delta_magnitude", 3.5},
                                             },
                                             {});
    expect(std::isnan(actual));
  };

  "max_loop_iterations"_test = [] {
    auto actual = krbn::exprtk_utility::eval("var x := 0; while (x < 1) { x -= 1; }; x;",
                                             {},
                                             {});
    expect(std::isnan(actual));
  };

  "expression_wrapper::replace_variables"_test = [] {
    double example_variable1 = 11.0;
    double example_variable2 = 12.0;

    auto expression = krbn::exprtk_utility::compile("example_constant1 + example_constant2 + example_variable1 + example_variable2",
                                                    {
                                                        {"example_constant1", 1.0},
                                                        {"example_constant2", 2.0},
                                                    },
                                                    {
                                                        {"example_variable1", example_variable1},
                                                        {"example_variable2", example_variable2},
                                                    });
    expect(26.0_d == expression->value());

    // No recompilation occurs if the variable addresses have not changed.
    example_variable1 = 21.0;
    expect(36.0_d == expression->value());

    expect(false == expression->replace_variables({
                                                      {"example_constant1", 1.0},
                                                      {"example_constant2", 2.0},
                                                  },
                                                  {
                                                      {"example_variable1", example_variable1},
                                                      {"example_variable2", example_variable2},
                                                  }));
    expect(36.0_d == expression->value());

    // A recompilation is triggered when constants change.
    expect(true == expression->replace_variables({
                                                     {"example_constant1", 11.0},
                                                     {"example_constant2", 2.0},
                                                 },
                                                 {
                                                     {"example_variable1", example_variable1},
                                                     {"example_variable2", example_variable2},
                                                 }));
    expect(46.0_d == expression->value());

    // A recompilation is triggered when variables address change.
    double new_example_variable1 = 11.0;
    expect(true == expression->replace_variables({
                                                     {"example_constant1", 11.0},
                                                     {"example_constant2", 2.0},
                                                 },
                                                 {
                                                     {"example_variable1", new_example_variable1},
                                                     {"example_variable2", example_variable2},
                                                 }));
    expect(36.0_d == expression->value());

    // No recompilation occurs if the variable addresses have not changed.
    new_example_variable1 = 111.0;
    expect(136.0_d == expression->value());

    expect(false == expression->replace_variables({
                                                      {"example_constant1", 11.0},
                                                      {"example_constant2", 2.0},
                                                  },
                                                  {
                                                      {"example_variable1", new_example_variable1},
                                                      {"example_variable2", example_variable2},
                                                  }));
    expect(136.0_d == expression->value());
  };

  "expression_wrapper::assign_variables remove/add constants"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_constant1 + example_constant2",
                                                    {
                                                        {"example_constant1", 1.0},
                                                        {"example_constant2", 2.0},
                                                    },
                                                    {});
    expect(3.0_d == expression->value());

    // Remove example_constant2.
    expect(true == expression->replace_variables({
                                                     {"example_constant1", 1.0},
                                                 },
                                                 {}));
    expect(std::isnan(expression->value()));

    // Add example_constant2 again.
    expect(true == expression->replace_variables({
                                                     {"example_constant1", 10.0},
                                                     {"example_constant2", 20.0},
                                                 },
                                                 {}));
    expect(30.0_d == expression->value());
  };

  "expression_wrapper::assign_variables remove/add variables"_test = [] {
    double example_variable1 = 11.0;
    double example_variable2 = 12.0;

    auto expression = krbn::exprtk_utility::compile("example_variable1 + example_variable2",
                                                    {},
                                                    {
                                                        {"example_variable1", example_variable1},
                                                        {"example_variable2", example_variable2},
                                                    });
    expect(23.0_d == expression->value());

    // Remove example_variable2.
    expect(true == expression->replace_variables({},
                                                 {
                                                     {"example_variable1", example_variable1},
                                                 }));
    expect(std::isnan(expression->value()));

    // Add example_constant2 again.
    expect(true == expression->replace_variables({},
                                                 {
                                                     {"example_variable1", example_variable1},
                                                     {"example_variable2", example_variable2},
                                                 }));
    expect(23.0_d == expression->value());
  };

  return 0;
}
