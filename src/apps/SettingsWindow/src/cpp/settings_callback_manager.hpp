#pragma once

#include "settings.hpp"

template <typename T>
class settings_callback_manager final {
public:
  settings_callback_manager(const settings_callback_manager&) = delete;

  settings_callback_manager() {
  }

  [[nodiscard]] const std::vector<T>& get_callbacks() const {
    return callbacks_;
  }

  void register_callback(T callback) {
    callbacks_.push_back(callback);
  }

  void unregister_callback(T callback) {
    callbacks_.erase(std::remove_if(std::begin(callbacks_),
                                    std::end(callbacks_),
                                    [&](auto& c) {
                                      return c == callback;
                                    }),
                     std::end(callbacks_));
  }

private:
  std::vector<T> callbacks_;
};
