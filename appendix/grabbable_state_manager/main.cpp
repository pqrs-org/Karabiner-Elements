#include "boost_defs.hpp"

#include "grabbable_state_manager.hpp"
#include "hid_manager.hpp"
#include "hid_observer.hpp"
#include <boost/optional/optional_io.hpp>

namespace {
class grabbable_state_manager_demo final {
public:
  grabbable_state_manager_demo(const grabbable_state_manager_demo&) = delete;

  grabbable_state_manager_demo(void) {
    queue_ = std::make_unique<krbn::thread_utility::queue>();

    grabbable_state_manager_ = std::make_unique<krbn::grabbable_state_manager>();

    grabbable_state_manager_->grabbable_state_changed.connect([this](auto&& grabbable_state) {
      queue_->push_back([grabbable_state] {
        std::cout << "grabbable_state_changed "
                  << grabbable_state.get_registry_entry_id()
                  << " "
                  << grabbable_state.get_state()
                  << std::endl;
      });
    });

    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_mouse),
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_pointer),
    });

    hid_manager_ = std::make_unique<krbn::hid_manager>(targets);

    hid_manager_->device_detected.connect([this](auto&& weak_hid) {
      queue_->push_back([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          hid->values_arrived.connect([this](auto&& shared_event_queue) {
            queue_->push_back([this, shared_event_queue] {
              if (grabbable_state_manager_) {
                grabbable_state_manager_->update(*shared_event_queue);
              }
            });
          });

          // Observe

          auto hid_observer = std::make_shared<krbn::hid_observer>(hid);

          hid_observer->device_observed.connect([this, weak_hid] {
            queue_->push_back([weak_hid] {
              if (auto hid = weak_hid.lock()) {
                krbn::logger::get_logger().info("{0} is observed.", hid->get_name_for_log());
              }
            });
          });

          hid_observer->device_unobserved.connect([this, weak_hid] {
            queue_->push_back([weak_hid] {
              if (auto hid = weak_hid.lock()) {
                krbn::logger::get_logger().info("{0} is unobserved.", hid->get_name_for_log());
              }
            });
          });

          hid_observer->async_observe();

          hid_observers_[hid->get_registry_entry_id()] = hid_observer;
        }
      });
    });

    hid_manager_->device_removed.connect([this](auto&& weak_hid) {
      queue_->push_back([this, weak_hid] {
        if (auto hid = weak_hid.lock()) {
          krbn::logger::get_logger().info("{0} is removed.", hid->get_name_for_log());
          hid_observers_.erase(hid->get_registry_entry_id());
        }
      });
    });

    hid_manager_->async_start();
  }

  ~grabbable_state_manager_demo(void) {
    queue_->push_back([this] {
      hid_manager_ = nullptr;
      hid_observers_.clear();
    });

    queue_->terminate();
    queue_ = nullptr;
  }

private:
  std::unique_ptr<krbn::thread_utility::queue> queue_;
  std::unique_ptr<krbn::grabbable_state_manager> grabbable_state_manager_;
  std::unique_ptr<krbn::hid_manager> hid_manager_;
  std::unordered_map<krbn::registry_entry_id, std::shared_ptr<krbn::hid_observer>> hid_observers_;
};
} // namespace

int main(int argc, const char* argv[]) {
  krbn::thread_utility::register_main_thread();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto d = std::make_unique<grabbable_state_manager_demo>();

  CFRunLoopRun();

  d = nullptr;

  return 0;
}
