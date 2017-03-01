#include "constants.hpp"
#include "libkrbn.h"
#include "libkrbn.hpp"
#include "system_preferences.hpp"
#include "system_preferences_monitor.hpp"

namespace {
class libkrbn_system_preferences_monitor_class final {
public:
  libkrbn_system_preferences_monitor_class(const libkrbn_system_preferences_monitor_class&) = delete;

  libkrbn_system_preferences_monitor_class(libkrbn_system_preferences_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
    system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(libkrbn::get_logger(),
                                                                               [this](const system_preferences::values& values) { cpp_callback(values); });
  }

private:
  void cpp_callback(const system_preferences::values& values) {
    if (callback_) {
      libkrbn_system_preferences_values v;
      v.keyboard_fn_state = values.get_keyboard_fn_state();
      callback_(&v, refcon_);
    }
  }

  libkrbn_system_preferences_monitor_callback callback_;
  void* refcon_;

  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;
};
}

bool libkrbn_system_preferences_monitor_initialize(libkrbn_system_preferences_monitor* _Nullable* _Nonnull out,
                                                   libkrbn_system_preferences_monitor_callback _Nullable callback,
                                                   void* _Nullable refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_system_preferences_monitor*>(new libkrbn_system_preferences_monitor_class(callback, refcon));
  return true;
}

void libkrbn_system_preferences_monitor_terminate(libkrbn_system_preferences_monitor* _Nullable* _Nonnull p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_system_preferences_monitor_class*>(*p);
    *p = nullptr;
  }
}
