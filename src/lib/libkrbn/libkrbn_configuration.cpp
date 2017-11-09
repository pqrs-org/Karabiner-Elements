#include "configuration_monitor.hpp"
#include "core_configuration.hpp"
#include "libkrbn.h"
#include "libkrbn_cpp.hpp"

namespace {
class libkrbn_core_configuration_class final {
public:
  libkrbn_core_configuration_class(std::shared_ptr<krbn::core_configuration> core_configuration) : core_configuration_(core_configuration) {
  }

  krbn::core_configuration& get_core_configuration(void) {
    return *core_configuration_;
  }

private:
  std::shared_ptr<krbn::core_configuration> core_configuration_;
};

class libkrbn_configuration_monitor_class final {
public:
  libkrbn_configuration_monitor_class(const libkrbn_configuration_monitor_class&) = delete;

  libkrbn_configuration_monitor_class(libkrbn_configuration_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    configuration_monitor_ = std::make_unique<krbn::configuration_monitor>(
        krbn::constants::get_user_core_configuration_file_path(),
        [this](const std::shared_ptr<krbn::core_configuration> core_configuration) {
          if (callback_) {
            auto* p = new libkrbn_core_configuration_class(core_configuration);
            callback_(p, refcon_);
          }
        });
  }

private:
  libkrbn_configuration_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<krbn::configuration_monitor> configuration_monitor_;
};

krbn::core_configuration::profile::simple_modifications* find_simple_modifications(libkrbn_core_configuration* p,
                                                                                   const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      return c->get_core_configuration().get_selected_profile().find_simple_modifications(libkrbn_cpp::make_device_identifiers(*device_identifiers));
    } else {
      return &(c->get_core_configuration().get_selected_profile().get_simple_modifications());
    }
  }
  return nullptr;
}

krbn::core_configuration::profile::simple_modifications* find_fn_function_keys(libkrbn_core_configuration* p,
                                                                               const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      return c->get_core_configuration().get_selected_profile().find_fn_function_keys(libkrbn_cpp::make_device_identifiers(*device_identifiers));
    } else {
      return &(c->get_core_configuration().get_selected_profile().get_fn_function_keys());
    }
  }
  return nullptr;
}
} // namespace

void libkrbn_core_configuration_terminate(libkrbn_core_configuration** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_core_configuration_class*>(*p);
    *p = nullptr;
  }
}

bool libkrbn_core_configuration_save(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().save_to_file(krbn::constants::get_user_core_configuration_file_path());
  }
  return false;
}

bool libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_check_for_updates_on_startup();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_global_configuration().set_check_for_updates_on_startup(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_show_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_show_in_menu_bar(value);
  }
}

bool libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().get_show_profile_name_in_menu_bar();
  }
  return false;
}

void libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbn_core_configuration* p, bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_global_configuration().set_show_profile_name_in_menu_bar(value);
  }
}

size_t libkrbn_core_configuration_get_profiles_size(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_profiles().size();
  }
  return 0;
}

const char* libkrbn_core_configuration_get_profile_name(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& profiles = c->get_core_configuration().get_profiles();
    if (index < profiles.size()) {
      return profiles[index].get_name().c_str();
    }
  }
  return nullptr;
}

void libkrbn_core_configuration_set_profile_name(libkrbn_core_configuration* p, size_t index, const char* value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (value) {
      c->get_core_configuration().set_profile_name(index, value);
    }
  }
}

bool libkrbn_core_configuration_get_profile_selected(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& profiles = c->get_core_configuration().get_profiles();
    if (index < profiles.size()) {
      return profiles[index].get_selected();
    }
  }
  return false;
}

void libkrbn_core_configuration_select_profile(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().select_profile(index);
  }
}

const char* libkrbn_core_configuration_get_selected_profile_name(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_name().c_str();
  }
  return nullptr;
}

void libkrbn_core_configuration_push_back_profile(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().push_back_profile();
  }
}

void libkrbn_core_configuration_erase_profile(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().erase_profile(index);
  }
}

size_t libkrbn_core_configuration_get_selected_profile_simple_modifications_size(libkrbn_core_configuration* p,
                                                                                 const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    return m->get_pairs().size();
  }
  return 0;
}

libkrbn_simple_modifications_definition libkrbn_core_configuration_get_selected_profile_simple_modification_first(libkrbn_core_configuration* p,
                                                                                                                  size_t index,
                                                                                                                  const libkrbn_device_identifiers* device_identifiers) {
  libkrbn_simple_modifications_definition d;
  d.type = nullptr;
  d.value = nullptr;

  if (auto m = find_simple_modifications(p, device_identifiers)) {
    const auto& pairs = m->get_pairs();
    if (index < pairs.size()) {
      d.type = pairs[index].first.get_type().c_str();
      d.value = pairs[index].first.get_value().c_str();
    }
  }
  return d;
}

libkrbn_simple_modifications_definition libkrbn_core_configuration_get_selected_profile_simple_modification_second(libkrbn_core_configuration* p,
                                                                                                                   size_t index,
                                                                                                                   const libkrbn_device_identifiers* device_identifiers) {
  libkrbn_simple_modifications_definition d;
  d.type = nullptr;
  d.value = nullptr;

  if (auto m = find_simple_modifications(p, device_identifiers)) {
    const auto& pairs = m->get_pairs();
    if (index < pairs.size()) {
      d.type = pairs[index].second.get_type().c_str();
      d.value = pairs[index].second.get_value().c_str();
    }
  }
  return d;
}

void libkrbn_core_configuration_replace_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                             size_t index,
                                                                             const libkrbn_simple_modifications_definition* from,
                                                                             const libkrbn_simple_modifications_definition* to,
                                                                             const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    if (from &&
        from->type &&
        from->value &&
        to &&
        to->type &&
        to->value) {
      m->replace_pair(index, {from->type, from->value}, {to->type, to->value});
    }
  }
}

void libkrbn_core_configuration_push_back_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                               const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    m->push_back_pair();
  }
}

void libkrbn_core_configuration_erase_selected_profile_simple_modification(libkrbn_core_configuration* p,
                                                                           size_t index,
                                                                           const libkrbn_device_identifiers* device_identifiers) {
  if (auto m = find_simple_modifications(p, device_identifiers)) {
    m->erase_pair(index);
  }
}

size_t libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(libkrbn_core_configuration* p,
                                                                             const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    return k->get_pairs().size();
  }
  return 0;
}

libkrbn_simple_modifications_definition libkrbn_core_configuration_get_selected_profile_fn_function_key_first(libkrbn_core_configuration* p,
                                                                                                              size_t index,
                                                                                                              const libkrbn_device_identifiers* device_identifiers) {
  libkrbn_simple_modifications_definition d;
  d.type = nullptr;
  d.value = nullptr;

  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    const auto& pairs = k->get_pairs();
    if (index < pairs.size()) {
      d.type = pairs[index].first.get_type().c_str();
      d.value = pairs[index].first.get_value().c_str();
    }
  }
  return d;
}

libkrbn_simple_modifications_definition libkrbn_core_configuration_get_selected_profile_fn_function_key_second(libkrbn_core_configuration* p,
                                                                                                               size_t index,
                                                                                                               const libkrbn_device_identifiers* device_identifiers) {
  libkrbn_simple_modifications_definition d;
  d.type = nullptr;
  d.value = nullptr;

  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    const auto& pairs = k->get_pairs();
    if (index < pairs.size()) {
      d.type = pairs[index].second.get_type().c_str();
      d.value = pairs[index].second.get_value().c_str();
    }
  }
  return d;
}

void libkrbn_core_configuration_replace_selected_profile_fn_function_key(libkrbn_core_configuration* p,
                                                                         const libkrbn_simple_modifications_definition* from,
                                                                         const libkrbn_simple_modifications_definition* to,
                                                                         const libkrbn_device_identifiers* device_identifiers) {
  if (auto k = find_fn_function_keys(p, device_identifiers)) {
    if (from &&
        from->type &&
        from->value &&
        to &&
        to->type &&
        to->value) {
      k->replace_second({from->type, from->value}, {to->type, to->value});
    }
  }
}

size_t libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_complex_modifications().get_rules().size();
  }
  return 0;
}

void libkrbn_core_configuration_push_back_complex_modifications_rule_to_selected_profile(libkrbn_core_configuration* p,
                                                                                         const krbn::core_configuration::profile::complex_modifications::rule& rule) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().push_back_complex_modifications_rule(rule);
  }
}

void libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().erase_complex_modifications_rule(index);
  }
}

void libkrbn_core_configuration_swap_selected_profile_complex_modifications_rules(libkrbn_core_configuration* p, size_t index1, size_t index2) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().swap_complex_modifications_rules(index1, index2);
  }
}

const char* _Nullable libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(libkrbn_core_configuration* p, size_t index) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    const auto& rules = c->get_core_configuration().get_selected_profile().get_complex_modifications().get_rules();
    if (index < rules.size()) {
      return rules[index].get_description().c_str();
    }
  }
  return 0;
}

int libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                    const char* name) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (name) {
      return c->get_core_configuration().get_selected_profile().get_complex_modifications().get_parameters().get_value(name);
    }
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(libkrbn_core_configuration* p,
                                                                                     const char* name,
                                                                                     int value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (name) {
      c->get_core_configuration().get_selected_profile().set_complex_modifications_parameter(name, value);
    }
  }
}

const char* libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_virtual_hid_keyboard().get_keyboard_type().c_str();
  }
  return nullptr;
}

void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type(libkrbn_core_configuration* p, const char* value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (value) {
      c->get_core_configuration().get_selected_profile().get_virtual_hid_keyboard().set_keyboard_type(value);
    }
  }
}

int libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(libkrbn_core_configuration* p) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    return c->get_core_configuration().get_selected_profile().get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds();
  }
  return 0;
}

void libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_caps_lock_delay_milliseconds(libkrbn_core_configuration* p, uint32_t value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    c->get_core_configuration().get_selected_profile().get_virtual_hid_keyboard().set_caps_lock_delay_milliseconds(value);
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_ignore(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_ignore(libkrbn_core_configuration* p,
                                                                   const libkrbn_device_identifiers* device_identifiers,
                                                                   bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_ignore(identifiers, value);
    }
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_manipulate_caps_lock_led(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(libkrbn_core_configuration* p,
                                                                                     const libkrbn_device_identifiers* device_identifiers,
                                                                                     bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_manipulate_caps_lock_led(identifiers, value);
    }
  }
}

bool libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      return c->get_core_configuration().get_selected_profile().get_device_disable_built_in_keyboard_if_exists(identifiers);
    }
  }
  return false;
}

void libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(libkrbn_core_configuration* p,
                                                                                                const libkrbn_device_identifiers* device_identifiers,
                                                                                                bool value) {
  if (auto c = reinterpret_cast<libkrbn_core_configuration_class*>(p)) {
    if (device_identifiers) {
      auto identifiers = libkrbn_cpp::make_device_identifiers(*device_identifiers);
      c->get_core_configuration().get_selected_profile().set_device_disable_built_in_keyboard_if_exists(identifiers, value);
    }
  }
}

bool libkrbn_configuration_monitor_initialize(libkrbn_configuration_monitor** out, libkrbn_configuration_monitor_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_configuration_monitor*>(new libkrbn_configuration_monitor_class(callback, refcon));
  return true;
}

void libkrbn_configuration_monitor_terminate(libkrbn_configuration_monitor** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_configuration_monitor_class*>(*p);
    *p = nullptr;
  }
}
