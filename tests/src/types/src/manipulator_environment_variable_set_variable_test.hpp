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
           value_t("kuv"));
    t name_only("n",
                std::nullopt,
                std::nullopt);
    t value_only(std::nullopt,
                 value_t("v"),
                 std::nullopt);
    t key_up_value_only(std::nullopt,
                        std::nullopt,
                        value_t("kuv"));
    t no_name(std::nullopt,
              value_t("v"),
              value_t("kuv"));
    t no_value("n",
               std::nullopt,
               value_t("kuv"));
    t no_key_up_value("n",
                      value_t("v"),
                      std::nullopt);

    expect(empty == empty);
    expect(full == full);
    expect(empty != full);
    expect(no_name != full);
    expect(no_value != full);
    expect(no_key_up_value != full);

    expect(nlohmann::json::object({}) == nlohmann::json(empty));
    expect(nlohmann::json::object({
               {"name", "n"},
               {"value", "v"},
               {"key_up_value", "kuv"},
           }) == nlohmann::json(full));
    expect(nlohmann::json::object({
               {"name", "n"},
           }) == nlohmann::json(name_only));
    expect(nlohmann::json::object({
               {"value", "v"},
           }) == nlohmann::json(value_only));
    expect(nlohmann::json::object({
               {"key_up_value", "kuv"},
           }) == nlohmann::json(key_up_value_only));

    expect(nlohmann::json(empty).get<t>() == empty);
    expect(nlohmann::json(full).get<t>() == full);
    expect(nlohmann::json(name_only).get<t>() == name_only);
    expect(nlohmann::json(value_only).get<t>() == value_only);
    expect(nlohmann::json(key_up_value_only).get<t>() == key_up_value_only);
    expect(nlohmann::json(no_name).get<t>() == no_name);
    expect(nlohmann::json(no_value).get<t>() == no_value);
    expect(nlohmann::json(no_key_up_value).get<t>() == no_key_up_value);
  };
}
