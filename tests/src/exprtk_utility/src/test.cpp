#include "exprtk_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "is_valid_variable_name"_test = [] {
    expect(true == krbn::exprtk_utility::is_valid_variable_name("example_variable"));
    expect(true == krbn::exprtk_utility::is_valid_variable_name("system.now.milliseconds"));
    expect(false == krbn::exprtk_utility::is_valid_variable_name("hello world"));
  };

  "is_string_variable_name"_test = [] {
    expect(false == krbn::exprtk_utility::is_string_variable_name("example_variable"));
    expect(true == krbn::exprtk_utility::is_string_variable_name("example_string"));
  };

  "normal"_test = [] {
    auto expression = krbn::exprtk_utility::compile("cos(radian) * delta_magnitude");
    expect(true == expression->set_variable("radian", 0.78539816339));
    expect(true == expression->set_variable("delta_magnitude", 3.5));
    expect(2.47487_d == expression->value());
  };

  "empty string"_test = [] {
    auto expression = krbn::exprtk_utility::compile("");
    expect(std::isnan(expression->value()));
  };

  "with var"_test = [] {
    auto expression = krbn::exprtk_utility::compile("var m := 3; m * 2");
    expect(6.0_d == expression->value());
  };

  "undefined variable"_test = [] {
    auto expression = krbn::exprtk_utility::compile("x + 2");
    expect(2.0_d == expression->value());
  };

  "with actual formula"_test = [] {
    auto expression = krbn::exprtk_utility::compile(R"(

var m := 0;

if (abs(cos(radian)) < abs(sin(radian))) {
  if (continued_movement == false) {
    m := delta_magnitude;
  } else {
    m := absolute_magnitude * 0.1;
  };
};

sin(radian) * m;

    )");
    expect(0.0_d == expression->value());
  };

  "compilation error"_test = [] {
    auto expression = krbn::exprtk_utility::compile("cos(");
    expect(std::isnan(expression->value()));
  };

  "max_loop_iterations"_test = [] {
    auto expression = krbn::exprtk_utility::compile("var x := 0; while (x < 1) { x -= 1; }; x;");
    expect(std::isnan(expression->value()));
  };

  "set_variable (double)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_variable1 + example_variable2");
    expect(true == expression->set_variable("example_variable1", 1.0));
    expect(true == expression->set_variable("example_variable2", 2.0));
    expect(3.0_d == expression->value());

    expect(true == expression->set_variable("example_variable1", 10.0));
    expect(12.0_d == expression->value());
  };

  "set_variable (string)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_string like 'hello*'");
    expect(0.0_d == expression->value());

    expect(true == expression->set_variable("example_string", "hello world"));
    expect(1.0_d == expression->value());
  };

  "set_variable type check"_test = [] {
    auto expression = krbn::exprtk_utility::compile("if (example_string like 'h*') { example_variable; } else { example_variable * 2; }");
    expect(0.0_d == expression->value());

    // A string variable must end with "_string".
    expect(false == expression->set_variable("example_variable", "example"));

    // A double variable must not end with "_string".
    expect(false == expression->set_variable("example_string", 1.0));

    expect(true == expression->set_variable("example_variable", 42.0));
    expect(true == expression->set_variable("example_string", "world"));
    expect(84.0_d == expression->value());

    expect(true == expression->set_variable("example_string", "hello"));
    expect(42.0_d == expression->value());
  };

  "set_variable with predefined constants"_test = [] {
    auto expression = krbn::exprtk_utility::compile("pi * 2");
    expect(6.28_d == expression->value());

    // Calling set_variable on a defined constant does not change its value.
    expression->set_variable("pi", 3.0);
    expect(6.28_d == expression->value());
  };

  return 0;
}
