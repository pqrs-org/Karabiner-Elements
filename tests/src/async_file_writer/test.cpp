#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "async_file_writer.hpp"
#include "dispatcher_utility.hpp"

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("async_file_writer") {
  krbn::async_file_writer::enqueue("tmp/example", "example1", 0755, 0600);
  krbn::async_file_writer::enqueue("tmp/example", "example2", 0755, 0600);
  krbn::async_file_writer::enqueue("tmp/example", "example3", 0755, 0600);
  krbn::async_file_writer::enqueue("tmp/mode666", "mode666", 0755, 0666);
  krbn::async_file_writer::enqueue("tmp/mode644", "mode644", 0755, 0644);
  krbn::async_file_writer::enqueue("tmp/not_found/example", "example", 0755, 0600);
  krbn::async_file_writer::wait();
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
