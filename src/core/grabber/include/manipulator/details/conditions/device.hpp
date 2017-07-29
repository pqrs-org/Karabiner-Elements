#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class device final : public base {
public:
  enum class type {
    device_if,
    device_unless,
  };

  device(const nlohmann::json& json) : base(),
                                       type_(type::device_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "device_if") {
              type_ = type::device_if;
            }
            if (value == "device_unless") {
              type_ = type::device_unless;
            }
          }
        } else if (key == "identifiers") {
          if (value.is_array()) {
            handle_identifiers_json(value);
          } else {
            logger::get_logger().error("complex_modifications json error: Invalid form of {0} in {1}", key, json.dump());
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~device(void) {
  }

  virtual bool is_fulfilled(const event_queue::queued_event& queued_event,
                            const manipulator_environment& manipulator_environment) const {
    for (const auto& identifier : identifiers_) {
      bool fulfilled = true;

      if (identifier.first != vendor_id::zero &&
          identifier.first != types::find_vendor_id(queued_event.get_device_id())) {
        fulfilled = false;
      }
      if (identifier.second != product_id::zero &&
          identifier.second != types::find_product_id(queued_event.get_device_id())) {
        fulfilled = false;
      }

      if (fulfilled) {
        switch (type_) {
          case type::device_if:
            return true;
          case type::device_unless:
            return false;
        }
      }
    }

    // Not found

    switch (type_) {
      case type::device_if:
        return false;
      case type::device_unless:
        return true;
    }
  }

private:
  void handle_identifiers_json(const nlohmann::json& json) {
    for (const auto& j : json) {
      if (j.is_object()) {
        auto vid = vendor_id::zero;
        auto pid = product_id::zero;

        for (auto it = std::begin(j); it != std::end(j); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "vendor_id") {
            if (value.is_number()) {
              int v = value;
              vid = vendor_id(v);
            } else {
              logger::get_logger().error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else if (key == "product_id") {
            if (value.is_number()) {
              int v = value;
              pid = product_id(v);
            } else {
              logger::get_logger().error("complex_modifications json error: Invalid form of {0} in {1}", key, j.dump());
            }
          } else {
            logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, j.dump());
          }
        }

        if (vid != vendor_id::zero ||
            pid != product_id::zero) {
          identifiers_.emplace_back(vid, pid);
        }

      } else {
        logger::get_logger().error("complex_modifications json error: `identifiers` children should be object {0}", json.dump());
      }
    }
  }

  type type_;
  std::vector<std::pair<vendor_id, product_id>> identifiers_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
