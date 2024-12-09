#pragma once

#include "../../share/json_helper.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/condition_manager.hpp"
#include <boost/ut.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

class actual_examples_helper final {
public:
  actual_examples_helper(const std::string& file_name) {
    auto json = krbn::unit_testing::json_helper::load_jsonc(std::string("json/") + file_name);
    for (const auto& j : json) {
      try {
        condition_manager_.push_back_condition(krbn::manipulator::condition_factory::make_condition(j));
      } catch (pqrs::json::unmarshal_error& e) {
        error_messages_.push_back(e.what());
      } catch (std::exception& e) {
        expect(false);
      }
    }
  }

  const krbn::manipulator::condition_manager& get_condition_manager(void) const {
    return condition_manager_;
  }

  const std::vector<std::string> get_error_messages(void) const {
    return error_messages_;
  }

private:
  krbn::manipulator::condition_manager condition_manager_;
  std::vector<std::string> error_messages_;
};
} // namespace
