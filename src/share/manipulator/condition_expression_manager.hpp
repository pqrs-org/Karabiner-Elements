#pragma once

#include "conditions/expression.hpp"
#include <gsl/gsl>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace krbn {
namespace manipulator {

// We manage all conditions::expression instances with a manager
// because changing a variable via set_variable requires updating every instance.
//
// Note:
// Because expression variables can include key information, they must remain within the grabber.
// Specifically, do not write them to JSON files or logs, and do not send them to other processes.
// (e.g., console_user_server)

class condition_expression_manager final {
public:
  void insert(std::weak_ptr<exprtk_utility::expression_wrapper> expression) {
    expressions_.push_back(expression);

    // Set variables for new instance.
    if (auto shared_ptr = expression.lock()) {
      for (auto&& [name, value] : variables_) {
        shared_ptr->set_variable(name, value);
      }

      for (auto&& [name, value] : string_variables_) {
        shared_ptr->set_variable(name, value);
      }
    }
  }

  void set_variable(const std::string& name,
                    double value) {
    std::erase_if(expressions_, [&](auto&& weak_ptr) {
      if (auto shared_ptr = weak_ptr.lock()) {
        if (!exprtk_utility::is_string_variable_name(name)) {
          shared_ptr->set_variable(name, value);
          variables_.insert_or_assign(name, value);
        }

        return false;
      }

      return true;
    });
  }

  void set_variable(const std::string& name,
                    const std::string& value) {
    std::erase_if(expressions_, [&](auto&& weak_ptr) {
      if (auto shared_ptr = weak_ptr.lock()) {
        if (exprtk_utility::is_string_variable_name(name)) {
          shared_ptr->set_variable(name, value);
          string_variables_.insert_or_assign(name, value);
        }

        return false;
      }

      return true;
    });
  }

private:
  std::vector<std::weak_ptr<exprtk_utility::expression_wrapper>> expressions_;
  std::unordered_map<std::string, double> variables_;
  std::unordered_map<std::string, std::string> string_variables_;
};

inline gsl::not_null<std::shared_ptr<condition_expression_manager>> get_shared_condition_expression_manager(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);

  static std::shared_ptr<condition_expression_manager> p;
  if (!p) {
    p = std::make_shared<condition_expression_manager>();
  }

  return p;
}

} // namespace manipulator
} // namespace krbn
