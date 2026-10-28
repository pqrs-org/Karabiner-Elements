#include "exprtk_utility.hpp"
#include <boost/ut.hpp>

int main() {
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
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(true == expression_wrapper.set_variable("radian", 0.78539816339));
    expect(true == expression_wrapper.set_variable("delta_magnitude", 3.5));
    expect(2.47487_d == expression_wrapper.value());
  };

  "empty string"_test = [] {
    auto expression = krbn::exprtk_utility::compile("");
    expect(std::isnan(expression.get_expression_wrapper().value()));
  };

  "with var"_test = [] {
    auto expression = krbn::exprtk_utility::compile("var m := 3; m * 2");
    expect(6.0_d == expression.get_expression_wrapper().value());
  };

  "undefined variable"_test = [] {
    auto expression = krbn::exprtk_utility::compile("x + 2");
    expect(2.0_d == expression.get_expression_wrapper().value());
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
    expect(0.0_d == expression.get_expression_wrapper().value());
  };

  "compilation error"_test = [] {
    auto expression = krbn::exprtk_utility::compile("cos(");
    expect(std::isnan(expression.get_expression_wrapper().value()));
  };

  "max_loop_iterations"_test = [] {
    auto expression = krbn::exprtk_utility::compile("var x := 0; while (x < 1) { x -= 1; }; x;");
    expect(std::isnan(expression.get_expression_wrapper().value()));
  };

  "set_variable (double)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_variable1 + example_variable2");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(true == expression_wrapper.set_variable("example_variable1", 1.0));
    expect(true == expression_wrapper.set_variable("example_variable2", 2.0));
    expect(3.0_d == expression_wrapper.value());

    expect(true == expression_wrapper.set_variable("example_variable1", 10.0));
    expect(12.0_d == expression_wrapper.value());
  };

  "set_variable (string)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_string like 'hello*'");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(0.0_d == expression_wrapper.value());

    expect(true == expression_wrapper.set_variable("example_string", "hello world"));
    expect(1.0_d == expression_wrapper.value());
  };

  "set_variable type check"_test = [] {
    auto expression = krbn::exprtk_utility::compile("if (example_string like 'h*') { example_variable; } else { example_variable * 2; }");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(0.0_d == expression_wrapper.value());

    // A string variable must end with "_string".
    expect(false == expression_wrapper.set_variable("example_variable", "example"));

    // A double variable must not end with "_string".
    expect(false == expression_wrapper.set_variable("example_string", 1.0));

    expect(true == expression_wrapper.set_variable("example_variable", 42.0));
    expect(true == expression_wrapper.set_variable("example_string", "world"));
    expect(84.0_d == expression_wrapper.value());

    expect(true == expression_wrapper.set_variable("example_string", "hello"));
    expect(42.0_d == expression_wrapper.value());
  };

  "set_variable with predefined constants"_test = [] {
    auto expression = krbn::exprtk_utility::compile("pi * 2");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(6.28_d == expression_wrapper.value());

    // Calling set_variable on a defined constant does not change its value.
    expression_wrapper.set_variable("pi", 3.0);
    expect(6.28_d == expression_wrapper.value());
  };

  "unset_variable (double)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_variable");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(0_i64 == expression_wrapper.value<int64_t>());

    expression_wrapper.set_variable("example_variable", 42.0);
    expect(42_i64 == expression_wrapper.value<int64_t>());

    expression_wrapper.unset_variable("example_variable");
    expect(0_i64 == expression_wrapper.value<int64_t>());
  };

  "unset_variable (string)"_test = [] {
    auto expression = krbn::exprtk_utility::compile("example_string like 'hello*'");
    auto& expression_wrapper = expression.get_expression_wrapper();
    expect(0.0_d == expression_wrapper.value());

    expect(true == expression_wrapper.set_variable("example_string", "hello world"));
    expect(1.0_d == expression_wrapper.value());

    expression_wrapper.unset_variable("example_string");
    expect(0.0_d == expression_wrapper.value());
  };

  "value<int64_t>"_test = [] {
    auto expression = krbn::exprtk_utility::compile("42");
    expect(42_i64 == expression.get_expression_wrapper().value<int64_t>());

    auto nan = krbn::exprtk_utility::compile("_hello");
    expect(std::isnan(nan.get_expression_wrapper().value()));
    expect(0_i64 == nan.get_expression_wrapper().value<int64_t>());

    auto out_of_range = krbn::exprtk_utility::compile("10000000000000000000.0");
    expect(10000000000000000000.0 == out_of_range.get_expression_wrapper().value());
    expect(0_i64 == out_of_range.get_expression_wrapper().value<int64_t>());
  };

  "compare"_test = [] {
    auto e1_1 = krbn::exprtk_utility::compile("1");
    auto e1_2 = krbn::exprtk_utility::compile("1");
    auto e2_1 = krbn::exprtk_utility::compile("2");
    auto e2_2 = krbn::exprtk_utility::compile("2");

    std::optional<krbn::exprtk_utility::compiled_expression> empty1;
    std::optional<krbn::exprtk_utility::compiled_expression> empty2;

    expect(empty1 == empty2);
    expect(empty1 != std::optional(e1_1));
    expect(std::optional(e1_1) != empty1);
    expect(e1_1 == e1_2);
    expect(e2_1 == e2_2);
    expect(e1_1 != e2_2);

    expect(std::hash<std::optional<krbn::exprtk_utility::compiled_expression>>{}(empty1) ==
           std::hash<std::optional<krbn::exprtk_utility::compiled_expression>>{}(empty2));
    expect(std::hash<krbn::exprtk_utility::compiled_expression>{}(e1_1) ==
           std::hash<krbn::exprtk_utility::compiled_expression>{}(e1_2));
    expect(std::hash<krbn::exprtk_utility::compiled_expression>{}(e2_1) ==
           std::hash<krbn::exprtk_utility::compiled_expression>{}(e2_2));
  };

  return 0;
}
