#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "file_monitor.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
std::string file_path_1_1 = "target/sub1/file1_1";
std::string file_path_1_2 = "target/sub1/file1_2";
std::string file_path_2_1 = "../file_monitor/target//sub2/file2_1";

class test_file_monitor final {
public:
  test_file_monitor(void) : register_stream_finished_(false),
                            count_(0) {
    std::vector<std::string> targets({
        file_path_1_1,
        file_path_1_2,
        file_path_2_1,
    });

    file_monitor_ = std::make_unique<krbn::file_monitor>(targets);

    file_monitor_->register_stream_finished.connect([&] {
      if (!file_monitor_thread_id_) {
        file_monitor_thread_id_ = std::this_thread::get_id();
      }
      if (file_monitor_thread_id_ != std::this_thread::get_id()) {
        throw std::logic_error("thread id mismatch");
      }

      register_stream_finished_ = true;
    });

    file_monitor_->file_changed.connect([&](auto&& file_path, auto&& file_body) {
      if (!file_monitor_thread_id_) {
        file_monitor_thread_id_ = std::this_thread::get_id();
      }
      if (file_monitor_thread_id_ != std::this_thread::get_id()) {
        throw std::logic_error("thread id mismatch");
      }

      ++count_;
      last_file_path_ = file_path;

      if (file_path == file_path_1_1) {
        if (file_body) {
          last_file_body1_1_ = std::string(std::begin(*file_body),
                                           std::end(*file_body));
        } else {
          last_file_body1_1_ = boost::none;
        }
      }
      if (file_path == file_path_1_2) {
        if (file_body) {
          last_file_body1_2_ = std::string(std::begin(*file_body),
                                           std::end(*file_body));
        } else {
          last_file_body1_2_ = boost::none;
        }
      }
      if (file_path == file_path_2_1) {
        if (file_body) {
          last_file_body2_1_ = std::string(std::begin(*file_body),
                                           std::end(*file_body));
        } else {
          last_file_body2_1_ = boost::none;
        }
      }
    });

    file_monitor_->async_start();

    wait();
  }

  bool get_register_stream_finished(void) const {
    return register_stream_finished_;
  }

  size_t get_count(void) const {
    return count_;
  }

  const boost::optional<std::string>& get_last_file_path(void) const {
    return last_file_path_;
  }

  const boost::optional<std::string>& get_last_file_body1_1(void) const {
    return last_file_body1_1_;
  }

  const boost::optional<std::string>& get_last_file_body1_2(void) const {
    return last_file_body1_2_;
  }

  const boost::optional<std::string>& get_last_file_body2_1(void) const {
    return last_file_body2_1_;
  }

  void clear_results(void) {
    count_ = 0;
    last_file_path_ = boost::none;
    last_file_body1_1_ = boost::none;
    last_file_body1_2_ = boost::none;
    last_file_body2_1_ = boost::none;
  }

  void wait(void) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  void enqueue_file_changed(const std::string& file_path) {
    file_monitor_->enqueue_file_changed(file_path);
  }

private:
  std::unique_ptr<krbn::file_monitor> file_monitor_;
  boost::optional<std::thread::id> file_monitor_thread_id_;
  bool register_stream_finished_;
  size_t count_;
  boost::optional<std::string> last_file_path_;
  boost::optional<std::string> last_file_body1_1_;
  boost::optional<std::string> last_file_body1_2_;
  boost::optional<std::string> last_file_body2_1_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
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

    REQUIRE(monitor.get_register_stream_finished());
    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_0"s);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_0"s);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file1_1)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_1 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_1"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_2 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_2"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file1_2)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_2_1 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_1"s);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file1_2 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_2_2 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == "1_2_2"s);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_3 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_3"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification (update file2_1)
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 2_1_1 > target/sub2/file2_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == "2_1_1"s);

    // ========================================
    // File removal
    // ========================================

    monitor.clear_results();

    system("rm target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_2);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // File removal
    // ========================================

    monitor.clear_results();

    system("rm target/sub2/file2_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Directory removal
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification
    // ========================================

    monitor.clear_results();

    system("mkdir -p target/sub1");

    monitor.wait();

    system("/bin/echo -n 1_1_4 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_4"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Move file
    // ========================================

    monitor.clear_results();

    system("/bin/echo -n 1_1_5 > target/sub1/file1_1.new");
    system("mv target/sub1/file1_1.new target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_5"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Move directory
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);

    system("mkdir -p target.new/sub1");
    system("/bin/echo -n 1_1_6 > target.new/sub1/file1_1");
    system("mv target.new target");

    monitor.wait();

    REQUIRE(monitor.get_count() == 2);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_6"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Ignore own process
    // ========================================

    monitor.clear_results();

    {
      std::ofstream(file_path_1_1) << "1_1_7";
    }

    monitor.wait();

    REQUIRE(monitor.get_count() == 0);
    REQUIRE(monitor.get_last_file_path() == boost::none);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // enqueue_file_changed
    // ========================================

    monitor.clear_results();

    monitor.enqueue_file_changed(file_path_1_1);

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_7"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);
  }

  {
    // ========================================
    // Create test_file_monitor when any target files do not exist.
    // ========================================

    system("rm -rf target");

    test_file_monitor monitor;

    REQUIRE(monitor.get_register_stream_finished());
    REQUIRE(monitor.get_count() == 3);
    REQUIRE(monitor.get_last_file_path() == file_path_2_1);
    REQUIRE(monitor.get_last_file_body1_1() == boost::none);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);

    // ========================================
    // Generic file modification
    // ========================================

    monitor.clear_results();

    system("mkdir -p target/sub1");
    system("/bin/echo -n 1_1_0 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_count() == 1);
    REQUIRE(monitor.get_last_file_path() == file_path_1_1);
    REQUIRE(monitor.get_last_file_body1_1() == "1_1_0"s);
    REQUIRE(monitor.get_last_file_body1_2() == boost::none);
    REQUIRE(monitor.get_last_file_body2_1() == boost::none);
  }
}
