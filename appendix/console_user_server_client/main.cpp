#include "logger.hpp"
#include "manipulator/details/post_event_to_virtual_devices.hpp"
#include "thread_utility.hpp"
#include "time_utility.hpp"
#include "virtual_hid_device_client.hpp"

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  try {
    krbn::console_user_server_client client(getuid());
    //krbn::console_user_server_client client(201);

    {
      std::string shell_command = "open /Applications/Safari.app";
      client.shell_command_execution(shell_command);
    }
  } catch (std::exception& e) {
    krbn::logger::get_logger().error(e.what());
  }

  return 0;
}
