#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "filesystem.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>
#include <ostream>

TEST_CASE("exists") {
  REQUIRE(krbn::filesystem::exists("data/file") == true);
  REQUIRE(krbn::filesystem::exists("data/not_found") == false);
  REQUIRE(krbn::filesystem::exists("data/symlink") == true);
  REQUIRE(krbn::filesystem::exists("data/not_found_symlink") == false);
}

TEST_CASE("create_directory_with_intermediate_directories") {
  REQUIRE(krbn::filesystem::create_directory_with_intermediate_directories("/", 0700) == true);
  REQUIRE(krbn::filesystem::create_directory_with_intermediate_directories(".", 0700) == true);
  REQUIRE(krbn::filesystem::create_directory_with_intermediate_directories("mkdir_example/a/b/c/d/e", 0700) == true);
}

TEST_CASE("is_directory") {
  REQUIRE(krbn::filesystem::is_directory("/") == true);
  REQUIRE(krbn::filesystem::is_directory(".") == true);
  REQUIRE(krbn::filesystem::is_directory("..") == true);
  REQUIRE(krbn::filesystem::is_directory("/bin/ls") == false);
  REQUIRE(krbn::filesystem::is_directory("/not_found") == false);
}

TEST_CASE("dirname") {
  REQUIRE(krbn::filesystem::dirname("/usr/bin/ls") == "/usr/bin");
  REQUIRE(krbn::filesystem::dirname("/usr/bin/ls/") == "/usr/bin");
  REQUIRE(krbn::filesystem::dirname("/usr/bin/l") == "/usr/bin");
  REQUIRE(krbn::filesystem::dirname("/usr/bin/l/") == "/usr/bin");
  REQUIRE(krbn::filesystem::dirname("/usr") == "/");
  REQUIRE(krbn::filesystem::dirname("/") == "/");
  REQUIRE(krbn::filesystem::dirname("usr/bin/ls") == "usr/bin");
  REQUIRE(krbn::filesystem::dirname("usr/bin/") == "usr");
  REQUIRE(krbn::filesystem::dirname("usr") == ".");
  REQUIRE(krbn::filesystem::dirname("usr/") == ".");
  REQUIRE(krbn::filesystem::dirname("") == ".");
}

TEST_CASE("normalize_file_path") {
  std::string file_path;

  file_path = "";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = ".";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "./";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == ".");

  file_path = "..";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "..");

  file_path = "../";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../");

  file_path = "..//foo";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "abcde";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde");

  file_path = "abcde/";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "abcde/");

  file_path = "/foo//bar/../baz";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "/../foo//bar/../baz";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "/foo/baz");

  file_path = "foo/../bar";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "bar");

  file_path = "foo/.../bar";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/.../bar");

  file_path = "a/../b/../c/d";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "c/d");

  file_path = "a/./b/./c/d";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "a/b/c/d");

  file_path = "foo/bar/..";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo");

  file_path = "foo/bar/../";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/");

  file_path = "foo/bar/.";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "foo/bar/./";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar/");

  file_path = "../foo/bar";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar");

  file_path = "../../../foo/bar";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../../foo/bar");

  file_path = "./foo/bar";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "foo/bar");

  file_path = "../foo/bar/..";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo");

  file_path = "../foo/bar///...";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../foo/bar/...");

  file_path = "../a/b/../c/../d///..";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../a");

  file_path = "a/../..////../b/c";
  krbn::filesystem::normalize_file_path(file_path);
  REQUIRE(file_path == "../../b/c");
}

TEST_CASE("realpath") {
  auto actual = krbn::filesystem::realpath("/bin/ls");
  REQUIRE(*actual == "/bin/ls");

  actual = krbn::filesystem::realpath("/var/log/not_found");
  REQUIRE(actual == boost::none);

  actual = krbn::filesystem::realpath("/var/log/system.log");
  REQUIRE(*actual == "/private/var/log/system.log");
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
