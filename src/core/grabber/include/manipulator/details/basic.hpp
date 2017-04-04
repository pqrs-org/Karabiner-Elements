#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"
#include "manipulator/event_queue.hpp"
#include <json/json.hpp>
#include <unordered_map>

namespace krbn {
namespace manipulator {
namespace details {
class basic final : public base {
public:
  basic(const nlohmann::json& json) : base(),
                                      from_(json.find("from") != std::end(json) ? json["from"] : nlohmann::json()) {
    {
      const std::string key = "to";
      if (json.find(key) != std::end(json) && json[key].is_array()) {
        for (const auto& j : json[key]) {
          to_.emplace_back(j);
        }
      }
    }
  }

  virtual ~basic(void) {
  }

  virtual void manipulate(event_queue& event_queue, std::chrono::nanoseconds time) {
  }

  virtual bool active(void) const {
    return false;
  }

  const event_definition& get_from(void) const {
    return from_;
  }

  const std::vector<event_definition>& get_to(void) const {
    return to_;
  }

private:
  event_definition from_;
  std::vector<event_definition> to_;
};
} // namespace details
} // namespace manipulator
} // namespace krbn
