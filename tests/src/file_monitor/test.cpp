#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "file_monitor.hpp"
#include "thread_utility.hpp"
#include <ostream>
#include <time.h>

namespace {
class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_color_mt("file_monitor");
      logger->set_level(spdlog::level::off);
    }

    return *logger;
  }
};

std::atomic<time_t> last_callback_time;
} // namespace

TEST_CASE("file_monitor") {
  std::vector<std::pair<std::string, std::vector<std::string>>> targets({
      {
          "target/sub", {
                            "target/sub/file1", "target/sub/file2",
                        },
      },
  });
  krbn::file_monitor file_monitor(logger::get_logger(), targets, [](const std::string& file_path) {
    static int i = 0;

    last_callback_time = time(nullptr);

    std::cerr << "." << std::flush;

    std::ifstream ifstream(file_path);
    REQUIRE(ifstream);

    std::string line;
    std::getline(ifstream, line);

    switch (i) {
      case 0:
        REQUIRE(line == "11");
        break;
      case 1:
        REQUIRE(line == "12");
        break;
      case 2:
        REQUIRE(line == "21");
        break;
      case 3:
        REQUIRE(line == "22");
        break;
      case 4:
        REQUIRE(line == "13");
        break;
      // Note: `rm target/sub/file2` does not call this callback.
      case 5:
        REQUIRE(line == "14");
        break;
      case 6:
        REQUIRE(line == "15");
        break;
      case 7:
        REQUIRE(line == "16");
        break;
    }

    if (line == "end") {
      REQUIRE(i == 8);
      std::cerr << std::endl;
      exit(0);
    }

    ++i;
  });

  CFRunLoopRun();
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();

  last_callback_time = time(nullptr);

  // watchdog timer
  krbn::gcd_utility::main_queue_timer timer(
      dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC),
      1.0 * NSEC_PER_SEC,
      0,
      ^{
        time_t now = time(nullptr);

        if (now - last_callback_time > 10) {
          exit(1);
        }
      });

  return Catch::Session().run(argc, argv);
}
