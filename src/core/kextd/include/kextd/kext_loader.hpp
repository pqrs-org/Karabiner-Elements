#pragma once

// `krbn::kextd::kext_loader` can be used safely in a multi-threaded environment.

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "monitor/version_monitor.hpp"
#include "state_json_writer.hpp"
#include <IOKit/kext/KextManager.h>
#include <nlohmann/json.hpp>
#include <pqrs/cf/url.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace kextd {
class kext_loader final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> kext_loaded;

  // Methods

  kext_loader(const kext_loader&) = delete;

  kext_loader(std::weak_ptr<version_monitor> weak_version_monitor) : dispatcher_client(),
                                                                     weak_version_monitor_(weak_version_monitor),
                                                                     timer_(*this),
                                                                     state_json_writer_(constants::get_kextd_state_json_file_path()) {
  }

  virtual ~kext_loader(void) {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      timer_.start(
          [this] {
            if (auto m = weak_version_monitor_.lock()) {
              m->async_manual_check();
            }

            auto kext_file_path =
                std::string("/Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice/Extensions/") +
                pqrs::karabiner_virtual_hid_device::get_kernel_extension_name();

            if (auto url = pqrs::cf::make_file_path_url(kext_file_path, false)) {
              auto kr = KextManagerLoadKextWithURL(*url, nullptr);
              krbn::logger::get_logger()->info("KextManagerLoadKextWithURL: {0}", kr);

              state_json_writer_.set("kext_load_result", kr);

              if (kr == kOSReturnSuccess) {
                timer_.stop();

                enqueue_to_dispatcher([this] {
                  kext_loaded();
                });
              }
            }
          },
          std::chrono::milliseconds(3000));
    });
  }

private:
  std::weak_ptr<version_monitor> weak_version_monitor_;
  pqrs::dispatcher::extra::timer timer_;
  state_json_writer state_json_writer_;
};
} // namespace kextd
} // namespace krbn
