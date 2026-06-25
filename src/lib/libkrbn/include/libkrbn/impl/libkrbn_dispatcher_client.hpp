#pragma once

#include <pqrs/dispatcher.hpp>

class libkrbn_dispatcher_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_dispatcher_client(const libkrbn_dispatcher_client&) = delete;

  libkrbn_dispatcher_client()
      : dispatcher_client() {
  }

  ~libkrbn_dispatcher_client() {
    detach_from_dispatcher();
  }

  void enqueue(void (*callback)()) {
    enqueue_to_dispatcher([callback] {
      callback();
    });
  }
};
