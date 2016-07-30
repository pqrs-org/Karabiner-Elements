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
#include "grabber_observer.hpp"
#include "io_hid_post_event_wrapper.hpp"

int main(int argc, const char* argv[]) {
  grabber_client client;
  client.open();

  io_hid_post_event_wrapper wrapper;
  wrapper.start();

  wrapper.post_modifier_flags(NX_SECONDARYFNMASK);
  wrapper.post_modifier_flags(0);

  return 0;
}
