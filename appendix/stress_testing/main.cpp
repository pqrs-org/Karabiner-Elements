#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main(void) {
  std::atomic_flag lock = ATOMIC_FLAG_INIT;

  lock.test_and_set(std::memory_order_acquire);

  // Run threads

  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back([i, &lock] {
      // spinlock
      while (lock.test_and_set(std::memory_order_acquire)) {
      }
      std::cout << "finished (" << i << ")" << std::endl;
      lock.clear(std::memory_order_release);
    });
  }

  // Sleep

  {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
  }

  lock.clear(std::memory_order_release);

  // Stop threads

  for (auto&& t : threads) {
    t.join();
  }

  return 0;
}
