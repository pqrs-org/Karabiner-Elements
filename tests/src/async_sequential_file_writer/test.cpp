#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "async_sequential_file_writer.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("async_sequential_file_writer") {
  krbn::async_sequential_file_writer writer;
  writer.push_back("tmp/example", "example1", boost::none);
  writer.push_back("tmp/example", "example2", boost::none);
  writer.push_back("tmp/example", "example3", boost::none);
  writer.push_back("tmp/mode666", "mode666", 0666);
  writer.push_back("tmp/mode644", "mode644", 0644);
  writer.push_back("not_found/example", "example", boost::none);
  writer.wait();
}
