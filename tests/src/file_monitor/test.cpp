#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "monitor/file_monitor.hpp"

namespace {
std::string file_path_1_1 = "target/sub1/file1_1";
std::string file_path_1_2 = "target/sub1/file1_2";
std::string file_path_2_1 = "../file_monitor/target//sub2/file2_1";

class test_file_monitor final {
public:
  test_file_monitor(void) : count_(0) {
    std::vector<std::string> targets({
        file_path_1_1,
        file_path_1_2,
        file_path_2_1,
    });

    file_monitor_ = std::make_unique<krbn::file_monitor>(targets);

    file_monitor_->file_changed.connect([&](auto&& changed_file_path,
                                            auto&& changed_file_body) {
      if (!file_monitor_thread_id_) {
        file_monitor_thread_id_ = std::this_thread::get_id();
      }
      if (file_monitor_thread_id_ != std::this_thread::get_id()) {
        throw std::logic_error("thread id mismatch");
      }

      ++count_;
      last_file_path_ = changed_file_path;

      if (changed_file_path == file_path_1_1) {
        if (changed_file_body) {
          last_file_body1_1_ = std::string(std::begin(*changed_file_body),
                                           std::end(*changed_file_body));
        } else {
          last_file_body1_1_ = std::nullopt;
        }
      }
      if (changed_file_path == file_path_1_2) {
        if (changed_file_body) {
          last_file_body1_2_ = std::string(std::begin(*changed_file_body),
                                           std::end(*changed_file_body));
        } else {
          last_file_body1_2_ = std::nullopt;
        }
      }
      if (changed_file_path == file_path_2_1) {
        if (changed_file_body) {
          last_file_body2_1_ = std::string(std::begin(*changed_file_body),
                                           std::end(*changed_file_body));
        } else {
          last_file_body2_1_ = std::nullopt;
        }
      }
    });

    file_monitor_->async_start();

    wait();
  }

  size_t get_count(void) const {
    return count_;
  }

  const std::optional<std::string>& get_last_file_path(void) const {
    return last_file_path_;
  }

  const std::optional<std::string>& get_last_file_body1_1(void) const {
    return last_file_body1_1_;
  }

  const std::optional<std::string>& get_last_file_body1_2(void) const {
    return last_file_body1_2_;
  }

  const std::optional<std::string>& get_last_file_body2_1(void) const {
    return last_file_body2_1_;
  }

  void clear_results(void) {
    count_ = 0;
    last_file_path_ = std::nullopt;
    last_file_body1_1_ = std::nullopt;
    last_file_body1_2_ = std::nullopt;
    last_file_body2_1_ = std::nullopt;
  }

  void wait(void) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  void enqueue_file_changed(const std::string& file_path) {
    file_monitor_->enqueue_file_changed(file_path);
  }

private:
  std::unique_ptr<krbn::file_monitor> file_monitor_;
  std::optional<std::thread::id> file_monitor_thread_id_;
  size_t count_;
  std::optional<std::string> last_file_path_;
  std::optional<std::string> last_file_body1_1_;
  std::optional<std::string> last_file_body1_2_;
  std::optional<std::string> last_file_body2_1_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("file_monitor") {
  using namespace std::string_literals;

  {
    system("rm -rf target");
    system("mkdir -p target/sub1");
    system("mkdir -p target/sub2");
    system("/bin/echo -n 1_1_0 > target/sub1/file1_1");
    system("/bin/echo -n 1_2_0 > target/sub1/file1_2");

    test_file_monitor monitor;

    REQUIRE(monitor.get_count() >= 3);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_0"s);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_0"s);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file1_1)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_1 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_1"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_2 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_2"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file1_2)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_2_1 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_1"s);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file1_2 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_2_2 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_2"s);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_3 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_3"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification (update file2_1)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 2_1_1 > target/sub2/file2_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == "2_1_1"s);

    // ========================================
    // File removal
    // ========================================

    monitor.clear_results();

    system("rm target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // File removal
    // ========================================

    monitor.clear_results();

    system("rm target/sub2/file2_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Directory removal
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification
    // ========================================

    monitor.clear_results();

    system("mkdir -p target/sub1");

    monitor.wait();

    system("/bin/echo -n 1_1_4 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_4"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Move file
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_5 > target/sub1/file1_1.new");
    system("mv target/sub1/file1_1.new target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_5"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Move directory
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);

    system("mkdir -p target.new/sub1");
    system("/bin/echo -n 1_1_6 > target.new/sub1/file1_1");
    system("mv target.new target");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 2);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_6"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Ignore own process
    // ========================================

    monitor.clear_results();

    {
      std::ofstream(file_path_1_1) << "1_1_7";
    }

    monitor.wait();

    REQUIRE(monitor.get_count() == 0);
    REQUIRE(monitor.get_last_file_path() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // enqueue_file_changed
    // ========================================

    monitor.clear_results();

    monitor.enqueue_file_changed(file_path_1_1);

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_7"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);
  }

  {
    // ========================================
    // Create test_file_monitor when any target files do not exist.
    // ========================================

    system("rm -rf target");

    test_file_monitor monitor;

    REQUIRE(monitor.get_count() >= 3);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == std::nullopt);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);

    // ========================================
    // Generic file modification
    // ========================================

    monitor.clear_results();

    system("mkdir -p target/sub1");
    system("/bin/echo -n 1_1_0 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() >= 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_0"s);
    REQUIRE(monitor.get_last_file_body1_2() == std::nullopt);
    REQUIRE(monitor.get_last_file_body2_1() == std::nullopt);
  }
}

TEST_CASE("read_file") {
  {
    auto buffer = krbn::file_monitor::read_file("data/not_found");

    REQUIRE(!buffer);
  }

  {
    auto buffer = krbn::file_monitor::read_file("data/app.icns");

    REQUIRE(buffer);
    REQUIRE(buffer->size() == 225321);
    REQUIRE((*buffer)[0] == 0x69);
    REQUIRE((*buffer)[1] == 0x63);
    REQUIRE((*buffer)[2] == 0x6e);
    REQUIRE((*buffer)[3] == 0x73);
    REQUIRE((*buffer)[4] == 0x00);
    REQUIRE((*buffer)[5] == 0x03);
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
