#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace krbn {
class connected_device_capabilities final {
public:
  [[nodiscard]] const std::vector<uint16_t>& get_logitech_hidpp_divertable_control_ids() const {
    return logitech_hidpp_divertable_control_ids_;
  }

  void set_logitech_hidpp_divertable_control_ids(const std::vector<uint16_t>& value) {
    logitech_hidpp_divertable_control_ids_ = value;
  }

  [[nodiscard]] nlohmann::json to_json() const {
    nlohmann::json json = nlohmann::json::object();

    if (!logitech_hidpp_divertable_control_ids_.empty()) {
      json["logitech_hidpp"] = {
          {"divertable_control_ids", logitech_hidpp_divertable_control_ids_},
      };
    }

    return json;
  }

  static connected_device_capabilities make_connected_device_capabilities(
      const nlohmann::json& json) {
    connected_device_capabilities result;

    if (auto it = json.find("logitech_hidpp");
        it != json.end() && it->is_object()) {
      if (auto control_ids = it->find("divertable_control_ids");
          control_ids != it->end()) {
        result.logitech_hidpp_divertable_control_ids_ = control_ids->get<std::vector<uint16_t>>();
      }
    }

    return result;
  }

private:
  std::vector<uint16_t> logitech_hidpp_divertable_control_ids_;
};
} // namespace krbn
