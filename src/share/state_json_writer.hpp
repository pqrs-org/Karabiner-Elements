#pragma once

#include "json_writer.hpp"

namespace krbn {
class state_json_writer final {
public:
  state_json_writer(const std::string& file_path) : file_path_(file_path),
                                                    state_(nlohmann::json::object()) {
    sync_save();
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
