#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "filesystem.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>
#include <ostream>

TEST_CASE("create_directory_with_intermediate_directories") {
  REQUIRE(filesystem::create_directory_with_intermediate_directories("/", 0700) == true);
  REQUIRE(filesystem::create_directory_with_intermediate_directories(".", 0700) == true);
  REQUIRE(filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0700) == true);
}

TEST_CASE("is_directory") {
  REQUIRE(filesystem::is_directory("/") == true);
  REQUIRE(filesystem::is_directory(".") == true);
  REQUIRE(filesystem::is_directory("..") == true);
  REQUIRE(filesystem::is_directory("/bin/ls") == false);
  REQUIRE(filesystem::is_directory("/not_found") == false);
}

TEST_CASE("dirname") {
  REQUIRE(filesystem::dirname("/usr/bin/ls") == "/usr/bin");
  REQUIRE(filesystem::dirname("/usr/bin/ls/") == "/usr/bin");
  REQUIRE(filesystem::dirname("/usr/bin/l") == "/usr/bin");
  REQUIRE(filesystem::dirname("/usr/bin/l/") == "/usr/bin");
  REQUIRE(filesystem::dirname("/usr") == "/");
  REQUIRE(filesystem::dirname("/") == "/");
  REQUIRE(filesystem::dirname("usr/bin/ls") == "usr/bin");
  REQUIRE(filesystem::dirname("usr/bin/") == "usr");
  REQUIRE(filesystem::dirname("usr") == ".");
  REQUIRE(filesystem::dirname("usr/") == ".");
  REQUIRE(filesystem::dirname("") == ".");
}

TEST_CASE("normalize_file_path") {
  std::string file_path;

  file_path = "";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = ".";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "./";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "..";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "..");

  file_path = "../";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../");

  file_path = "..//foo";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "abcde";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde");

  file_path = "abcde/";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde/");

  file_path = "/foo//bar/../baz";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "/../foo//bar/../baz";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "foo/../bar";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "bar");

  file_path = "foo/.../bar";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/.../bar");

  file_path = "a/../b/../c/d";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "c/d");

  file_path = "a/./b/./c/d";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "a/b/c/d");

  file_path = "foo/bar/..";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo");

  file_path = "foo/bar/../";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/");

  file_path = "foo/bar/.";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "foo/bar/./";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar/");

  file_path = "../foo/bar";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar");

  file_path = "../../../foo/bar";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../../foo/bar");

  file_path = "./foo/bar";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "../foo/bar/..";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "../foo/bar///...";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar/...");

  file_path = "../a/b/../c/../d///..";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../a");

  file_path = "a/../..////../b/c";
  filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../b/c");
}

TEST_CASE("realpath") {
  auto actual = filesystem::realpath("/bin/ls");
  REQUIRE(*actual == "/bin/ls");

  actual = filesystem::realpath("/var/log/not_found");
  REQUIRE(actual == boost::none);

  actual = filesystem::realpath("/var/log/system.log");
  REQUIRE(*actual == "/private/var/log/system.log");
}

int main(int argc, char* const argv[]) {
  thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
