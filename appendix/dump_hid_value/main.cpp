#include "boost_defs.hpp"

#include "dispatcher_utility.hpp"
#include "hid_observer.hpp"
#include <boost/optional/optional_io.hpp>
#include <csignal>
#include <pqrs/osx/iokit_hid_manager.hpp>

namespace {
class dump_hid_value final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  dump_hid_value(const dump_hid_value&) = delete;

  dump_hid_value(void) : dispatcher_client() {
    std::vector<pqrs::cf_ptr<CFDictionaryRef>> matching_dictionaries{
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
      auto hid = std::make_shared<krbn::human_interface_device>(*device_ptr,
                                                                registry_entry_id);
      hids_[registry_entry_id] = hid;
      auto device_name = hid->get_name_for_log();

      hid->values_arrived.connect([this, registry_entry_id](auto&& shared_event_queue) {
        values_arrived(shared_event_queue, registry_entry_id);
      });

      // Observe

      auto hid_observer = std::make_shared<krbn::hid_observer>(hid);

      hid_observer->device_observed.connect([device_name, registry_entry_id] {
        krbn::logger::get_logger().info("{0}:{1} is observed.",
                                        device_name,
                                        type_safe::get(registry_entry_id));
      });

      hid_observer->device_unobserved.connect([device_name, registry_entry_id] {
        krbn::logger::get_logger().info("{0}:{1} is unobserved.",
                                        device_name,
                                        type_safe::get(registry_entry_id));
      });

      hid_observer->async_observe();

      hid_observers_[hid->get_registry_entry_id()] = hid_observer;
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      krbn::logger::get_logger().info("registry_entry_id:{0} is terminated.", type_safe::get(registry_entry_id));

      hids_.erase(registry_entry_id);
      hid_observers_.erase(registry_entry_id);
    });

    hid_manager_->async_start();
  }

  virtual ~dump_hid_value(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_observers_.clear();
    });
  }

private:
  void values_arrived(std::shared_ptr<krbn::event_queue::queue> event_queue,
                      pqrs::osx::iokit_registry_entry_id registry_entry_id) const {
    for (const auto& entry : event_queue->get_entries()) {
      std::cout << entry.get_event_time_stamp().get_time_stamp() << " ";

      switch (entry.get_event().get_type()) {
        case krbn::event_queue::event::type::none:
          std::cout << "none" << std::endl;
          break;

        case krbn::event_queue::event::type::key_code:
          if (auto key_code = entry.get_event().get_key_code()) {
            std::cout << "Key: " << std::dec << static_cast<uint32_t>(*key_code) << " "
                      << entry.get_event_type()
                      << std::endl;
          }
          break;

        case krbn::event_queue::event::type::consumer_key_code:
          if (auto consumer_key_code = entry.get_event().get_consumer_key_code()) {
            std::cout << "ConsumerKey: " << std::dec << static_cast<uint32_t>(*consumer_key_code) << " "
                      << entry.get_event_type()
                      << std::endl;
          }
          break;

        case krbn::event_queue::event::type::pointing_button:
          if (auto pointing_button = entry.get_event().get_pointing_button()) {
            std::cout << "Button: " << std::dec << static_cast<uint32_t>(*pointing_button) << " "
                      << entry.get_event_type()
                      << std::endl;
          }
          break;

        case krbn::event_queue::event::type::pointing_motion:
          if (auto pointing_motion = entry.get_event().get_pointing_motion()) {
            std::cout << "pointing_motion: " << pointing_motion->to_json() << std::endl;
          }
          break;

        case krbn::event_queue::event::type::shell_command:
          std::cout << "shell_command" << std::endl;
          break;

        case krbn::event_queue::event::type::select_input_source:
          std::cout << "select_input_source" << std::endl;
          break;

        case krbn::event_queue::event::type::set_variable:
          std::cout << "set_variable" << std::endl;
          break;

        case krbn::event_queue::event::type::mouse_key:
          std::cout << "mouse_key" << std::endl;
          break;

        case krbn::event_queue::event::type::stop_keyboard_repeat:
          std::cout << "stop_keyboard_repeat" << std::endl;
          break;

        case krbn::event_queue::event::type::device_keys_and_pointing_buttons_are_released:
          std::cout << "device_keys_and_pointing_buttons_are_released: " << registry_entry_id << std::endl;
          break;

        case krbn::event_queue::event::type::device_ungrabbed:
          std::cout << "device_ungrabbed: " << registry_entry_id << std::endl;
          break;

        case krbn::event_queue::event::type::caps_lock_state_changed:
          if (auto integer_value = entry.get_event().get_integer_value()) {
            std::cout << "caps_lock_state_changed " << *integer_value << std::endl;
          }
          break;

        case krbn::event_queue::event::type::pointing_device_event_from_event_tap:
          std::cout << "pointing_device_event_from_event_tap: " << registry_entry_id << std::endl;
          break;

        case krbn::event_queue::event::type::frontmost_application_changed:
          if (auto frontmost_application = entry.get_event().get_frontmost_application()) {
            std::cout << "frontmost_application_changed "
                      << frontmost_application->get_bundle_identifier() << " "
                      << frontmost_application->get_file_path() << std::endl;
          }
          break;

        case krbn::event_queue::event::type::input_source_changed:
          if (auto input_source_identifiers = entry.get_event().get_input_source_identifiers()) {
            std::cout << "input_source_changed " << input_source_identifiers << std::endl;
          }
          break;

        case krbn::event_queue::event::type::keyboard_type_changed:
          if (auto keyboard_type = entry.get_event().get_keyboard_type()) {
            std::cout << "keyboard_type_changed " << keyboard_type << std::endl;
          }
          break;

        default:
          std::cout << std::endl;
      }
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<krbn::human_interface_device>> hids_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<krbn::hid_observer>> hid_observers_;
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
