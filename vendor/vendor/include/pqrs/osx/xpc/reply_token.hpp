#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "dictionary.hpp"
#include "peer.hpp"
#include "types.hpp"
#include <memory>
#include <pqrs/gsl.hpp>
#include <utility>

namespace pqrs::osx::xpc {

namespace impl {
template <typename Options>
class transport;
}

// Copies share a single-use native XPC reply. Only the owning transport may send it.
class reply_token final {
  template <typename Options>
  friend class impl::transport;

  struct state {
    // Created from the incoming request, preserving its native XPC reply context.
    // async_reply moves out this dictionary, leaving it empty to prevent
    // duplicate replies through any copy of this token.
    pqrs::osx::xpc::dictionary reply_dictionary;

    peer_id peer;

    // Identifies one peer session in one transport. The peer replaces its
    // identity on interruption, so pointer comparison rejects both stale tokens
    // and tokens issued by another transport. This weak reference does not keep
    // an ended session alive.
    std::weak_ptr<pqrs::osx::xpc::peer::session_identity> session;
  };

  explicit reply_token(state value)
      : state_(std::make_shared<state>(std::move(value))) {}

  pqrs::not_null_shared_ptr_t<state> state_;
};

} // namespace pqrs::osx::xpc
