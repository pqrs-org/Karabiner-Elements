#pragma once

#include <pqrs/dispatcher.hpp>

class settings_dispatcher_client final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  settings_dispatcher_client(const settings_dispatcher_client&) = delete;

  settings_dispatcher_client()
      : dispatcher_client() {
  }

  ~settings_dispatcher_client() override {
    detach_from_dispatcher();
  }

  void enqueue(void (*callback)()) {
    enqueue_to_dispatcher([callback] {
      callback();
    });
  }
};
