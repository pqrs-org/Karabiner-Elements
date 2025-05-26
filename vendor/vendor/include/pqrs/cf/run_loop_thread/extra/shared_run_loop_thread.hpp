#pragma once

// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

// `pqrs::cf::run_loop_thread::extra::shared_run_loop_thread` can be used safely in a multi-threaded environment.

// namespace pqrs {
// namespace cf {
// class run_loop_thread {
// class extra {

class shared_run_loop_thread final {
public:
  void initialize(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!run_loop_thread_) {
      run_loop_thread_ = std::make_shared<run_loop_thread>();
    }
  }

  void terminate(void) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (run_loop_thread_) {
      run_loop_thread_->terminate();
      run_loop_thread_ = nullptr;
    }
  }

  std::shared_ptr<run_loop_thread> get_run_loop_thread(void) const {
    return run_loop_thread_;
  }

  static std::shared_ptr<shared_run_loop_thread> get_shared_run_loop_thread(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    static std::shared_ptr<shared_run_loop_thread> p;
    if (!p) {
      p = std::make_shared<shared_run_loop_thread>();
    }

    return p;
  }

private:
  std::shared_ptr<run_loop_thread> run_loop_thread_;
  mutable std::mutex mutex_;
};

static void initialize_shared_run_loop_thread(void) {
  auto p = shared_run_loop_thread::get_shared_run_loop_thread();
  p->initialize();
}

static void terminate_shared_run_loop_thread(void) {
  auto p = shared_run_loop_thread::get_shared_run_loop_thread();
  p->terminate();
}

static std::shared_ptr<run_loop_thread> get_shared_run_loop_thread(void) {
  auto p = shared_run_loop_thread::get_shared_run_loop_thread();
  return p->get_run_loop_thread();
}
