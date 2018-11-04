#pragma once

// `krbn::dispatcher_utility` can be used safely in a multi-threaded environment.

#include <pqrs/dispatcher.hpp>

namespace krbn {
class dispatcher_utility final {
public:
  static void initialize_dispatchers(void) {
    pqrs::dispatcher::extra::initialize_shared_dispatcher();

    {
      std::lock_guard<std::mutex> lock(get_file_writer_mutex());

      if (!get_file_writer()) {
        get_file_writer() = std::make_shared<file_writer>();
      }
    }
  }

  static void terminate_dispatchers(void) {
    pqrs::dispatcher::extra::terminate_shared_dispatcher();

    {
      std::lock_guard<std::mutex> lock(get_file_writer_mutex());

      get_file_writer() = nullptr;
    }
  }

  static void enqueue_to_file_writer_dispatcher(const std::function<void(void)>& function) {
    std::lock_guard<std::mutex> lock(get_file_writer_mutex());

    if (get_file_writer()) {
      get_file_writer()->enqueue(function);
    }
  }

private:
  class file_writer final {
  public:
    file_writer(void) : time_source_(std::make_shared<pqrs::dispatcher::hardware_time_source>()),
                        dispatcher_(std::make_shared<pqrs::dispatcher::dispatcher>(time_source_)),
                        object_id_(pqrs::dispatcher::make_new_object_id()) {
      dispatcher_->attach(object_id_);
    }

    ~file_writer(void) {
      dispatcher_->detach(object_id_);
    }

    void enqueue(const std::function<void(void)>& function) {
      dispatcher_->enqueue(object_id_,
                           function);
    }

  private:
    std::shared_ptr<pqrs::dispatcher::hardware_time_source> time_source_;
    std::shared_ptr<pqrs::dispatcher::dispatcher> dispatcher_;
    pqrs::dispatcher::object_id object_id_;
  };

  static std::mutex& get_file_writer_mutex(void) {
    static std::mutex mutex;
    return mutex;
  }

  static std::shared_ptr<file_writer>& get_file_writer(void) {
    static std::shared_ptr<file_writer> p;
    return p;
  }
};
} // namespace krbn
