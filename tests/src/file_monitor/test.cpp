#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "file_monitor.hpp"
#include "thread_utility.hpp"

namespace {
class test_file_monitor final {
public:
  test_file_monitor(void) : register_stream_finished_(false) {
    std::vector<std::string> targets({
        "target/sub1/file1_1",
        "target/sub1/file1_2",
        "target/sub2/file2_1",
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

    file_monitor_->file_changed.connect([&](auto& file_path) {
      if (!file_monitor_thread_id_) {
        file_monitor_thread_id_ = std::this_thread::get_id();
      }
      if (file_monitor_thread_id_ != std::this_thread::get_id()) {
        throw std::logic_error("thread id mismatch");
      }

      last_file_path_ = file_path;

      std::ifstream ifstream(file_path);
      REQUIRE(ifstream);

      if (auto realpath = krbn::filesystem::realpath("target/sub1/file1_1")) {
        if (file_path == *realpath) {
          std::getline(ifstream, last_file_line1_1_);
        }
      }
      if (auto realpath = krbn::filesystem::realpath("target/sub1/file1_2")) {
        if (file_path == *realpath) {
          std::getline(ifstream, last_file_line1_2_);
        }
      }
      if (auto realpath = krbn::filesystem::realpath("target/sub2/file2_1")) {
        if (file_path == *realpath) {
          std::getline(ifstream, last_file_line2_1_);
        }
      }
    });

    file_monitor_->start();

    wait();
  }

  bool get_register_stream_finished(void) const {
    return register_stream_finished_;
  }

  const std::string& get_last_file_path(void) const {
    return last_file_path_;
  }

  const std::string& get_last_file_line1_1(void) const {
    return last_file_line1_1_;
  }

  const std::string& get_last_file_line1_2(void) const {
    return last_file_line1_2_;
  }

  const std::string& get_last_file_line2_1(void) const {
    return last_file_line2_1_;
  }

  void clear_results(void) {
    last_file_path_.clear();
    last_file_line1_1_.clear();
    last_file_line1_2_.clear();
    last_file_line2_1_.clear();
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
  std::string last_file_path_;
  std::string last_file_line1_1_;
  std::string last_file_line1_2_;
  std::string last_file_line2_1_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("file_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target/sub1");
    system("mkdir -p target/sub2");
    system("echo 1_1_0 > target/sub1/file1_1");
    system("echo 1_2_0 > target/sub1/file1_2");

    test_file_monitor monitor;

    REQUIRE(monitor.get_register_stream_finished());
    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_2"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_0");
    REQUIRE(monitor.get_last_file_line1_2() == "1_2_0");
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file1_1)
    // ========================================

    monitor.clear_results();

    system("echo 1_1_1 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_1");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("echo 1_1_2 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_2");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file1_2)
    // ========================================

    monitor.clear_results();

    system("echo 1_2_1 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_2"));
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2() == "1_2_1");
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file1_2 again)
    // ========================================

    monitor.clear_results();

    system("echo 1_2_2 > target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_2"));
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2() == "1_2_2");
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file1_1 again)
    // ========================================

    monitor.clear_results();

    system("echo 1_1_3 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_3");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification (update file2_1)
    // ========================================

    monitor.clear_results();

    system("echo 2_1_1 > target/sub2/file2_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub2/file2_1"));
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1() == "2_1_1");

    // ========================================
    // File removal
    // ========================================

    monitor.clear_results();

    system("rm target/sub1/file1_2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path().empty());
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Directory removal
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path().empty());
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification
    // ========================================

    monitor.clear_results();

    system("mkdir -p target/sub1");

    monitor.wait();

    system("echo 1_1_4 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_4");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Move file
    // ========================================

    monitor.clear_results();

    system("echo 1_1_5 > target/sub1/file1_1.new");
    system("mv target/sub1/file1_1.new target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_5");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Move directory
    // ========================================

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    system("mkdir -p target.new/sub1");
    system("echo 1_1_6 > target.new/sub1/file1_1");
    system("mv target.new target");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_6");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // enqueue_file_changed
    // ========================================

    monitor.clear_results();

    monitor.enqueue_file_changed("target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_6");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());
  }

  {
    // ========================================
    // Create test_file_monitor when any target files do not exist.
    // ========================================

    system("rm -rf target");

    test_file_monitor monitor;

    REQUIRE(monitor.get_register_stream_finished());
    REQUIRE(monitor.get_last_file_path().empty());
    REQUIRE(monitor.get_last_file_line1_1().empty());
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());

    // ========================================
    // Generic file modification
    // ========================================

    system("mkdir -p target/sub1");
    system("echo 1_1_0 > target/sub1/file1_1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub1/file1_1"));
    REQUIRE(monitor.get_last_file_line1_1() == "1_1_0");
    REQUIRE(monitor.get_last_file_line1_2().empty());
    REQUIRE(monitor.get_last_file_line2_1().empty());
  }
}
