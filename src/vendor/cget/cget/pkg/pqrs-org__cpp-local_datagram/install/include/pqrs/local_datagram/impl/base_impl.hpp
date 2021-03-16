#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

// `pqrs::local_datagram::impl::base_impl` can be used safely in a multi-threaded environment.

#include "asio_helper.hpp"
#include "send_entry.hpp"
#include <deque>
#include <filesystem>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace pqrs {
namespace local_datagram {
namespace impl {
class base_impl : public dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const asio::error_code&)> bind_failed;
  nod::signal<void(std::shared_ptr<std::vector<uint8_t>>, std::shared_ptr<asio::local::datagram_protocol::endpoint> sender_endpoint)> received;
  nod::signal<void(void)> closed;
  nod::signal<void(const asio::error_code&)> error_occurred;

  enum class mode {
    server,
    client,
  };

protected:
  base_impl(const base_impl&) = delete;

  base_impl(std::weak_ptr<dispatcher::dispatcher> weak_dispatcher,
            mode mode,
            std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> send_entries) : dispatcher_client(weak_dispatcher),
                                                                                     mode_(mode),
                                                                                     send_entries_(send_entries),
                                                                                     io_service_(),
                                                                                     work_(std::make_unique<asio::io_service::work>(io_service_)),
                                                                                     socket_ready_(false),
                                                                                     send_invoker_(io_service_, asio_helper::time_point::pos_infin()),
                                                                                     send_deadline_(io_service_, asio_helper::time_point::pos_infin()) {
    io_service_thread_ = std::thread([this] {
      this->io_service_.run();
    });
  }

  virtual ~base_impl(void) {
  }

  // We have to terminate asio and pqrs::dispatcher while all instance variables of child class are alive.
  // Thus, `base_impl::terminate` is provided to terminate in the decstructor of child class.
  void terminate_base_impl(void) {
    //
    // asio
    //

    io_service_.post([this] {
      work_ = nullptr;
    });

    if (io_service_thread_.joinable()) {
      io_service_thread_.join();
    }

    //
    // pqrs::dispatcher
    //

    detach_from_dispatcher();
  }

  void set_socket_options(size_t buffer_size) {
    if (!socket_) {
      return;
    }

    //
    // receive options
    //

    // A margin (32 byte) is required to receive data which size == buffer_size.
    size_t buffer_margin = 32;
    receive_buffer_.resize(buffer_size + buffer_margin);
    socket_->set_option(asio::socket_base::receive_buffer_size(receive_buffer_.size()));

    //
    // send options
    //

    // A margin (1 byte) is required to append send_entry::type.
    socket_->set_option(asio::socket_base::send_buffer_size(buffer_size + 1));
  }

  void start_actors(void) {
    //
    // Sender
    //

    await_send_entry(std::nullopt);

    send_deadline_.expires_at(asio_helper::time_point::pos_infin());
    if (mode_ == mode::client) {
      check_send_deadline();
    }

    //
    // Receiver
    //

    async_receive();
  }

public:
  void async_close(void) {
    io_service_.post([this] {
      if (!socket_) {
        return;
      }

      // Close socket

      asio::error_code error_code;

      socket_->cancel(error_code);
      socket_->close(error_code);

      socket_ = nullptr;

      send_invoker_.cancel();
      send_deadline_.cancel();

      // Signal

      if (socket_ready_) {
        socket_ready_ = false;

        if (!bound_path_.empty()) {
          std::error_code error_code;
          std::filesystem::remove(bound_path_, error_code);

          bound_path_.clear();
        }

        enqueue_to_dispatcher([this] {
          closed();
        });
      }
    });
  }

#pragma region server

  // This method is executed in `io_service_thread_`.
  void async_receive(void) {
    if (!socket_ ||
        !socket_ready_) {
      return;
    }

    socket_->async_receive_from(asio::buffer(receive_buffer_),
                                receive_sender_endpoint_,
                                [this](auto&& error_code, auto&& bytes_transferred) {
                                  if (!error_code) {
                                    if (bytes_transferred > 0) {
                                      auto t = send_entry::type(receive_buffer_[0]);
                                      if (t == send_entry::type::user_data) {
                                        auto v = std::make_shared<std::vector<uint8_t>>(bytes_transferred - 1);
                                        std::copy(std::begin(receive_buffer_) + 1,
                                                  std::begin(receive_buffer_) + bytes_transferred,
                                                  std::begin(*v));

                                        auto sender_endpoint = std::make_shared<asio::local::datagram_protocol::endpoint>(receive_sender_endpoint_);

                                        enqueue_to_dispatcher([this, v, sender_endpoint] {
                                          received(v, sender_endpoint);
                                        });
                                      }
                                    }
                                  }

                                  // receive once if not closed

                                  if (socket_ready_) {
                                    async_receive();
                                  }
                                });
  }

#pragma endregion

#pragma region sender

public:
  void async_send(std::shared_ptr<send_entry> entry) {
    if (!entry) {
      return;
    }

    io_service_.post([this, entry] {
      send_entries_->push_back(entry);
      send_invoker_.expires_after(std::chrono::milliseconds(0));
    });
  }

protected:
  //
  // Sender
  //

  // This method is executed in `io_service_thread_`.
  void await_send_entry(std::optional<std::chrono::milliseconds> delay) {
    if (!socket_ ||
        !socket_ready_) {
      return;
    }

    if (delay || send_entries_->empty()) {
      // Sleep until new entry is added.
      if (delay) {
        send_invoker_.expires_after(*delay);
      } else {
        send_invoker_.expires_at(asio_helper::time_point::pos_infin());
      }

      send_invoker_.async_wait(
          [this](const auto& error_code) {
            await_send_entry(std::nullopt);
          });

    } else {
      auto entry = send_entries_->front();
      auto destination_endpoint = entry->get_destination_endpoint();

      send_deadline_.expires_after(std::chrono::milliseconds(5000));

      if (destination_endpoint) {
        socket_->async_send_to(
            entry->make_buffer(),
            *destination_endpoint,
            [this, entry](const auto& error_code, auto bytes_transferred) {
              handle_send(error_code, bytes_transferred, entry);
            });
      } else {
        socket_->async_send(
            entry->make_buffer(),
            [this, entry](const auto& error_code, auto bytes_transferred) {
              handle_send(error_code, bytes_transferred, entry);
            });
      }
    }
  }

  // This method is executed in `io_service_thread_`.
  void handle_send(const asio::error_code& error_code,
                   size_t bytes_transferred,
                   std::shared_ptr<send_entry> entry) {
    std::optional<std::chrono::milliseconds> next_delay;

    entry->add_bytes_transferred(bytes_transferred);

    send_deadline_.expires_at(asio_helper::time_point::pos_infin());

    //
    // Handle error.
    //

    if (error_code == asio::error::no_buffer_space) {
      //
      // Retrying the sending data or abort the buffer is required.
      //
      // - Keep the connection.
      // - Keep or drop the entry.
      //

      // Retry if no_buffer_space error is continued too much times.
      entry->set_no_buffer_space_error_count(
          entry->get_no_buffer_space_error_count() + 1);

      if (entry->get_no_buffer_space_error_count() > 10) {
        // `send` always returns no_buffer_space error on macOS
        // when entry->buffer_.size() > server_buffer_size.
        //
        // Thus, we have to cancel sending data in the such case.
        // (We consider we have to cancel when `send_entry::bytes_transferred` == 0.)

        if (entry->get_bytes_transferred() == 0 ||
            // Abort if too many errors
            entry->get_no_buffer_space_error_count() > 100) {
          // Drop entry

          entry->add_bytes_transferred(entry->rest_bytes());

          enqueue_to_dispatcher([this, error_code] {
            error_occurred(error_code);
          });
        }
      }

      // Wait until buffer is available.
      next_delay = std::chrono::milliseconds(100);

    } else if (error_code == asio::error::message_size) {
      //
      // Problem of the sending data.
      //
      // - Keep the connection.
      // - Drop the entry.
      //

      entry->add_bytes_transferred(entry->rest_bytes());

      enqueue_to_dispatcher([this, error_code] {
        error_occurred(error_code);
      });

    } else if (error_code) {
      //
      // Other errors (e.g., connection error)
      //
      // - Close the connection.
      // - Keep the entry.
      //

      // Ignore error if server mode.
      if (mode_ == mode::server) {
        entry->add_bytes_transferred(entry->rest_bytes());
      } else {
        enqueue_to_dispatcher([this, error_code] {
          error_occurred(error_code);
        });

        async_close();
        return;
      }
    }

    //
    // Remove send_entry if transfer is completed.
    //

    if (entry->transfer_complete()) {
      pop_front_send_entry();
    }

    await_send_entry(next_delay);
  }

  // This method is executed in `io_service_thread_`.
  void pop_front_send_entry(void) {
    if (send_entries_->empty()) {
      return;
    }

    auto entry = send_entries_->front();
    if (auto&& processed = entry->get_processed()) {
      enqueue_to_dispatcher([processed] {
        processed();
      });
    }

    send_entries_->pop_front();
  }

  // This method is executed in `io_service_thread_`.
  void check_send_deadline(void) {
    if (!socket_ ||
        !socket_ready_) {
      return;
    }

    if (send_deadline_.expiry() < asio_helper::time_point::now()) {
      // The deadline has passed.

      async_close();
      return;
    }

    send_deadline_.async_wait(
        [this](const auto& error_code) {
          check_send_deadline();
        });
  }

#pragma endregion

  // External variables
  mode mode_;
  std::shared_ptr<std::deque<std::shared_ptr<send_entry>>> send_entries_;

  // asio
  asio::io_service io_service_;
  std::unique_ptr<asio::io_service::work> work_;
  std::thread io_service_thread_;
  std::unique_ptr<asio::local::datagram_protocol::socket> socket_;
  bool socket_ready_;

  // Server
  std::filesystem::path bound_path_;
  std::vector<uint8_t> receive_buffer_;
  asio::local::datagram_protocol::endpoint receive_sender_endpoint_;

  // Sender
  asio::steady_timer send_invoker_;
  asio::steady_timer send_deadline_;
};
} // namespace impl
} // namespace local_datagram
} // namespace pqrs
