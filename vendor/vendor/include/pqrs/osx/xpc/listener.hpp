#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "impl/transport.hpp"

namespace pqrs::osx::xpc {

// Signals and completions run on the supplied dispatcher. Defer destruction
// until signal delivery has returned. Unexpected exceptions stop processing.
class listener final : private impl::transport<listener_options> {
public:
  listener(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher,
           const listener_options& options = {})
      : impl::transport<listener_options>(dispatcher, options) {}

  using impl::transport<listener_options>::async_cancel_peer;
  using impl::transport<listener_options>::async_reply;
  using impl::transport<listener_options>::async_request;
  using impl::transport<listener_options>::async_request_parameters;
  using impl::transport<listener_options>::async_send;
  using impl::transport<listener_options>::async_start;
  using impl::transport<listener_options>::copy_endpoint;
  using impl::transport<listener_options>::dispatcher_thread;
  using impl::transport<listener_options>::enqueue_to_dispatcher;
  using impl::transport<listener_options>::error_occurred;
  using impl::transport<listener_options>::listener_failed;
  using impl::transport<listener_options>::listener_started;
  using impl::transport<listener_options>::message_received;
  using impl::transport<listener_options>::peer_interrupted;
  using impl::transport<listener_options>::peer_invalidated;
  using impl::transport<listener_options>::peer_ready;
  using impl::transport<listener_options>::request_received;
};

} // namespace pqrs::osx::xpc
