#include "types.hpp"
#include <boost/ut.hpp>

void run_manipulator_environment_variable_set_variable_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::string_view_literals;

  "manipulator_environment_variable_set_variable"_test = [] {
    using t = krbn::manipulator_environment_variable_set_variable;
    using value_t = krbn::manipulator_environment_variable_value;

    t empty;
    t full("n",
           value_t("v"),
           nullptr,
           value_t("kuv"),
           nullptr);
    t name_only("n",
                std::nullopt,
                nullptr,
                std::nullopt,
                nullptr);
    t value_only(std::nullopt,
                 value_t("v"),
                 nullptr,
                 std::nullopt,
                 nullptr);
    t key_up_value_only(std::nullopt,
                        std::nullopt,
                        nullptr,
                        value_t("kuv"),
                        nullptr);
    t no_name(std::nullopt,
              value_t("v"),
              nullptr,
              value_t("kuv"),
              nullptr);
    t no_value("n",
               std::nullopt,
               nullptr,
               value_t("kuv"),
               nullptr);
    t no_key_up_value("n",
                      value_t("v"),
                      nullptr,
                      std::nullopt,
                      nullptr);
    t expression_full("n",
                      std::nullopt,
                      krbn::exprtk_utility::compile("system.now.milliseconds"),
                      std::nullopt,
                      krbn::exprtk_utility::compile("system.now.milliseconds + 1000"));
    t expression_only("n",
                      std::nullopt,
                      krbn::exprtk_utility::compile("system.now.milliseconds"),
                      std::nullopt,
                      nullptr);
    t expression_key_up_only("n",
                             std::nullopt,
                             nullptr,
                             std::nullopt,
                             krbn::exprtk_utility::compile("system.now.milliseconds + 1000"));
    t unset("u",
            std::nullopt,
            nullptr,
            std::nullopt,
            nullptr,
            t::type::unset);

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
