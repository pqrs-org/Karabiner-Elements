#pragma once

#include "grabber_client.hpp"
#include "libkrbn/libkrbn.h"

class libkrbn_grabber_client final {
public:
  libkrbn_grabber_client(const libkrbn_grabber_client&) = delete;

  libkrbn_grabber_client(void) {
    krbn::logger::get_logger()->info(__func__);

    grabber_client_ = std::make_unique<krbn::grabber_client>();

    grabber_client_->async_start();
  }

  ~libkrbn_grabber_client(void) {
    krbn::logger::get_logger()->info(__func__);
  }

  void async_set_variable(const std::string& name, int value) {
    auto json = nlohmann::json::object({
        {name, value},
    });
    grabber_client_->async_set_variables(json);
  }

private:
  std::unique_ptr<krbn::grabber_client> grabber_client_;
};
