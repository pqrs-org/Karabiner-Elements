#include "../../../../src/apps/ConsoleUserServer/include/console_user_server/ui_bridge.hpp"
#include "dispatcher_utility.hpp"
#include <boost/ut.hpp>
#include <future>
#include <thread>
#include <vector>

int main() {
  using namespace boost::ut;
  auto dispatchers = krbn::dispatcher_utility::initialize_dispatchers();

  "resolved language is cached and emitted on the dispatcher"_test = [] {
    krbn::console_user_server::ui_bridge bridge;
    expect(bridge.get_resolved_ui_language() == "en");
    std::vector<std::string> languages;
    std::thread::id callback_thread;
    std::promise<void> completed;
    auto future = completed.get_future();
    bridge.resolved_ui_language_changed.connect([&](const auto& language) {
      callback_thread = std::this_thread::get_id();
      languages.push_back(language);
      // Reading the cache from a signal handler must not deadlock.
      expect(bridge.get_resolved_ui_language() == language);
    });
    bridge.profile_selection_requested.connect([&](auto) { completed.set_value(); });

    bridge.async_set_resolved_ui_language("ja");
    bridge.async_set_resolved_ui_language("ja");
    bridge.async_set_resolved_ui_language("");
    bridge.async_set_resolved_ui_language("auto");
    bridge.async_set_resolved_ui_language("en");
    // Profile requests share the same dispatcher queue and serve as a completion barrier.
    bridge.select_profile(0);
    const auto ready = future.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
    expect(ready) << fatal;
    expect(languages == std::vector<std::string>{"ja", "en"});
    expect(callback_thread != std::this_thread::get_id());
    // A reconnect can read the current value even without a new language change.
    expect(bridge.get_resolved_ui_language() == "en");
    bridge.unregister_callbacks_and_detach();
  };
}
