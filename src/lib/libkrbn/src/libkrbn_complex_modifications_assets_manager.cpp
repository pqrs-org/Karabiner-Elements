#include "complex_modifications_assets_manager.hpp"
#include "libkrbn/libkrbn.h"

// libkrbn_configuration.cpp
void libkrbn_core_configuration_push_back_complex_modifications_rule_to_selected_profile(libkrbn_core_configuration* p,
                                                                                         const krbn::core_configuration::details::complex_modifications_rule& rule);

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

const krbn::complex_modifications_assets_manager::file* get_file(libkrbn_complex_modifications_assets_manager* p,
                                                                 size_t index) {
  if (auto files = get_files(p)) {
    if (index < files->size()) {
      return &((*files)[index]);
    }
  }
  return nullptr;
}

const std::vector<krbn::core_configuration::details::complex_modifications_rule>* get_rules(libkrbn_complex_modifications_assets_manager* p,
                                                                                            size_t index) {
  if (auto files = get_files(p)) {
    if (index < files->size()) {
      auto& f = (*files)[index];
      return &(f.get_rules());
    }
  }
  return nullptr;
}

const krbn::core_configuration::details::complex_modifications_rule* get_rule(libkrbn_complex_modifications_assets_manager* p,
                                                                              size_t file_index,
                                                                              size_t index) {
  if (auto rules = get_rules(p, file_index)) {
    if (index < rules->size()) {
      return &((*rules)[index]);
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

const char* libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager* p,
                                                                        size_t index) {
  if (auto file = get_file(p, index)) {
    return file->get_title().c_str();
  }
  return nullptr;
}

size_t libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbn_complex_modifications_assets_manager* p,
                                                                        size_t file_index) {
  if (auto rules = get_rules(p, file_index)) {
    return rules->size();
  }
  return 0;
}

const char* libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbn_complex_modifications_assets_manager* p,
                                                                                   size_t file_index,
                                                                                   size_t index) {
  if (auto rule = get_rule(p, file_index, index)) {
    return rule->get_description().c_str();
  }
  return nullptr;
}

void libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(libkrbn_complex_modifications_assets_manager* p,
                                                                                                  size_t file_index,
                                                                                                  size_t index,
                                                                                                  libkrbn_core_configuration* q) {
  if (auto rule = get_rule(p, file_index, index)) {
    libkrbn_core_configuration_push_back_complex_modifications_rule_to_selected_profile(q, *rule);
  }
}

bool libkrbn_complex_modifications_assets_manager_is_user_file(libkrbn_complex_modifications_assets_manager* p,
                                                               size_t index) {
  if (auto file = get_file(p, index)) {
    return file->is_user_file();
  }
  return false;
}

void libkrbn_complex_modifications_assets_manager_erase_file(libkrbn_complex_modifications_assets_manager* p,
                                                             size_t index) {
  if (auto file = get_file(p, index)) {
    file->unlink_file();
  }
}
