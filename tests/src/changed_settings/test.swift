import Foundation

// Verify formatting of an already summarized changes JSON: localized labels and values,
// stable row IDs, copied configuration keys, and handling of unknown keys or invalid input.
// Difference extraction and modification/device counts are covered by
// changed_settings_integration using real C++ configuration snapshots.
@main
@MainActor
struct ChangedSettingsTests {
  static func main() throws {
    let source = URL(fileURLWithPath: CommandLine.arguments[1])
    let catalog = try LocalizationCatalog(file: source)
    let en = AppLanguage.locale(for: "en", catalog: catalog)

    let json = #"""
      {
          "global": {
              "enable_cgeventtap_fallback": true,
              "notification_window_font_size": 18,
              "future_option": "custom"
          },
          "selected_profile": {
              "configured_devices_count": 2,
              "complex_modifications": {
                  "parameters": {
                      "basic.to_if_alone_timeout_milliseconds": 777
                  }
              }
          }
      }
      """#
    let english = try ChangedSettings(json: json, locale: en, catalog: catalog)
    let englishRows = english.sections.flatMap(\.rows)
    // Known setting keys resolve to their English labels.
    precondition(
      englishRows.first { $0.id == "global.enable_cgeventtap_fallback" }?.label
        == "Enable CGEventTap fallback")
    // Boolean values use localized on/off text instead of raw JSON booleans.
    precondition(englishRows.first { $0.id == "global.enable_cgeventtap_fallback" }?.value == "On")
    // Numeric values remain numeric text rather than being treated as booleans.
    precondition(
      englishRows.first { $0.id == "global.notification_window_font_size" }?.value == "18")
    // Unknown settings remain identifiable through their original key names.
    precondition(englishRows.first { $0.id == "global.future_option" }?.label == "future_option")
    // The device summary count has an English label.
    precondition(
      englishRows.first { $0.id == "selected_profile.configured_devices_count" }?.label
        == "Configured device count")
    // Formatting preserves the supplied device count; it does not recalculate it.
    precondition(
      englishRows.first { $0.id == "selected_profile.configured_devices_count" }?.value == "2")
    // Dotted parameter keys inside nested sections resolve to their explicit labels.
    precondition(
      englishRows.first { $0.id.hasSuffix("basic.to_if_alone_timeout_milliseconds") }?.label
        == "to_if_alone_timeout_milliseconds:")
    // Flattening nested sections must not produce duplicate row IDs.
    precondition(Set(englishRows.map(\.id)).count == englishRows.count)
    // Notification rows stay adjacent in Global, with a shared label prefix and original keys.
    let notifications = try ChangedSettings(
      json: #"""
        {"global": {
          "enable_notification_window": false,
          "indicate_sticky_modifier_keys_state": false,
          "notification_window_font_size": 18,
          "notification_window_show_icon": false,
          "notification_window_colors": {
            "dark": {"background_color": "#112233", "text_color": "#ffffff"},
            "light": {"background_color": "#445566", "text_color": "#000000"}
          },
          "filter_useless_events_from_specific_devices": false,
          "show_in_menu_bar": false
        }}
        """#,
      locale: en, catalog: catalog)
    precondition(notifications.sections.map(\.id) == ["global"])
    let notificationRows = notifications.sections[0].rows
    let prefixedIndexes = notificationRows.indices.filter {
      notificationRows[$0].label.hasPrefix("Notification Window / ")
    }
    precondition(prefixedIndexes.count == 8)
    precondition(prefixedIndexes.last! - prefixedIndexes.first! == 7)
    let colorRows = notificationRows.filter {
      $0.id.hasPrefix("global.notification_window_colors.")
    }
    precondition(colorRows.count == 4)
    precondition(Set(colorRows.map(\.label)).count == 4)
    precondition(
      colorRows.first {
        $0.id == "global.notification_window_colors.dark.background_color"
      }?.value == "#112233")
    precondition(
      notifications.text(version: "test", systemVersion: "test").contains(
        "[global.notification_window_colors.dark.background_color]"))
    let colorsOnly = try ChangedSettings(
      json: ##"{"global":{"notification_window_colors":{"light":{"text_color":"#000000"}}}}"##,
      locale: en, catalog: catalog)
    precondition(colorsOnly.sections.map(\.id) == ["global"])
    precondition(colorsOnly.sections[0].rows.count == 1)
    // Verify display order in both languages; IDs remain stable even when order changes.
    func checkDisplayOrder(_ report: ChangedSettings, locale: Locale) {
      for section in report.sections {
        for (first, second) in zip(section.rows, section.rows.dropFirst()) {
          precondition(
            first.label.compare(
              second.label, options: [.caseInsensitive, .numeric], locale: locale
            ) != .orderedDescending)
        }
      }
    }
    checkDisplayOrder(notifications, locale: en)
    let machine = try ChangedSettings(
      json:
        #"{"machine_specific":{"enable_multitouch_extension":true},"future_section":{"future_option":false}}"#,
      locale: en, catalog: catalog)
    // Known section titles are localized as well as individual setting labels.
    precondition(
      machine.sections.first { $0.id == "machine_specific" }?.title
        == "Settings specific to this machine")
    // Unknown section titles fall back to their original keys.
    precondition(machine.sections.first { $0.id == "future_section" }?.title == "future_section")
    // An empty changes object produces no sections for the empty-state UI.
    let empty = try ChangedSettings(json: "{}", locale: en, catalog: catalog)
    precondition(empty.sections.isEmpty)
    // The changes document must be an object; an array root is rejected.
    do {
      _ = try ChangedSettings(json: "[]", locale: en, catalog: catalog)
      preconditionFailure("Invalid report root accepted")
    } catch {}
    // Use Japanese only as a second locale to exercise switching, not as a
    // per-language checklist. Adding a language does not require duplicating these tests.
    let ja = AppLanguage.locale(for: "ja", catalog: catalog)
    let japanese = try ChangedSettings(json: json, locale: ja, catalog: catalog)
    let japaneseRows = japanese.sections.flatMap(\.rows)
    // Changing the display language preserves row identities, but may change ordering.
    precondition(Set(englishRows.map(\.id)) == Set(japaneseRows.map(\.id)))
    checkDisplayOrder(english, locale: en)
    checkDisplayOrder(japanese, locale: ja)
    // The same setting resolves to its Japanese label when Japanese is selected.
    precondition(
      japaneseRows.first { $0.id == "global.enable_cgeventtap_fallback" }?.label
        == "CGEventTapフォールバックを有効にする")
    precondition(
      japaneseRows.first { $0.id == "global.notification_window_font_size" }?.label.hasPrefix(
        "通知ウインドウ / ") == true)
    // Copying rebuilds the report in English, including section titles, values and row order.
    let clipboard = try ChangedSettings.clipboardText(
      json: json, version: "test", systemVersion: "13.0.0", catalog: catalog)
    precondition(clipboard == english.text(version: "test", systemVersion: "13.0.0"))
    precondition(clipboard.contains("Global settings"))
    precondition(
      clipboard.contains("Enable CGEventTap fallback: On [global.enable_cgeventtap_fallback]"))
    let emptyClipboard = try ChangedSettings.clipboardText(
      json: "{}", version: nil, systemVersion: "13.0.0", catalog: catalog)
    precondition(
      emptyClipboard == "Karabiner-Elements: "
        + AppLanguage.text("shared.value.unknown", locale: en, catalog: catalog)
        + "\nmacOS: 13.0.0\n\nNo settings have been changed from their default values.")
    // User-provided strings are configuration data and must not be translated.
    let customClipboard = try ChangedSettings.clipboardText(
      json: #"{"global":{"future_option":"日本語の値"}}"#,
      version: "test", systemVersion: "13.0.0", catalog: catalog)
    precondition(customClipboard.contains("future_option: 日本語の値 [global.future_option]"))
    // The stored ignore flag is presented as the inverse modify-events option used by the UI.
    for ignore in [false, true] {
      let report = try ChangedSettings(
        json: "{\"selected_profile\":{\"ignore_pointing_device_events_by_default\":\(ignore)}}",
        locale: en, catalog: catalog)
      let row = report.sections[0].rows[0]
      precondition(row.id == "selected_profile.ignore_pointing_device_events_by_default")
      precondition(
        row.label
          == AppLanguage.text(
            "settings.expert.modify_pointing_by_default", locale: en, catalog: catalog))
      precondition(row.value == (ignore ? "Off" : "On"))
    }
    print("Changed Settings report tests passed")
  }
}
