#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "async_sequential_file_writer.hpp"

TEST_CASE("async_sequential_file_writer") {
  krbn::async_sequential_file_writer writer;
  writer.push_back("tmp/example", "example1", 0755, 0600);
  writer.push_back("tmp/example", "example2", 0755, 0600);
  writer.push_back("tmp/example", "example3", 0755, 0600);
  writer.push_back("tmp/mode666", "mode666", 0755, 0666);
  writer.push_back("tmp/mode644", "mode644", 0755, 0644);
  writer.push_back("tmp/not_found/example", "example", 0755, 0600);
  writer.wait();
}
