// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

class file_monitors_manager final {
public:
  static void insert(file_monitor* p) {
    std::lock_guard<std::mutex> lock(mutex());

    set().insert(p);
  }

  static void erase(file_monitor* p) {
    std::lock_guard<std::mutex> lock(mutex());

    set().erase(p);
  }

  static bool alive(file_monitor* p) {
    std::lock_guard<std::mutex> lock(mutex());

    return set().contains(p);
  }

private:
  static std::mutex& mutex() {
    static std::mutex mutex;
    return mutex;
  }

  static std::unordered_set<file_monitor*>& set() {
    static std::unordered_set<file_monitor*> set;
    return set;
  }
};
