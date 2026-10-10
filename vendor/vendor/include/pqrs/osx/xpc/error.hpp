#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <system_error>

namespace pqrs::osx::xpc {

enum class errc {
  connection_interrupted = 1,
  connection_invalid,
  signature_rejected,
  invalid_signing_requirement,
  invalid_message,
  message_too_large,
  not_ready,
  request_timeout,
  too_many_requests,
  cancelled,
  unexpected_exception,
};

inline std::error_code make_error_code(errc value) noexcept {
  class category final : public std::error_category {
  public:
    const char* name() const noexcept override {
      return "xpc";
    }

    std::string message(int value) const override {
      switch (static_cast<errc>(value)) {
        case errc::connection_interrupted:
          return "XPC connection interrupted";
        case errc::connection_invalid:
          return "XPC connection invalidated";
        case errc::signature_rejected:
          return "XPC peer signature rejected";
        case errc::invalid_signing_requirement:
          return "Invalid XPC signing requirement";
        case errc::invalid_message:
          return "Invalid XPC message";
        case errc::message_too_large:
          return "XPC payload exceeds size limit";
        case errc::not_ready:
          return "XPC peer is not ready";
        case errc::request_timeout:
          return "XPC request timed out";
        case errc::too_many_requests:
          return "Too many pending XPC requests";
        case errc::cancelled:
          return "XPC connection cancelled";
        case errc::unexpected_exception:
          return "Unexpected XPC transport exception";
      }
      return "Unknown XPC error";
    }
  };

  static category instance;
  return {
      static_cast<int>(value),
      instance,
  };
}

struct error_info final {
  std::error_code code;
  std::string message;
};

} // namespace pqrs::osx::xpc
