#pragma once

#include "filesystem_utility.hpp"
#include "logger.hpp"
#include <filesystem>
#include <pqrs/filesystem.hpp>
#include <string_view>

namespace krbn::localizations_utility {

inline constexpr size_t max_file_size = 64 * 1024 * 1024;

inline constexpr auto source_file_path =
    "/Applications/Karabiner-Elements.app/Contents/Resources/localizations.json";

// /Library/Application Support/org.pqrs/Karabiner-Elements/localizations.json
inline const auto destination_file_path =
    constants::get_system_application_support_karabiner_elements_directory() / "localizations.json";

// /Library/Application Support/org.pqrs/Karabiner-Elements/localizations.source-version
inline const auto source_version_file_path =
    constants::get_system_application_support_karabiner_elements_directory() / "localizations.source-version";

inline bool write_file(const std::filesystem::path& file_path,
                       std::string_view contents) {
  auto result = pqrs::filesystem::write_file_atomically({
      .path = file_path,
      .contents = contents,
      .permissions = 0644,
  });
  if (!result) {
    logger::get_logger()->error("Failed to write {0}: {1}",
                                file_path.string(),
                                result.error().message());
    return false;
  }

  return true;
}

// Modifying localizations.json inside Karabiner-Elements.app invalidates the
// app's code signature. To allow localization contributors to update the file
// without modifying the signed app bundle, Karabiner-Elements loads the runtime
// copy from /Library/Application Support/org.pqrs/Karabiner-Elements.
//
// localizations.source-version records the version of the bundled source that
// was copied there. When it differs from source_version, the app is considered
// to have been updated, and the runtime localizations.json is replaced with the
// version bundled in Karabiner-Elements.app.
inline void synchronize(std::string_view source_version) {
  auto installed_source_version = pqrs::filesystem::read_file(source_version_file_path,
                                                              {
                                                                  .max_size = max_file_size,
                                                              });

  if (installed_source_version &&
      std::string_view(reinterpret_cast<const char*>((*installed_source_version)->data()),
                       (*installed_source_version)->size()) == source_version &&
      filesystem_utility::exists(destination_file_path)) {
    return;
  }

  auto localizations = pqrs::filesystem::read_file(source_file_path,
                                                   {
                                                       .max_size = max_file_size,
                                                   });
  if (!localizations) {
    logger::get_logger()->error("Failed to read {0}: {1}",
                                source_file_path,
                                localizations.error().message());
    return;
  }

  if (!filesystem_utility::prepare_system_application_support_karabiner_elements_directory() ||
      !write_file(destination_file_path,
                  std::string_view(reinterpret_cast<const char*>((*localizations)->data()),
                                   (*localizations)->size()))) {
    return;
  }

  if (!write_file(source_version_file_path,
                  source_version)) {
    return;
  }

  logger::get_logger()->info("Updated localizations.json to version {0}",
                             source_version);
}

} // namespace krbn::localizations_utility
