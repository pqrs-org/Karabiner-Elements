#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include <ostream>

#include "filesystem.hpp"

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
