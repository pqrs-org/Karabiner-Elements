#include "core_configuration.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"

bool libkrbn_core_configuration_initialize(libkrbn_core_configuration** out, const char* file_path) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_core_configuration*>(new core_configuration(libkrbn::get_logger(), file_path));
  return true;
}

void libkrbn_core_configuration_terminate(libkrbn_core_configuration** p) {
  if (p && *p) {
    delete reinterpret_cast<core_configuration*>(*p);
    *p = nullptr;
  }
}

bool libkrbn_core_configuration_is_loaded(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return c->is_loaded();
  }
  return false;
}

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* _Nonnull p) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return (c->get_global_configuration()).get_check_for_updates_on_startup();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* _Nonnull p, bool value) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    (c->get_global_configuration()).set_check_for_updates_on_startup(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* _Nonnull p) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return (c->get_global_configuration()).get_show_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* _Nonnull p, bool value) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return (c->get_global_configuration()).set_show_in_menu_bar(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* _Nonnull p) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return (c->get_global_configuration()).get_show_profile_name_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* _Nonnull p, bool value) {
  if (auto c = reinterpret_cast<core_configuration*>(p)) {
    return (c->get_global_configuration()).set_show_profile_name_in_menu_bar(value);
  }
}
