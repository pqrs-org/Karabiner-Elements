#pragma once

#include <fstream>
#include <json/json.hpp>
#include <string>

namespace krbn {
namespace unit_testing {
class json_helper final {
public:
  static bool compare_files(const std::string& file_path1,
                            const std::string& file_path2) {
    std::ifstream stream1(file_path1);
    std::ifstream stream2(file_path2);

    if (stream1 && stream2) {
      try {
        nlohmann::json json1;
        nlohmann::json json2;
        stream1 >> json1;
        stream2 >> json2;

        return json1 == json2;
      } catch (...) {
      }
    }

    return false;
  }
};
} // namespace unit_testing
} // namespace krbn
