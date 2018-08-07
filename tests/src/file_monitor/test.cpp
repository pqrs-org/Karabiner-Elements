#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "file_monitor.hpp"
#include "thread_utility.hpp"

namespace {
class test_file_monitor final {
public:
  test_file_monitor(void) {
    std::vector<std::pair<std::string, std::vector<std::string>>> targets({
        {
            "target/sub",
            {
                "target/sub/file1",
                "target/sub/file2",
            },
        },
    });

    file_monitor_ = std::make_unique<krbn::file_monitor>(targets);

    file_monitor_->file_changed.connect([&](auto& file_path) {
      last_file_path_ = file_path;

      std::ifstream ifstream(file_path);
      REQUIRE(ifstream);

      if (auto realpath = krbn::filesystem::realpath("target/sub/file1")) {
        if (file_path == *realpath) {
          std::getline(ifstream, last_file_line1_);
        }
      }
      if (auto realpath = krbn::filesystem::realpath("target/sub/file2")) {
        if (file_path == *realpath) {
          std::getline(ifstream, last_file_line2_);
        }
      }
    });

    file_monitor_->start();

    wait();
  }

  const std::string& get_last_file_path(void) const {
    return last_file_path_;
  }

  const std::string& get_last_file_line1(void) const {
    return last_file_line1_;
  }

  const std::string& get_last_file_line2(void) const {
    return last_file_line2_;
  }

  void clear_results(void) {
    last_file_path_.clear();
    last_file_line1_.clear();
    last_file_line2_.clear();
  }

  void wait(void) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

private:
  std::unique_ptr<krbn::file_monitor> file_monitor_;
  std::string last_file_path_;
  std::string last_file_line1_;
  std::string last_file_line2_;
};
} // namespace

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("file_monitor") {
  system("rm -rf target");
  system("mkdir -p target/sub");
  system("echo 10 > target/sub/file1");
  system("echo 20 > target/sub/file2");

  {
    test_file_monitor monitor;

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file2"));
    REQUIRE(monitor.get_last_file_line1() == "10");
    REQUIRE(monitor.get_last_file_line2() == "20");

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("echo 11 > target/sub/file1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "11");
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("echo 12 > target/sub/file1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "12");
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("echo 21 > target/sub/file2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file2"));
    REQUIRE(monitor.get_last_file_line1().empty());
    REQUIRE(monitor.get_last_file_line2() == "21");

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("echo 22 > target/sub/file2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file2"));
    REQUIRE(monitor.get_last_file_line1().empty());
    REQUIRE(monitor.get_last_file_line2() == "22");

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("echo 13 > target/sub/file1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "13");
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // File removal

    monitor.clear_results();

    system("rm target/sub/file2");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path().empty());
    REQUIRE(monitor.get_last_file_line1().empty());
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Directory removal

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path().empty());
    REQUIRE(monitor.get_last_file_line1().empty());
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Generic file modification

    monitor.clear_results();

    system("mkdir -p target/sub");

    monitor.wait();

    system("echo 14 > target/sub/file1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "14");
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Move file

    monitor.clear_results();

    system("echo 15 > target/sub/file1.new");
    system("mv target/sub/file1.new target/sub/file1");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "15");
    REQUIRE(monitor.get_last_file_line2().empty());

    // ========================================
    // Move directory

    monitor.clear_results();

    system("rm -rf target");

    monitor.wait();

    system("mkdir -p target.new/sub");
    system("echo 16 > target.new/sub/file1");
    system("mv target.new target");

    monitor.wait();

    REQUIRE(monitor.get_last_file_path() == *krbn::filesystem::realpath("target/sub/file1"));
    REQUIRE(monitor.get_last_file_line1() == "16");
    REQUIRE(monitor.get_last_file_line2().empty());
  }
}
