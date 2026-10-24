#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <bsm/audit.h>
#include <sys/types.h>

namespace pqrs::osx::xpc {

// Credentials captured by XPC when the connection was established.
class peer_identity final {
public:
  peer_identity(uid_t euid,
                gid_t egid,
                au_asid_t audit_session_id,
                pid_t pid) noexcept
      : euid_(euid),
        egid_(egid),
        audit_session_id_(audit_session_id),
        pid_(pid) {
  }

  [[nodiscard]] uid_t get_euid() const noexcept {
    return euid_;
  }

  [[nodiscard]] gid_t get_egid() const noexcept {
    return egid_;
  }

  [[nodiscard]] au_asid_t get_audit_session_id() const noexcept {
    return audit_session_id_;
  }

  // A PID can become stale and be reused after the peer exits. Keep it for
  // diagnostics rather than using it as a durable authorization identity.
  [[nodiscard]] pid_t get_pid() const noexcept {
    return pid_;
  }

private:
  uid_t euid_;
  gid_t egid_;
  au_asid_t audit_session_id_;
  pid_t pid_;
};

} // namespace pqrs::osx::xpc
