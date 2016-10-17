#include "types.hpp"
#include <json/json.hpp>

int main(int argc, const char* argv[]) {
  nlohmann::json json;
  for (const auto& it : krbn::types::get_key_code_map()) {
    json[it.first]["key_code"] = static_cast<uint32_t>(it.second);
    if (auto hid_system_key = krbn::types::get_hid_system_key(it.second)) {
      json[it.first]["hid_system_key"] = *hid_system_key;
    }
    if (auto hid_system_aux_control_button = krbn::types::get_hid_system_aux_control_button(it.second)) {
      json[it.first]["hid_system_aux_control_button"] = *hid_system_aux_control_button;
    }
  }
  std::cout << std::setw(4) << json << std::endl;

  return 0;
}
