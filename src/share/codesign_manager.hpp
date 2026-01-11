#pragma once

#include "logger.hpp"
#include <pqrs/gsl.hpp>
#include <pqrs/osx/codesign.hpp>
#include <thread>

namespace krbn {

class codesign_manager final {
public:
  codesign_manager(void)
      : current_process_signing_information_(pqrs::osx::codesign::get_signing_information_of_process(getpid())) {
  }

  void log(void) {
    auto team_id = current_process_signing_information_.get_verified_team_id().value_or(pqrs::osx::codesign::team_id("empty"));

    logger::get_logger()->info("signing_information team_id: {0}",
                               type_safe::get(team_id));
  }

  bool same_team_id(std::optional<pid_t> pid) {
    // If the binary is not code-signed (e.g., a self-build from source), always return true.
    auto current_team_id = current_process_signing_information_.get_verified_team_id();
    if (current_team_id == std::nullopt) {
      return true;
    }

    if (pid == std::nullopt) {
      return false;
    }

    auto signing_information = pqrs::osx::codesign::get_signing_information_of_process(*pid);
    return current_team_id == signing_information.get_verified_team_id();
  }

private:
  pqrs::osx::codesign::signing_information current_process_signing_information_;
};

inline pqrs::not_null_shared_ptr_t<codesign_manager> get_shared_codesign_manager(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);

  static std::shared_ptr<codesign_manager> p;
  if (!p) {
    p = std::make_shared<codesign_manager>();
  }

  return p;
}

} // namespace krbn
