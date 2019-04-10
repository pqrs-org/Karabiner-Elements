#include "device_properties_manager.hpp"
#include "dispatcher_utility.hpp"
#include "event_queue.hpp"
#include "iokit_utility.hpp"
#include <csignal>
#include <iomanip>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

namespace {
class dump_hid_value final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  dump_hid_value(const dump_hid_value&) = delete;

  dump_hid_value(void) : dispatcher_client() {
    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_pointer),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(weak_dispatcher_,
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);
        krbn::logger::get_logger()->info("{0} is matched.",
                                         krbn::iokit_utility::make_device_name_for_log(device_id,
                                                                                       *device_ptr));

        auto device_properties = std::make_shared<krbn::device_properties>(device_id,
                                                                           *device_ptr);
        std::cout << std::setw(4) << device_properties->to_json() << std::endl;

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(weak_dispatcher_,
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        hid_queue_value_monitor->values_arrived.connect([this, device_id](auto&& values_ptr) {
          auto event_queue = krbn::event_queue::utility::make_queue(device_id,
                                                                    krbn::iokit_utility::make_hid_values(values_ptr));
          for (const auto& entry : event_queue->get_entries()) {
            output_value(entry);
          }
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = krbn::make_device_id(registry_entry_id);

      krbn::logger::get_logger()->info("device_id:{0} is terminated.", type_safe::get(device_id));

      hid_queue_value_monitors_.erase(device_id);
    });

    hid_manager_->async_start();
  }

  virtual ~dump_hid_value(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_queue_value_monitors_.clear();
    });
  }

private:
  void output_value(const krbn::event_queue::entry& entry) const {
    switch (entry.get_event().get_type()) {
      case krbn::event_queue::event::type::key_code:
        if (auto key_code = entry.get_event().get_key_code()) {
          std::cout << entry.get_event_time_stamp().get_time_stamp() << " ";
          std::cout << "Key: " << std::dec << static_cast<uint32_t>(*key_code) << " "
                    << entry.get_event_type()
                    << std::endl;
        }
        break;

      case krbn::event_queue::event::type::consumer_key_code:
        if (auto consumer_key_code = entry.get_event().get_consumer_key_code()) {
          std::cout << entry.get_event_time_stamp().get_time_stamp() << " ";
          std::cout << "ConsumerKey: " << std::dec << static_cast<uint32_t>(*consumer_key_code) << " "
                    << entry.get_event_type()
                    << std::endl;
        }
        break;

      case krbn::event_queue::event::type::pointing_button:
        if (auto pointing_button = entry.get_event().get_pointing_button()) {
          std::cout << entry.get_event_time_stamp().get_time_stamp() << " ";
          std::cout << "Button: " << std::dec << static_cast<uint32_t>(*pointing_button) << " "
                    << entry.get_event_type()
                    << std::endl;
        }
        break;

      case krbn::event_queue::event::type::pointing_motion:
        if (auto pointing_motion = entry.get_event().get_pointing_motion()) {
          auto json = nlohmann::json::object({
              {"pointing_motion", *pointing_motion},
              {"time_stamp", entry.get_event_time_stamp().get_time_stamp()},
          });
          std::cout << json << "," << std::endl;
        }
        break;

      default:
        break;
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;
};

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  std::signal(SIGINT, [](int) {
    global_wait->notify();
  });

  auto d = std::make_unique<dump_hid_value>();

  // ------------------------------------------------------------

  global_wait->wait_notice();

  // ------------------------------------------------------------

  d = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  std::cout << "finished" << std::endl;

  return 0;
}
