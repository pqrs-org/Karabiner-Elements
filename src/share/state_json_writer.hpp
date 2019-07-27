#pragma once

#include "json_writer.hpp"

namespace krbn {
class state_json_writer final {
public:
  state_json_writer(const std::string& file_path) : file_path_(file_path),
                                                    state_(nlohmann::json::object()) {
    std::ifstream input(file_path);
    if (input) {
      try {
        state_ = nlohmann::json::parse(input);
      } catch (std::exception& e) {
        logger::get_logger()->error("parse error in {0}: {1}", file_path, e.what());
      }
    }
  }

  template <typename T>
  void set(const std::string& key,
           const T& value) {
    state_[key] = value;

    sync_save();
  }

private:
  void sync_save(void) {
    json_writer::sync_save_to_file(state_,
                                   file_path_,
                                   0755,
                                   0644);
  }

  std::string file_path_;
  nlohmann::json state_;
};
} // namespace krbn
