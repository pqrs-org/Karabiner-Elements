#pragma once

#include "libkrbn/libkrbn.h"

template <typename T>
class libkrbn_callback_manager final {
public:
  libkrbn_callback_manager(const libkrbn_callback_manager&) = delete;

  libkrbn_callback_manager(void) {
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

  void trigger(void) {
    for (const auto& callback : callbacks_) {
      callback();
    }
  }

private:
  std::vector<T> callbacks_;
};
