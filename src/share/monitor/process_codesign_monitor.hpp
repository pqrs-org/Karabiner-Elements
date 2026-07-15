#pragma once

// `krbn::process_codesign_monitor` can be used safely in a multi-threaded environment.

#include "codesign_manager.hpp"
#include "logger.hpp"
#include <chrono>
#include <functional>
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
class process_codesign_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  using team_id_provider = std::function<std::optional<pqrs::osx::codesign::team_id>()>;

  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void()> invalidated;

  // Methods

  process_codesign_monitor(const process_codesign_monitor&) = delete;

  process_codesign_monitor()
      : process_codesign_monitor(
            [] {
              return pqrs::osx::codesign::get_signing_information_of_process(getpid()).get_verified_team_id();
            },
            std::chrono::seconds(1),
            3) {
  }

  process_codesign_monitor(team_id_provider team_id_provider,
                           pqrs::dispatcher::duration interval,
                           int required_consecutive_failures)
      : timer_(*this),
        team_id_provider_(std::move(team_id_provider)),
        interval_(interval),
        required_consecutive_failures_(required_consecutive_failures),
        initial_team_id_(team_id_provider_()) {
  }

  ~process_codesign_monitor() override {
    detach_from_dispatcher([this] {
      timer_.stop();
    });
  }

  void async_start() {
    // An unsigned local build does not have a Team ID, so there is no known-good
    // state against which later checks can be compared.
    if (!initial_team_id_) {
      return;
    }

    timer_.start(
        [this] {
          check();
        },
        interval_);
  }

  [[nodiscard]] bool active() const {
    return initial_team_id_ != std::nullopt;
  }

  void async_manual_check() {
    enqueue_to_dispatcher([this] {
      check();
    });
  }

private:
  void check() {
    if (notified_ || !initial_team_id_) {
      return;
    }

    auto team_id = team_id_provider_();
    if (team_id == initial_team_id_) {
      consecutive_failures_ = 0;
      return;
    }

    ++consecutive_failures_;
    logger::get_logger()->info("Current process code signature validation failed ({0}/{1})",
                               consecutive_failures_,
                               required_consecutive_failures_);

    if (consecutive_failures_ >= required_consecutive_failures_) {
      notified_ = true;
      timer_.stop();
      invalidated();
    }
  }

  pqrs::dispatcher::extra::timer timer_;
  team_id_provider team_id_provider_;
  pqrs::dispatcher::duration interval_;
  int required_consecutive_failures_;
  std::optional<pqrs::osx::codesign::team_id> initial_team_id_;
  int consecutive_failures_ = 0;
  bool notified_ = false;
};
} // namespace krbn
