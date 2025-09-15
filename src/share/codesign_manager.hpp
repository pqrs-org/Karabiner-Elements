#pragma once

#include "logger.hpp"
#include <gsl/gsl>
#include <pqrs/osx/codesign.hpp>
#include <thread>

namespace krbn {

class codesign_manager final {
public:
  codesign_manager(void)
      : current_process_signing_information_(pqrs::osx::codesign::get_signing_information_of_process(getpid())) {
  }

  void log(void) {
    logger::get_logger()->info("signing_information team_id: {0}",
                               current_process_signing_information_.get_team_id().value_or("empty"));
  }

private:
  pqrs::osx::codesign::signing_information current_process_signing_information_;
};

inline gsl::not_null<std::shared_ptr<codesign_manager>> get_shared_codesign_manager(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);

  static std::shared_ptr<codesign_manager> p;
  if (!p) {
    p = std::make_shared<codesign_manager>();
  }

  return p;
}

} // namespace krbn
