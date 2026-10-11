#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "../connection.hpp"
#include "../dictionary.hpp"
#include "../error.hpp"
#include "../options.hpp"
#include "../peer.hpp"
#include "../peers.hpp"
#include "../reply_token.hpp"
#include "../types.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <dispatch/dispatch.h>
#include <expected>
#include <functional>
#include <memory>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <xpc/xpc.h>

namespace pqrs::osx::xpc::impl {

template <typename Options>
class transport : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard constructor_guard_{*this};

public:
  // All signals and completions run on the supplied dispatcher.
  // started means activated locally, not that launchd has confirmed registration.
  // Terminal failure: this transport stops processing work. Pending operations
  // are abandoned; the owner must reconcile application state when recreating it.
  // Handlers must not throw and must enqueue destruction/reinitialization, rather
  // than destroying this transport during signal emission.
  nod::signal<void(const error_info&)> error_occurred;
  nod::signal<void()> listener_started;
  // Terminal listener failure. This transport cannot be started again; its
  // owner must defer destruction and create a replacement listener.
  nod::signal<void(const std::error_code&)> listener_terminated;
  nod::signal<void(const std::error_code&)> connection_failed;
  nod::signal<void(peer_id)> peer_ready;
  nod::signal<void(peer_id)> peer_interrupted;
  nod::signal<void(peer_id, const std::error_code&)> peer_invalidated;
  nod::signal<void(peer_id, std::shared_ptr<const std::vector<uint8_t>>)> message_received;
  nod::signal<void(peer_id, reply_token, std::shared_ptr<const std::vector<uint8_t>>)> request_received;

  transport(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher,
            const Options& options)
      : dispatcher_client(dispatcher),
        options_(options) {
    static_assert(std::is_same_v<Options,
                                 listener_options> ||
                  std::is_same_v<Options,
                                 client_options>);

    constructor_guard_.initialize([&] {
      if (!options_.validate()) {
        throw std::invalid_argument("invalid XPC transport limits");
      }

      queue_ = dispatch_queue_create("org.pqrs.xpc.transport",
                                     DISPATCH_QUEUE_SERIAL);
    });
  }

  transport(const transport&) = delete;

  ~transport() override {
    detach_from_dispatcher();

    dispatch_sync(
        queue_,
        ^{
          *alive_ = false;
          listener_.cancel();
          peers_.clear();
        });

    dispatch_release(queue_);
  }

  void async_start() {
    dispatch_async(
        queue_,
        ^{
          if (started_) {
            return;
          }

          started_ = true;

          run_guarded([&] {
            start();
          });

          tick();
        });
  }

  // For anonymous listeners, after listener_started.
  [[nodiscard]] object copy_endpoint() const {
    __block object result;

    dispatch_sync(
        queue_,
        ^{
          result = listener_.copy_endpoint();
        });

    return result;
  }

  // Constructing arguments or copying them for submission can still throw on
  // the calling thread. Exceptions during queued work are reported asynchronously.
  // Send operations retain the payload until queued work completes.
  // Callers must not mutate it through another shared reference.
  void async_send(peer_id id,
                  pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>> data) {
    dispatch_async(
        queue_,
        ^{
          run_guarded([&] {
            if (auto peer = peers_.find(id); peer && peer->ready_for_communication()) {
              if (options_.common_parameters.max_message_size &&
                  data->size() > *options_.common_parameters.max_message_size) {
                invalidate(id,
                           errc::message_too_large);
                return;
              }

              auto message = make_message(data);
              if (!peer->get_connection().send_message(message)) {
                invalidate(id,
                           errc::connection_invalid);
              }
            }
          });
        });
  }

  struct async_request_parameters final {
    peer_id id;
    pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>> data;
    pqrs::osx::xpc::completion completion;
  };

  void async_request(async_request_parameters parameters) {
    dispatch_async(
        queue_,
        ^{
          run_guarded([&] {
            auto peer = peers_.find(parameters.id);
            if (!peer || !peer->ready_for_communication()) {
              complete(parameters.completion,
                       std::unexpected(make_error_code(errc::not_ready)));

            } else if (options_.common_parameters.max_message_size &&
                       parameters.data->size() > *options_.common_parameters.max_message_size) {
              complete(parameters.completion,
                       std::unexpected(make_error_code(errc::message_too_large)));

            } else if (peer->get_pending_request_count() >= 128) {
              complete(parameters.completion,
                       std::unexpected(make_error_code(errc::too_many_requests)));

            } else {
              send_request({
                  .id = parameters.id,
                  .message = make_message(parameters.data),
                  .completion = parameters.completion,
                  .handshake = false,
              });
            }
          });
        });
  }

  void async_reply(reply_token token,
                   pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>> data) {
    dispatch_async(
        queue_,
        ^{
          run_guarded([&] {
            auto& state = *token.state_;

            auto found_peer = peers_.find(state.peer);
            // Check session identity before accessing the mutable reply dictionary,
            // which may belong to another transport's queue.
            if (!found_peer ||
                !found_peer->matches_session(state.session)) {
              return;
            }

            if (auto reply = std::move(state.reply_dictionary)) {
              if (options_.common_parameters.max_message_size &&
                  data->size() > *options_.common_parameters.max_message_size) {
                invalidate(state.peer,
                           errc::message_too_large);
                return;
              }

              reply.set_uint64(type_key,
                               static_cast<uint64_t>(message_type::data));
              reply.set_data(data_key,
                             *data);
              if (!found_peer->get_connection().send_message(reply)) {
                invalidate(state.peer,
                           errc::connection_invalid);
              }
            }
          });
        });
  }

  void async_cancel_peer(peer_id id) {
    dispatch_async(
        queue_,
        ^{
          run_guarded([&] {
            invalidate(id,
                       errc::cancelled);
          });
        });
  }

private:
  enum class message_type : uint64_t {
    hello = 1,
    hello_reply,
    data,
  };

  static constexpr char type_key[]{"type"};
  static constexpr char data_key[]{"data"};

  [[nodiscard]] static constexpr bool is_listener() noexcept {
    return std::is_same_v<Options,
                          listener_options>;
  }

  [[nodiscard]] std::chrono::milliseconds reconnect_interval() const noexcept {
    if constexpr (is_listener()) {
      // Listeners do not retry handshakes or reconnect their accepted peers.
      return std::chrono::milliseconds::zero();
    } else {
      return options_.client_parameters.reconnect_interval;
    }
  }

  // All transport state changes run on queue_. Never unwind into libdispatch.
  template <typename Function>
  void run_guarded(Function&& function) noexcept {
    if (failed_->load()) {
      return;
    }

    try {
      function();
    } catch (...) {
      stop_on_unexpected_exception();
    }
  }

  void stop_on_unexpected_exception() noexcept {
    try {
      if (stopped_) {
        return;
      }

      stopped_ = true;
      failed_->store(true);
      // Stop potentially partially updated state without replaying messages.
      listener_.cancel();
      peers_.clear();

      enqueue_to_dispatcher([this] {
        try {
          error_info error{make_error_code(errc::unexpected_exception),
                           "Unexpected XPC transport exception"};
          error_occurred(error);
        } catch (...) {
          std::fputs("XPC transport error creation or notification threw an exception\n",
                     stderr);
        }
      });
    } catch (...) {
      // Reporting is best effort; never unwind into libdispatch.
      std::fputs("XPC transport exception could not be reported\n", stderr);
    }
  }

  void terminate_listener(const std::error_code& error) {
    stopped_ = true;
    listener_.cancel();
    peers_.clear();

    enqueue_notification([this, error] {
      listener_terminated(error);
    });
  }

  template <typename Function>
  void enqueue_notification(Function&& function) {
    enqueue_to_dispatcher([this, failed = failed_, function = std::forward<Function>(function)] {
      if (failed->load()) {
        return;
      }

      try {
        function();
      } catch (...) {
        // Stop subsequent notifications immediately, before queue_ handles it.
        failed->store(true);
        auto alive = alive_;
        dispatch_async(
            queue_,
            ^{
              // This block can run after destruction. The captured lifetime flag
              // survives the transport and is accessed only on queue_. Check it
              // before touching this; stop_on_unexpected_exception() cleans up protocol state on this queue.
              if (!*alive) {
                return;
              }

              stop_on_unexpected_exception();
            });
      }
    });
  }

  bool apply_peer_signing_requirement(connection& connection) const {
    return !options_.common_parameters.signing_requirement ||
           connection.set_peer_code_signing_requirement(
               options_.common_parameters.signing_requirement->c_str()) == 0;
  }

  static errc connection_error(xpc_object_t event) {
    if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
      return errc::connection_interrupted;
    }

    if (event == XPC_ERROR_PEER_CODE_SIGNING_REQUIREMENT) {
      return errc::signature_rejected;
    }

    return errc::connection_invalid;
  }

  static dictionary make_message(const pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>>& data) {
    dictionary result;
    result.set_uint64(type_key, static_cast<uint64_t>(message_type::data));
    result.set_data(data_key,
                    *data);
    return result;
  }

  struct received_message final {
    message_type type;
    std::shared_ptr<const std::vector<uint8_t>> data;
  };

  std::expected<received_message, errc> read_message(xpc_object_t event,
                                                     std::optional<message_type> expected_type = {}) const {
    auto message = dictionary::from_xpc_object(event);
    if (!message) {
      return std::unexpected(connection_error(event));
    }

    auto raw_type = message->get_uint64(type_key);
    if (!raw_type) {
      return std::unexpected(errc::invalid_message);
    }

    auto type = static_cast<message_type>(*raw_type);
    if ((type != message_type::hello &&
         type != message_type::hello_reply &&
         type != message_type::data) ||
        (expected_type &&
         type != *expected_type) ||
        // hello_reply must arrive through the reply handler registered by
        // send_message_with_reply when sending hello, with expected_type set to
        // hello_reply. Reject hello_reply messages received through any other path.
        (!expected_type &&
         type == message_type::hello_reply)) {
      return std::unexpected(errc::invalid_message);
    }

    // Control messages have no application payload.
    if (type != message_type::data) {
      return received_message{
          .type = type,
          .data = {},
      };
    }

    auto value = message->get_data(data_key,
                                   {
                                       .max_size = options_.common_parameters.max_message_size,
                                   });
    if (!value) {
      return std::unexpected(value.error());
    }

    return received_message{
        .type = type,
        .data = std::make_shared<const std::vector<uint8_t>>(std::move(*value)),
    };
  }

  void start() {
    if constexpr (is_listener()) {
      listener_ = connection::create_listener(options_,
                                              queue_);
      if (!listener_.get()) {
        terminate_listener(make_error_code(errc::connection_invalid));

        return;
      }

      if (!apply_peer_signing_requirement(listener_)) {
        terminate_listener(make_error_code(errc::invalid_signing_requirement));

        return;
      }

      auto alive = alive_;
      listener_.set_event_handler(
          ^(xpc_object_t event) {
            if (!*alive) {
              return;
            }

            run_guarded([&] {
              if (xpc_get_type(event) == XPC_TYPE_CONNECTION) {
                add_peer(connection::from_peer(static_cast<xpc_connection_t>(event)));
              } else {
                auto error = make_error_code(connection_error(event));
                terminate_listener(error);
              }
            });
          });

      listener_.resume();

      enqueue_notification([this] {
        listener_started();
      });

    } else {
      auto connection = connection::create_client(options_,
                                                  queue_);
      if (!connection.get()) {
        enqueue_notification([this] {
          connection_failed(make_error_code(errc::connection_invalid));
        });

        reconnect_at_ = std::chrono::steady_clock::now() + reconnect_interval();

      } else if (!add_peer(std::move(connection))) {
        stopped_ = true;

        enqueue_notification([this] {
          connection_failed(make_error_code(errc::invalid_signing_requirement));
        });
      }
    }
  }

  bool add_peer(connection connection) {
    connection.set_target_queue(queue_);

    if (!apply_peer_signing_requirement(connection)) {
      return false;
    }

    auto added_peer = peers_.add(std::move(connection));
    auto id = added_peer.first;
    auto& peer_connection = added_peer.second.get_connection();

    auto alive = alive_;
    peer_connection.set_event_handler(
        ^(xpc_object_t event) {
          if (!*alive) {
            return;
          }

          run_guarded([&] {
            handle_event(id,
                         event);
          });
        });

    peer_connection.resume();

    if (!is_listener()) {
      begin_handshake(id);
    }

    return true;
  }

  void begin_handshake(peer_id id) {
    dictionary message;
    message.set_uint64(type_key,
                       static_cast<uint64_t>(message_type::hello));

    send_request({
        .id = id,
        .message = message,
        .completion = {},
        .handshake = true,
    });
  }

  void mark_ready_for_communication(peer_id id,
                                    peer& peer) {
    if (!peer.mark_ready_for_communication()) {
      return;
    }

    enqueue_notification([this, id] {
      peer_ready(id);
    });
  }

  // XPC transfers the request's reply context at most once.
  // Ordinary messages have no reply context.
  static std::optional<dictionary> create_reply(xpc_object_t request) noexcept {
    auto reply = adopt_xpc_object(xpc_dictionary_create_reply(request));
    return dictionary::from_xpc_object(reply.get());
  }

  void handle_event(peer_id id,
                    xpc_object_t event) {
    auto found_peer = peers_.find(id);
    if (!found_peer) {
      return;
    }

    auto data = read_message(event);
    if (!data) {
      handle_failure(id,
                     data.error());
      return;
    }

    auto reply = create_reply(event);
    if (data->type == message_type::hello) {
      if (!is_listener() ||
          !reply) {
        invalidate(id,
                   errc::invalid_message);
        return;
      }

      mark_ready_for_communication(id,
                                   *found_peer);
      reply->set_uint64(type_key,
                        static_cast<uint64_t>(message_type::hello_reply));

      if (!found_peer->get_connection().send_message(*reply)) {
        invalidate(id,
                   errc::connection_invalid);
      }

    } else {
      // Reply handlers and event handlers have no cross-handler ordering guarantee.
      // Any authenticated message from the server establishes its identity, even
      // if a notification is dispatched before the handshake reply handler.
      if (!found_peer->ready_for_communication()) {
        if (is_listener()) {
          invalidate(id,
                     errc::not_ready);
          return;
        }

        mark_ready_for_communication(id,
                                     *found_peer);
      }

      if (reply) {
        auto token = reply_token(reply_token::state{
            .reply_dictionary = std::move(*reply),
            .peer = id,
            .session = found_peer->get_session(),
        });

        enqueue_notification([this, id, token, data = data->data] {
          request_received(id,
                           token,
                           data);
        });

      } else {
        enqueue_notification([this, id, data = data->data] {
          message_received(id,
                           data);
        });
      }
    }
  }

  struct send_request_parameters final {
    peer_id id;
    // Borrowed only for the duration of send_request; never captured by queued work.
    const dictionary& message;
    pqrs::osx::xpc::completion completion;
    bool handshake;
  };

  void send_request(const send_request_parameters& parameters) {
    // The asynchronous reply handler needs its own copy of the peer ID.
    auto id = parameters.id;

    auto peer = peers_.find(id);
    if (!peer) {
      complete(parameters.completion,
               std::unexpected(make_error_code(errc::not_ready)));
      return;
    }

    // Request ID wraparound is intentionally permitted.
    auto request = ++next_request_id_;

    peer->add_pending_request(
        request,
        pqrs::osx::xpc::peer::pending_request{
            .completion = parameters.completion,
            .deadline = std::chrono::steady_clock::now() + options_.common_parameters.request_timeout,
            .handshake = parameters.handshake,
        });

    auto alive = alive_;
    connection::send_message_with_reply_parameters send_message_parameters{
        .message = parameters.message,
        .queue = queue_,
    };
    // Assign the Block separately because clang-format misformats a Block in a designated initializer.
    send_message_parameters.handler = ^(xpc_object_t event) {
      if (!*alive) {
        return;
      }

      run_guarded([&] {
        auto found_peer = peers_.find(id);
        if (!found_peer) {
          return;
        }

        auto request_state = found_peer->take_pending_request(request);
        if (!request_state) {
          return;
        }

        auto data = read_message(event,
                                 request_state->handshake
                                     ? message_type::hello_reply
                                     : message_type::data);
        if (!data) {
          // A reply error describes this request, not necessarily the connection.
          // Only the connection event handler emits peer_interrupted.
          complete(request_state->completion,
                   std::unexpected(make_error_code(data.error())));

          if (request_state->handshake &&
              data.error() == errc::connection_interrupted) {
            // XPC can reuse this connection. Retry the probe without replacing it.
            found_peer->schedule_handshake_retry(std::chrono::steady_clock::now() + reconnect_interval());

            auto error = make_error_code(data.error());
            enqueue_notification([this, error] {
              connection_failed(error);
            });

          } else {
            if constexpr (!is_listener()) {
              if (data.error() == errc::signature_rejected) {
                // A client-side signing requirement cannot become satisfied by retrying.
                stopped_ = true;
              }
            }

            if (request_state->handshake ||
                xpc_get_type(event) == XPC_TYPE_DICTIONARY) {
              invalidate(id,
                         data.error());
            }
          }

          return;
        }

        if (request_state->handshake) {
          mark_ready_for_communication(id,
                                       *found_peer);
        } else {
          complete(request_state->completion,
                   pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>>(data->data));
        }
      });
    };

    auto sent = peer->get_connection().send_message_with_reply(send_message_parameters);
    if (!sent) {
      // Invalidation consumes pending requests and reports their failure immediately.
      invalidate(id,
                 errc::connection_invalid);
    }
  }

  void complete(completion callback,
                std::expected<pqrs::not_null_shared_ptr_t<const std::vector<uint8_t>>, std::error_code> result) {
    if (callback) {
      enqueue_notification([callback, result] {
        callback(result);
      });
    }
  }

  void fail_all_pending_requests(peer& peer,
                                 errc error) {
    for (const auto& [id, request] : peer.take_pending_requests()) {
      complete(request.completion,
               std::unexpected(make_error_code(error)));
    }
  }

  void handle_failure(peer_id id,
                      errc error) {
    if (error != errc::connection_interrupted) {
      invalidate(id,
                 error);
      return;
    }

    auto peer = peers_.find(id);
    if (!peer ||
        !peer->mark_connection_interrupted(std::chrono::steady_clock::now() + reconnect_interval())) {
      return;
    }

    fail_all_pending_requests(*peer,
                              error);
    enqueue_notification([this, id] {
      peer_interrupted(id);
    });
  }

  void invalidate(peer_id id,
                  errc reason) {
    if (auto found_peer = peers_.find(id); found_peer) {
      fail_all_pending_requests(*found_peer,
                                reason);

      peers_.erase(id);

      auto error = make_error_code(reason);
      enqueue_notification([this, id, error] {
        peer_invalidated(id,
                         error);
      });

      if (!is_listener()) {
        reconnect_at_ = std::chrono::steady_clock::now() + reconnect_interval();
      }
    }
  }

  void tick() noexcept {
    run_guarded([&] {
      auto now = std::chrono::steady_clock::now();
      auto actions = peers_.collect_tick_actions(now);
      if (!is_listener()) {
        for (auto id : actions.handshake_retry_target_peer_ids) {
          begin_handshake(id);
        }
      }

      for (auto id : actions.expired_peer_ids) {
        invalidate(id,
                   errc::request_timeout);
      }

      if (!stopped_ &&
          !is_listener() &&
          peers_.empty() &&
          now >= reconnect_at_) {
        start();
      }
    });

    // tick can stop during its work. Do not reserve another timer in that case.
    if (stopped_ ||
        failed_->load()) {
      return;
    }

    auto alive = alive_;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW,
                      std::chrono::duration_cast<std::chrono::nanoseconds>(
                          options_.common_parameters.tick_interval)
                          .count()),
        queue_,
        ^{
          if (!*alive) {
            return;
          }

          tick();
        });
  }

  const Options options_;

  dispatch_queue_t queue_{nullptr};
  pqrs::not_null_shared_ptr_t<bool> alive_{std::make_shared<bool>(true)}; // only accessed on queue_
  pqrs::not_null_shared_ptr_t<std::atomic<bool>> failed_{std::make_shared<std::atomic<bool>>(false)};
  bool stopped_{false}; // only accessed on queue_; set after a terminal failure
  bool started_{false};
  connection listener_;
  peers peers_;
  uint64_t next_request_id_{0};
  std::chrono::steady_clock::time_point reconnect_at_{};
};
} // namespace pqrs::osx::xpc::impl
