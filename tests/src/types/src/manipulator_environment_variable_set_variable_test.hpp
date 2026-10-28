#include "types.hpp"
#include <boost/ut.hpp>

void run_manipulator_environment_variable_set_variable_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_view_literals;

  "manipulator_environment_variable_set_variable"_test = [] {
    using t = krbn::manipulator_environment_variable_set_variable;
    using value_t = krbn::manipulator_environment_variable_value;

    t empty;
    t full({
        .name = "n",
        .value = value_t("v"),
        .key_up_value = value_t("kuv"),
    });
    t name_only({.name = "n"});
    t value_only({.value = value_t("v")});
    t key_up_value_only({.key_up_value = value_t("kuv")});
    t no_name({
        .value = value_t("v"),
        .key_up_value = value_t("kuv"),
    });
    t no_value({
        .name = "n",
        .key_up_value = value_t("kuv"),
    });
    t no_key_up_value({
        .name = "n",
        .value = value_t("v"),
    });
    t expression_full({
        .name = "n",
        .expression = krbn::exprtk_utility::compile("system.now.milliseconds"),
        .key_up_expression = krbn::exprtk_utility::compile("system.now.milliseconds + 1000"),
    });
    t expression_only({
        .name = "n",
        .expression = krbn::exprtk_utility::compile("system.now.milliseconds"),
    });
    t expression_key_up_only({
        .name = "n",
        .key_up_expression = krbn::exprtk_utility::compile("system.now.milliseconds + 1000"),
    });
    t unset({
        .name = "u",
        .type = t::type::unset,
    });

    expect(empty == empty);
    expect(full == full);
    expect(empty != full);
    expect(no_name != full);
    expect(no_value != full);
    expect(no_key_up_value != full);
    expect(expression_full != full);
    expect(expression_only != full);
    expect(expression_key_up_only != full);
    expect(unset != full);

    {
      // Equal objects must have equal hashes, even when the expressions are separately compiled.
      t expression_full_copy({
          .name = "n",
          .expression = krbn::exprtk_utility::compile("system.now.milliseconds"),
          .key_up_expression = krbn::exprtk_utility::compile("system.now.milliseconds + 1000"),
      });
      expect(expression_full == expression_full_copy);
      expect(std::hash<t>{}(expression_full) == std::hash<t>{}(expression_full_copy));
    }

    expect(nlohmann::json::object({{"type", "set"}}) == nlohmann::json(empty));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"value", "v"},
               {"key_up_value", "kuv"},
               {"type", "set"},
           }) == nlohmann::json(full));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"type", "set"},
           }) == nlohmann::json(name_only));
    expect(nlohmann::json::object({
               {"value", "v"},
               {"type", "set"},
           }) == nlohmann::json(value_only));
    expect(nlohmann::json::object({
               {"key_up_value", "kuv"},
               {"type", "set"},
           }) == nlohmann::json(key_up_value_only));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"expression", "system.now.milliseconds"},
               {"key_up_expression", "system.now.milliseconds + 1000"},
               {"type", "set"},
           }) == nlohmann::json(expression_full));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"expression", "system.now.milliseconds"},
               {"type", "set"},
           }) == nlohmann::json(expression_only));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"key_up_expression", "system.now.milliseconds + 1000"},
               {"type", "set"},
           }) == nlohmann::json(expression_key_up_only));
    expect(nlohmann::json::object({
               {"name", "u"},
               {"type", "unset"},
           }) == nlohmann::json(unset));

    expect(nlohmann::json(empty).get<t>() == empty);
    expect(nlohmann::json(full).get<t>() == full);
    expect(nlohmann::json(name_only).get<t>() == name_only);
    expect(nlohmann::json(value_only).get<t>() == value_only);
    expect(nlohmann::json(key_up_value_only).get<t>() == key_up_value_only);
    expect(nlohmann::json(no_name).get<t>() == no_name);
    expect(nlohmann::json(no_value).get<t>() == no_value);
    expect(nlohmann::json(no_key_up_value).get<t>() == no_key_up_value);
    expect(nlohmann::json(expression_full).get<t>() == expression_full);
    expect(nlohmann::json(expression_only).get<t>() == expression_only);
    expect(nlohmann::json(expression_key_up_only).get<t>() == expression_key_up_only);
    expect(nlohmann::json(unset).get<t>() == unset);
  };
}
