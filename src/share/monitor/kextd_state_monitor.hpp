#pragma once

// `krbn::kextd_state_monitor` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "logger.hpp"
#include <fstream>
#include <libkern/OSKextLib.h>
#include <nod/nod.hpp>
#include <pqrs/filesystem.hpp>
#include <pqrs/json.hpp>
#include <pqrs/osx/file_monitor.hpp>

namespace krbn {
class kextd_state_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(kern_return_t)> kext_load_result_changed;

  // Methods

  kextd_state_monitor(const kextd_state_monitor&) = delete;

  kextd_state_monitor(const std::string& kextd_state_json_file_path) {
    std::vector<std::string> targets = {
        kextd_state_json_file_path,
    };

    file_monitor_ = std::make_unique<pqrs::osx::file_monitor>(weak_dispatcher_,
                                                              targets);

    file_monitor_->file_changed.connect([this](auto&& changed_file_path,
                                               auto&& changed_file_body) {
      if (changed_file_body) {
        try {
          auto json = nlohmann::json::parse(*changed_file_body);

          // json example
          //
          // {
          //     "kext_load_result": 27
          // }

          if (!json.is_object()) {
            throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
          }

          for (const auto& [key, value] : json.items()) {
            if (key == "kext_load_result") {
              if (!value.is_number()) {
                throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
              }

              auto v = value.template get<kern_return_t>();
              if (last_kext_load_result_ != v) {
                last_kext_load_result_ = v;

                enqueue_to_dispatcher([this, v] {
                  kext_load_result_changed(v);
                });
              }
            } else {
              throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
            }
          }
        } catch (std::exception& e) {
          logger::get_logger()->error("parse error in {0}: {1}", changed_file_path, e.what());
        }
      }
    });
  }

  virtual ~kextd_state_monitor(void) {
    detach_from_dispatcher([this] {
      file_monitor_ = nullptr;
    });
  }

  void async_start(void) {
    file_monitor_->async_start();
  }

private:
  std::unique_ptr<pqrs::osx::file_monitor> file_monitor_;
  std::optional<kern_return_t> last_kext_load_result_;
};
} // namespace krbn
