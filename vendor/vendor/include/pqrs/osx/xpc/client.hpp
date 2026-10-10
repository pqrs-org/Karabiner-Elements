#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "impl/transport.hpp"

namespace pqrs::osx::xpc {

// Signals and completions run on the supplied dispatcher. Defer destruction
// until signal delivery has returned. Unexpected exceptions stop processing.
class client final : private impl::transport<client_options> {
public:
  client(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher,
         const client_options& options = {})
      : impl::transport<client_options>(dispatcher, options) {}

  using impl::transport<client_options>::async_cancel_peer;
  using impl::transport<client_options>::async_reply;
  using impl::transport<client_options>::async_request;
  using impl::transport<client_options>::async_request_parameters;
  using impl::transport<client_options>::async_send;
  using impl::transport<client_options>::async_start;
  using impl::transport<client_options>::connection_failed;
  using impl::transport<client_options>::dispatcher_thread;
  using impl::transport<client_options>::enqueue_to_dispatcher;
  using impl::transport<client_options>::error_occurred;
  using impl::transport<client_options>::message_received;
  using impl::transport<client_options>::peer_interrupted;
  using impl::transport<client_options>::peer_invalidated;
  using impl::transport<client_options>::peer_ready;
  using impl::transport<client_options>::request_received;
};

} // namespace pqrs::osx::xpc
