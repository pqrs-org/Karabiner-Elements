#include "dispatcher_client_constructor_guard.hpp"
#include <boost/ut.hpp>
#include <stdexcept>

namespace {
class throwing_member final {
public:
  explicit throwing_member(bool fail) {
    if (fail) {
      throw std::runtime_error("member initialization failed");
    }
  }
};

class guarded_dispatcher_client final : public pqrs::dispatcher::extra::dispatcher_client {
  krbn::dispatcher_client_constructor_guard dispatcher_client_constructor_guard_{*this};
  throwing_member member_;
  pqrs::dispatcher::extra::debounced_task task_{*this};

public:
  guarded_dispatcher_client(bool fail_member, bool fail_body, bool* cleaned_up = nullptr) : member_(fail_member) {
    dispatcher_client_constructor_guard_.initialize(
        [&] {
      if (fail_body) {
        throw std::runtime_error("constructor body failed");
      } }, [&] {
      if (cleaned_up) {
        *cleaned_up = !attached() && dispatcher_thread();
      } });
  }

  ~guarded_dispatcher_client() override {
    detach_from_dispatcher();
  }
};
} // namespace

void run_dispatcher_client_constructor_guard_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "dispatcher client constructor failure"_test = [] {
    // Without the guard, the base destructor aborts before these exceptions
    // can reach the caller, including when the constructor body is never entered.
    expect(throws<std::runtime_error>([] {
      guarded_dispatcher_client client(true, false);
    }));
    bool cleaned_up = false;
    expect(throws<std::runtime_error>([&] {
      guarded_dispatcher_client client(false, true, &cleaned_up);
    }));
    expect(cleaned_up);

    // Completing construction must leave the client attached for normal work.
    cleaned_up = false;
    guarded_dispatcher_client client(false, false, &cleaned_up);
    expect(client.attached());
    expect(!cleaned_up);
  };
}
