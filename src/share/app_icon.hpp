#pragma once

#include "json_utility.hpp"
#include "json_writer.hpp"
#include "logger.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

// Json example:
//
// {
//     "number": 0
// }

namespace krbn {
class app_icon final {
public:
  app_icon(void)
      : number_(0) {
  }

  app_icon(int number)
      : number_(number) {
  }

  app_icon(const std::filesystem::path& file_path)
      : app_icon() {
    std::ifstream input(file_path);
    if (input) {
      try {
        auto json = json_utility::parse_jsonc(input);

        number_ = json.at("number").get<int>();

      } catch (std::exception& e) {
        logger::get_logger()->error("parse error in {0}: {1}", file_path.string(), e.what());
      }
    }
  }

  int get_number(void) const {
    return number_;
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json::object({
        {"number", number_},
    });
  }

  void async_save_to_file(const std::filesystem::path& file_path) {
    json_writer::async_save_to_file(to_json(), file_path, 0755, 0644);
  }

private:
  int number_;
};
} // namespace krbn
