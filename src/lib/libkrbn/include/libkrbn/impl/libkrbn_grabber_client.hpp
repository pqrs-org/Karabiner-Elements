#pragma once

#include "grabber_client.hpp"
#include "libkrbn/libkrbn.h"

class libkrbn_grabber_client final {
public:
  libkrbn_grabber_client(const libkrbn_grabber_client&) = delete;

  libkrbn_grabber_client(libkrbn_grabber_client_connected_callback connected_callback,
                         libkrbn_grabber_client_connect_failed_callback connect_failed_callback,
                         libkrbn_grabber_client_closed_callback closed_callback) : connected_callback_(connected_callback),
                                                                                   connect_failed_callback_(connect_failed_callback),
                                                                                   closed_callback_(closed_callback) {
    krbn::logger::get_logger()->info(__func__);

    grabber_client_ = std::make_unique<krbn::grabber_client>();

    grabber_client_->connected.connect([this] {
      if (connected_callback_) {
        connected_callback_();
      }
    });

    grabber_client_->connect_failed.connect([this](auto&& error_code) {
      if (connect_failed_callback_) {
        connect_failed_callback_();
      }
    });

    grabber_client_->closed.connect([this] {
      if (closed_callback_) {
        closed_callback_();
      }
    });

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

  void sync_set_variable(const std::string& name, int value) {
    auto wait = pqrs::make_thread_wait();

    auto json = nlohmann::json::object({
        {name, value},
    });
    grabber_client_->async_set_variables(json, [wait] {
      wait->notify();
    });

    wait->wait_notice();
  }

private:
  std::unique_ptr<krbn::grabber_client> grabber_client_;
  libkrbn_grabber_client_connected_callback connected_callback_;
  libkrbn_grabber_client_connect_failed_callback connect_failed_callback_;
  libkrbn_grabber_client_closed_callback closed_callback_;
};
