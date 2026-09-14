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
    let catalog = try LocalizationCatalog(data: Data(contentsOf: source))
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
        == "Alone key timeout (ms)")
    // Flattening nested sections must not produce duplicate row IDs.
    precondition(Set(englishRows.map(\.id)).count == englishRows.count)
    // Copied text retains full configuration keys even when labels are translated.
    precondition(
      english.text(version: "test", systemVersion: "test").contains(
        "[global.enable_cgeventtap_fallback]"))
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
    // Changing the display language must preserve row identities and ordering.
    precondition(englishRows.map(\.id) == japaneseRows.map(\.id))
    // The same setting resolves to its Japanese label when Japanese is selected.
    precondition(
      japaneseRows.first { $0.id == "global.enable_cgeventtap_fallback" }?.label
        == "CGEventTap fallback を有効にする")
    // Changing locale back must not leave Japanese strings cached in the report.
    let again = try ChangedSettings(json: json, locale: en, catalog: catalog)
    precondition(again.sections.flatMap(\.rows).map(\.label) == englishRows.map(\.label))
    print("Changed Settings report tests passed")
  }
}
