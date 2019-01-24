#pragma once

#include "complex_modifications_assets_manager.hpp"
#include "libkrbn_configuration_monitor.hpp"

class libkrbn_complex_modifications_assets_manager final {
public:
  libkrbn_complex_modifications_assets_manager(const libkrbn_complex_modifications_assets_manager&) = delete;

  libkrbn_complex_modifications_assets_manager(void) {
    manager_ = std::make_unique<krbn::complex_modifications_assets_manager>();
  }

  void reload(void) const {
    manager_->reload(krbn::constants::get_user_complex_modifications_assets_directory());
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
                                                       libkrbn_core_configuration* core_configuration) const {
    if (auto r = find_rule(file_index, index)) {
      if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(core_configuration)) {
        c->get_core_configuration().get_selected_profile().push_back_complex_modifications_rule(*r);
      }
    }
  }

private:
  const krbn::complex_modifications_assets_manager::file* find_file(size_t index) const {
    auto& files = manager_->get_files();
    if (index < files.size()) {
      return &(files[index]);
    }
    return nullptr;
  }

  const krbn::core_configuration::details::complex_modifications_rule* find_rule(size_t file_index,
                                                                                 size_t index) const {
    if (auto f = find_file(file_index)) {
      auto& rules = f->get_rules();
      if (index < rules.size()) {
        return &(rules[index]);
      }
    }
    return nullptr;
  }

  std::unique_ptr<krbn::complex_modifications_assets_manager> manager_;
};
