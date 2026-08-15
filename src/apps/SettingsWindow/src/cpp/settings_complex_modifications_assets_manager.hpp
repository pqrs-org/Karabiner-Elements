#pragma once

#include "complex_modifications_assets_manager.hpp"
#include "settings_configuration_monitor.hpp"
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
    for (size_t file_index = 0; file_index < files.size(); ++file_index) {
      const auto& file = files[file_index];

      auto rules_json = nlohmann::json::array();
      const auto& rules = file->get_rules();
      for (size_t rule_index = 0; rule_index < rules.size(); ++rule_index) {
        rules_json.push_back({
            {"file_index", file_index},
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
          {"index", file_index},
          {"title", file->get_title()},
          {"user_file", file->user_file()},
          {"imported_at", imported_at.count()},
          {"asset_rules", std::move(rules_json)},
      });
    }

    return json;
  }

  void erase_file(size_t index) const {
    if (auto f = find_file(index)) {
      f->unlink_file();
    }
  }

  void add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                       size_t index,
                                                       krbn::core_configuration::core_configuration& core_configuration) const {
    if (auto r = find_rule(file_index, index)) {
      core_configuration.get_selected_profile().get_complex_modifications()->push_front_rule(r);
    }
  }

private:
  [[nodiscard]] std::shared_ptr<krbn::complex_modifications_assets_file> find_file(size_t index) const {
    auto& files = manager_->get_files();
    if (index < files.size()) {
      return files[index];
    }
    return nullptr;
  }

  [[nodiscard]] std::shared_ptr<krbn::core_configuration::details::complex_modifications_rule> find_rule(size_t file_index,
                                                                                                         size_t index) const {
    if (auto f = find_file(file_index)) {
      auto& rules = f->get_rules();
      if (index < rules.size()) {
        return rules[index];
      }
    }
    return nullptr;
  }

  std::unique_ptr<krbn::complex_modifications_assets_manager> manager_;
};
