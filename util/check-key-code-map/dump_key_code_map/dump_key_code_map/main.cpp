#include "userspace_types.hpp"
#include "json/json.hpp"

int main(int argc, const char* argv[]) {
  nlohmann::json json;
  for (const auto& it : krbn::types::get_key_code_map()) {
    json[it.first] = static_cast<uint32_t>(it.second);
  }
  std::cout << std::setw(4) << json << std::endl;

  return 0;
}
