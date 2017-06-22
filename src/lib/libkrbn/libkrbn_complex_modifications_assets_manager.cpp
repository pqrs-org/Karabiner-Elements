#include "complex_modifications_assets_manager.hpp"
#include "libkrbn.h"

bool libkrbn_complex_modifications_assets_manager_initialize(libkrbn_complex_modifications_assets_manager** out) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_complex_modifications_assets_manager*>(new krbn::complex_modifications_assets_manager());
  return true;
}

void libkrbn_complex_modifications_assets_manager_terminate(libkrbn_complex_modifications_assets_manager** p) {
  if (p && *p) {
    delete reinterpret_cast<krbn::complex_modifications_assets_manager*>(*p);
    *p = nullptr;
  }
}

void libkrbn_complex_modifications_assets_manager_reload(libkrbn_complex_modifications_assets_manager* p) {
  if (auto m = reinterpret_cast<krbn::complex_modifications_assets_manager*>(p)) {
    m->reload(krbn::constants::get_user_complex_modifications_assets_directory());
  }
}

namespace {
const std::vector<krbn::complex_modifications_assets_manager::file>* get_files(libkrbn_complex_modifications_assets_manager* p) {
  if (auto m = reinterpret_cast<krbn::complex_modifications_assets_manager*>(p)) {
    return &(m->get_files());
  }
  return nullptr;
}

const std::vector<krbn::core_configuration::profile::complex_modifications::rule>* get_rules(libkrbn_complex_modifications_assets_manager* p, size_t index) {
  if (auto files = get_files(p)) {
    if (index < files->size()) {
      auto& f = (*files)[index];
      return &(f.get_rules());
    }
  }
  return nullptr;
}
} // namespace

size_t libkrbn_complex_modifications_assets_manager_get_files_size(libkrbn_complex_modifications_assets_manager* p) {
  if (auto files = get_files(p)) {
    return files->size();
  }
  return 0;
}

const char* libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager* p, size_t index) {
  if (auto files = get_files(p)) {
    if (index < files->size()) {
      auto& f = (*files)[index];
      return f.get_title().c_str();
    }
  }
  return nullptr;
}

size_t libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbn_complex_modifications_assets_manager* p, size_t file_index) {
  if (auto rules = get_rules(p, file_index)) {
    return rules->size();
  }
  return 0;
}

const char* libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbn_complex_modifications_assets_manager* p, size_t file_index, size_t index) {
  if (auto rules = get_rules(p, file_index)) {
    if (index < rules->size()) {
      auto& r = (*rules)[index];
      return r.get_description().c_str();
    }
  }
  return nullptr;
}
