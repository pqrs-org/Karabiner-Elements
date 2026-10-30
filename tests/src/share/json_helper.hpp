#pragma once

#include "json_utility.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <pqrs/string.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace krbn::unit_testing {
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

  // Rough implementation. Only entire line comments are supported.
  //
  // `// Supported`
  // `[1,2,3], // Not supported this comment`
  // `/* Not supported this comment */`
  static nlohmann::json load_jsonc(const std::string& file_path) {
    std::ifstream json_file(file_path);
    if (!json_file) {
      throw std::runtime_error(file_path + " is not found");
    }

    return json_utility::parse_jsonc(json_file);
  }

  template <typename T>
  static std::string dump_single_line(const T& json) {
    std::istringstream input(json_utility::dump(json));
    std::string result;
    std::string line;

    while (std::getline(input, line)) {
      if (auto p = line.find_first_not_of(' ');
          p != std::string::npos) {
        if (!result.empty()) {
          result += ' ';
        }
        result.append(line, p);
      }
    }

    return result;
  }

  template <typename T>
  static std::string dump_aligned_array(const T& json,
                                        const std::vector<std::string>& keys) {
    std::vector<std::string> lines;
    for (const auto& j : json) {
      lines.push_back(dump_single_line(j));
    }

    for (const auto& key : keys) {
      std::size_t maximum_position = 0;
      auto marker = "\"" + key + "\":";

      for (const auto& line : lines) {
        if (auto position = line.rfind(marker);
            position != std::string::npos) {
          maximum_position = std::max(maximum_position,
                                      position);
        }
      }

      for (auto& line : lines) {
        if (auto position = line.rfind(marker);
            position != std::string::npos) {
          line.insert(position,
                      maximum_position - position,
                      ' ');
        }
      }
    }

    std::ostringstream output;
    output << "[" << std::endl;
    for (auto it = std::begin(lines); it != std::end(lines); ++it) {
      output << "    " << *it;
      if (std::next(it) != std::end(lines)) {
        output << ",";
      }
      output << std::endl;
    }
    output << "]";

    return output.str();
  }
};
} // namespace krbn::unit_testing
