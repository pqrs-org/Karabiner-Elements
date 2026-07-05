#pragma once

// pqrs::osx::application v2.0.0

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/application/impl/impl.h>

#include <functional>
#include <utility>

namespace pqrs::osx::application {

enum class activation_policy {
  regular = pqrs_osx_application_activation_policy_regular,
  accessory = pqrs_osx_application_activation_policy_accessory,
  prohibited = pqrs_osx_application_activation_policy_prohibited,
};

enum class terminate_reply {
  now = pqrs_osx_application_terminate_reply_now,
  cancel = pqrs_osx_application_terminate_reply_cancel,
};

namespace impl {

inline std::function<terminate_reply(void)> should_terminate_callback;

inline pqrs_osx_application_terminate_reply_t should_terminate_callback_trampoline(void) noexcept {
  if (should_terminate_callback) {
    try {
      return static_cast<pqrs_osx_application_terminate_reply_t>(should_terminate_callback());
    } catch (...) {
      return pqrs_osx_application_terminate_reply_now;
    }
  }

  return pqrs_osx_application_terminate_reply_now;
}

} // namespace impl

inline void set_activation_policy(activation_policy policy) noexcept {
  pqrs_osx_application_set_activation_policy(static_cast<pqrs_osx_application_activation_policy_t>(policy));
}

inline void finish_launching() noexcept {
  pqrs_osx_application_finish_launching();
}

inline void run() noexcept {
  pqrs_osx_application_run();
}

inline void stop() noexcept {
  pqrs_osx_application_stop();
}

// The callback is called from NSApplicationDelegate.applicationShouldTerminate.
// Exceptions are not propagated across the AppKit/Swift boundary; they are treated as terminate_reply::now.
inline void set_should_terminate_callback(std::function<terminate_reply(void)> callback) {
  impl::should_terminate_callback = std::move(callback);

  if (impl::should_terminate_callback) {
    pqrs_osx_application_set_should_terminate_callback(impl::should_terminate_callback_trampoline);
  } else {
    pqrs_osx_application_set_should_terminate_callback(nullptr);
  }
}

} // namespace pqrs::osx::application
