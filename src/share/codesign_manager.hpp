#pragma once

#include "logger.hpp"
#include <initializer_list>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/codesign.hpp>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace krbn {

class codesign_manager final {
public:
  codesign_manager()
      : current_process_signing_information_(pqrs::osx::codesign::get_signing_information_of_process(getpid())) {
  }

  void log() {
    auto team_id = current_process_signing_information_.get_verified_team_id().value_or(pqrs::osx::codesign::team_id("empty"));

    logger::get_logger()->info("signing_information team_id: {0}",
                               type_safe::get(team_id));
  }

  [[nodiscard]] bool same_team_id(std::optional<pid_t> pid) {
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

  [[nodiscard]] std::optional<std::string> xpc_signing_requirement(std::initializer_list<std::string_view> identifiers) const {
    if (auto team_id = current_process_signing_information_.get_verified_team_id()) {
      std::vector<pqrs::osx::codesign::signing_requirement_parameters> parameters_list;
      parameters_list.reserve(identifiers.size());
      for (auto identifier : identifiers) {
        parameters_list.emplace_back(pqrs::osx::codesign::signing_requirement_parameters{
            .anchor = pqrs::osx::codesign::anchor_type::apple_generic,
            .team_id = *team_id,
            .identifier = std::string(identifier),
        });
      }

      if (auto requirement = pqrs::osx::codesign::make_or_signing_requirement(parameters_list)) {
        return *requirement;
      }

      throw std::runtime_error("Failed to make the XPC signing requirement");
    }

    // Preserve the development-build policy: unsigned peers are permitted.
    return std::nullopt;
  }

private:
  pqrs::osx::codesign::signing_information current_process_signing_information_;
};

[[nodiscard]] inline pqrs::not_null_shared_ptr_t<codesign_manager> get_shared_codesign_manager() {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);

  static std::shared_ptr<codesign_manager> p;
  if (!p) {
    p = std::make_shared<codesign_manager>();
  }

  return p;
}

} // namespace krbn
