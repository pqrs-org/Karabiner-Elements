#include "constants.hpp"
#include "libkrbn.h"
#include "libkrbn_cpp.hpp"
#include "monitor/system_preferences_monitor.hpp"

namespace {
class libkrbn_system_preferences_monitor_class final {
public:
  libkrbn_system_preferences_monitor_class(const libkrbn_system_preferences_monitor_class&) = delete;

  libkrbn_system_preferences_monitor_class(libkrbn_system_preferences_monitor_callback callback,
                                           void* refcon,
                                           std::weak_ptr<krbn::configuration_monitor> weak_configuration_monitor) {
    system_preferences_monitor_ = std::make_unique<krbn::system_preferences_monitor>(weak_configuration_monitor);

    system_preferences_monitor_->system_preferences_changed.connect([callback, refcon](auto&& system_preferences) {
      if (callback) {
        libkrbn_system_preferences v;
        v.keyboard_fn_state = system_preferences.get_keyboard_fn_state();
        callback(&v, refcon);
      }
    });

    system_preferences_monitor_->async_start();
  }

  ~libkrbn_system_preferences_monitor_class(void) {
    system_preferences_monitor_ = nullptr;
  }

private:
  std::unique_ptr<krbn::system_preferences_monitor> system_preferences_monitor_;
};
} // namespace

bool libkrbn_system_preferences_monitor_initialize(libkrbn_system_preferences_monitor* _Nullable* _Nonnull out,
                                                   libkrbn_system_preferences_monitor_callback _Nullable callback,
                                                   void* _Nullable refcon,
                                                   libkrbn_configuration_monitor* _Nonnull libkrbn_configuration_monitor) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  auto configuration_monitor_class = reinterpret_cast<libkrbn_cpp::libkrbn_configuration_monitor_class*>(libkrbn_configuration_monitor);
  if (!configuration_monitor_class) return false;

  auto configuration_monitor = configuration_monitor_class->get_configuration_monitor();

  *out = reinterpret_cast<libkrbn_system_preferences_monitor*>(new libkrbn_system_preferences_monitor_class(callback,
                                                                                                            refcon,
                                                                                                            configuration_monitor));
  return true;
}

void libkrbn_system_preferences_monitor_terminate(libkrbn_system_preferences_monitor* _Nullable* _Nonnull p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_system_preferences_monitor_class*>(*p);
    *p = nullptr;
  }
}
