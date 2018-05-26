#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main(void) {
  std::atomic<bool> exit_flag(false);

  // Run threads

  std::vector<std::thread> threads;
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&] {
      int counter = 0;
      while (!exit_flag) {
        std::cout << ++counter << std::endl;
      }
    });
  }

  // Sleep

  {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
  }

  // Stop threads

  exit_flag = true;
  for (auto&& t : threads) {
    t.join();
  }

  return 0;
}
