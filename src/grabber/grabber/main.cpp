#include "grabber.hpp"

int main(int argc, const char* argv[]) {
  user_client client;
  if (client.open("org_pqrs_driver_VirtualHIDManager")) {
    std::cout << "connected: " << client.get_connect() << std::endl;
  } else {
    std::cout << "failed to connect" << std::endl;
  }

  hid_report::keyboard_input report;
  kern_return_t kr = IOConnectCallStructMethod(client.get_connect(),
                                               static_cast<uint32_t>(virtual_hid_manager_user_client_method::report),
                                               static_cast<const void*>(&report), sizeof(report),
                                               nullptr, 0);
  if (kr == KERN_SUCCESS) {
    std::cout << "report sent" << std::endl;
  } else {
    std::cout << "failed to sent report: 0x" << std::hex << kr << std::endl;
  }

  event_grabber observer;
  CFRunLoopRun();
  return 0;
}
