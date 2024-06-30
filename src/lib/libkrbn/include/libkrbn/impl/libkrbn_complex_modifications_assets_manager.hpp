#pragma once

#include "complex_modifications_assets_manager.hpp"
#include "libkrbn_configuration_monitor.hpp"

class libkrbn_complex_modifications_assets_manager final {
public:
  libkrbn_complex_modifications_assets_manager(const libkrbn_complex_modifications_assets_manager&) = delete;

  libkrbn_complex_modifications_assets_manager(void) {
    krbn::logger::get_logger()->info(__func__);

    manager_ = std::make_unique<krbn::complex_modifications_assets_manager>();
  }

  ~libkrbn_complex_modifications_assets_manager(void) {
    krbn::logger::get_logger()->info(__func__);
  }

  void reload(void) const {
    manager_->reload(krbn::constants::get_user_complex_modifications_assets_directory(),
                     krbn::core_configuration::error_handling::loose);
  }

  size_t get_files_size(void) const {
    return manager_->get_files().size();
  }

  const char* get_file_title(size_t index) const {
    if (auto f = find_file(index)) {
      return f->get_title().c_str();
    }
    return nullptr;
  }

  time_t get_file_last_write_time(size_t index) const {
    if (auto f = find_file(index)) {
      if (auto t = f->last_write_time()) {
        return std::chrono::duration_cast<std::chrono::seconds>(t->time_since_epoch()).count();
      }
    }
    return 0;
  }

  bool user_file(size_t index) const {
    if (auto f = find_file(index)) {
      return f->user_file();
    }
    return false;
  }

  void erase_file(size_t index) const {
    if (auto f = find_file(index)) {
      f->unlink_file();
    }
  }

  size_t get_rules_size(size_t file_index) const {
    if (auto f = find_file(file_index)) {
      return f->get_rules().size();
    }
    return 0;
  }

  const char* get_rule_description(size_t file_index,
                                   size_t index) const {
    if (auto r = find_rule(file_index, index)) {
      return r->get_description().c_str();
    }
    return nullptr;
  }

  void add_rule_to_core_configuration_selected_profile(size_t file_index,
                                                       size_t index,
                                                       krbn::core_configuration::core_configuration& core_configuration) const {
    if (auto r = find_rule(file_index, index)) {
      core_configuration.get_selected_profile().get_complex_modifications()->push_front_rule(r);
    }
  }

private:
  std::shared_ptr<krbn::complex_modifications_assets_file> find_file(size_t index) const {
    auto& files = manager_->get_files();
    if (index < files.size()) {
      return files[index];
    }
    return nullptr;
  }

  std::shared_ptr<krbn::core_configuration::details::complex_modifications_rule> find_rule(size_t file_index,
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
