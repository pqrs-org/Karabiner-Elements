#pragma once

#include "complex_modifications_assets_manager.hpp"
#include "core_configuration/core_configuration.hpp"
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <utility>

class settings_complex_modifications_assets_manager final {
public:
  settings_complex_modifications_assets_manager(const settings_complex_modifications_assets_manager&) = delete;

  settings_complex_modifications_assets_manager() {
    krbn::logger::get_logger()->debug(__func__);

    manager_ = std::make_unique<krbn::complex_modifications_assets_manager>();
  }

  ~settings_complex_modifications_assets_manager() {
    krbn::logger::get_logger()->debug(__func__);
  }

  nlohmann::json reload_and_get_files_json() const {
    manager_->reload(krbn::constants::get_user_complex_modifications_assets_directory(),
                     krbn::core_configuration::error_handling::loose);

    auto json = nlohmann::json::array();

    const auto& files = manager_->get_files();
    for (const auto& file : files) {
      auto rules_json = nlohmann::json::array();
      const auto& rules = file->get_rules();
      for (size_t rule_index = 0; rule_index < rules.size(); ++rule_index) {
        rules_json.push_back({
            {"file_path", file->get_file_path().string()},
            {"rule_index", rule_index},
            {"description", rules[rule_index]->get_description()},
            {"description_notes", rules[rule_index]->get_description_notes()},
        });
      }

      std::chrono::seconds imported_at(0);
      if (auto t = file->last_write_time()) {
        imported_at = std::chrono::duration_cast<std::chrono::seconds>(t->time_since_epoch());
      }

      json.push_back({
          {"file_path", file->get_file_path().string()},
          {"title", file->get_title()},
          {"user_file", file->user_file()},
          {"imported_at", imported_at.count()},
          {"asset_rules", std::move(rules_json)},
      });
    }

    return json;
  }

  void erase_file(const std::filesystem::path& file_path) const {
    if (auto file = find_file(file_path)) {
      file->unlink_file();
    }
  }

  void add_rule_to_core_configuration_selected_profile(const std::filesystem::path& file_path,
                                                       size_t index,
                                                       krbn::core_configuration::core_configuration& core_configuration) const {
    if (auto r = find_rule(file_path, index)) {
      core_configuration.get_selected_profile().get_complex_modifications()->push_front_rule(r);
    } else {
      throw std::runtime_error("The asset rule is no longer available. Reload the asset list.");
    }
  }

private:
  [[nodiscard]] std::shared_ptr<krbn::complex_modifications_assets_file> find_file(const std::filesystem::path& file_path) const {
    const auto& files = manager_->get_files();
    if (auto it = std::ranges::find(files,
                                    file_path, &krbn::complex_modifications_assets_file::get_file_path);
        it != files.end()) {
      return *it;
    }
    return nullptr;
  }

  [[nodiscard]] std::shared_ptr<krbn::core_configuration::details::complex_modifications_rule> find_rule(const std::filesystem::path& file_path,
                                                                                                         size_t index) const {
    if (auto f = find_file(file_path)) {
      auto& rules = f->get_rules();
      if (index < rules.size()) {
        return rules[index];
      }
    }
    return nullptr;
  }

  std::unique_ptr<krbn::complex_modifications_assets_manager> manager_;
};
