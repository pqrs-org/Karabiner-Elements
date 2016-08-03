#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>

#include <iostream>
#include <memory>
#include <thread>

// asio headers
#define ASIO_STANDALONE
#include <asio.hpp>

#include "grabber_client.hpp"
#include "receiver.hpp"
#include "io_hid_post_event_wrapper.hpp"

int main(int argc, const char* argv[]) {
  receiver r;
  auto th = r.start();

  CFRunLoopRun();
  return 0;
}
