#pragma once

#include "base.hpp"
#include "exprtk_utility.hpp"
#include <optional>
#include <string>

namespace krbn {
namespace manipulator {
namespace conditions {
class expression final : public base {
public:
  enum class type {
    expression_if,
    expression_unless,
  };

  expression(const nlohmann::json& json) : base(),
                                           type_(type::expression_if) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        pqrs::json::requires_string(value, key);

        auto t = value.get<std::string>();

        if (t == "expression_if") {
          type_ = type::expression_if;
        } else if (t == "expression_unless") {
          type_ = type::expression_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "expression") {
        if (value.is_string()) {
          expression_ = exprtk_utility::compile(value.get<std::string>());

        } else if (value.is_array()) {
          std::stringstream ss;

          for (const auto& j : value) {
            ss << j.template get<std::string>();
          }

          expression_ = exprtk_utility::compile(ss.str());

        } else {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array of string, or string, but is `{1}`",
                                                        key,
                                                        value));
        }

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }
  }

  virtual ~expression(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    if (expression_) {
      auto value = expression_->value();
      if (std::isnan(value)) {
        return false;
      }

      switch (type_) {
        case type::expression_if:
          return value;
        case type::expression_unless:
          return !value;
      }
    }

    return false;
  }

  std::shared_ptr<exprtk_utility::expression_wrapper> get_expression(void) const {
    return expression_;
  }

private:
  type type_;
  std::shared_ptr<exprtk_utility::expression_wrapper> expression_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
