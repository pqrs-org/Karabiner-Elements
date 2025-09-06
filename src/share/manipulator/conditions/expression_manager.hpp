#pragma once

#include <gsl/gsl>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class expression;

class expression_manager final {
public:
  void insert(std::weak_ptr<expression> expression) {
    expressions_.push_back(expression);
  }

  void call(std::function<void(std::shared_ptr<expression>)> function) {
    std::erase_if(expressions_, [&](auto&& weak_ptr) {
      if (auto shared_ptr = weak_ptr.lock()) {
        function(shared_ptr);
        return false;
      }

      return true;
    });
  }

private:
  std::vector<std::weak_ptr<expression>> expressions_;
};

class shared_expression_manager final {
public:
  expression_manager& get_expression_manager(void) {
    return expression_manager_;
  }

  static gsl::not_null<std::shared_ptr<shared_expression_manager>> get_shared_expression_manager(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    static std::shared_ptr<shared_expression_manager> p;
    if (!p) {
      p = std::make_shared<shared_expression_manager>();
    }

    return p;
  }

private:
  expression_manager expression_manager_;
};

} // namespace conditions
} // namespace manipulator
} // namespace krbn
