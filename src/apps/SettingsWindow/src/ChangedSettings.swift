import CoreFoundation
import Foundation

@MainActor
// Collects changed settings and formats them for display and copying.
struct ChangedSettings {
  struct Row: Identifiable {
    let id: String
    let label: String
    let value: String
  }

  struct Section: Identifiable {
    let id: String
    let title: String
    let rows: [Row]
  }

  // Keep localization keys explicit so their usages can be found by searching.
  private static let sectionLocalizationKeys: [String: String] = [
    "complex_modifications": "settings.changed_settings.section.complex_modifications",
    "global": "settings.changed_settings.section.global",
    "machine_specific": "settings.changed_settings.section.machine_specific",
    "parameters": "settings.changed_settings.section.parameters",
    "selected_profile": "settings.changed_settings.section.selected_profile",
    "virtual_hid_keyboard": "settings.changed_settings.section.virtual_hid_keyboard",
  ]

  static let settingLocalizationKeys: [String: [String]] = [
    "background_color": [
      "setting.background_color"
    ],
    "basic.simultaneous_threshold_milliseconds": [
      "settings.complex_modifications.parameters.basic.simultaneous_threshold_milliseconds"
    ],
    "basic.to_delayed_action_delay_milliseconds": [
      "settings.complex_modifications.parameters.basic.to_delayed_action_delay_milliseconds"
    ],
    "basic.to_if_alone_timeout_milliseconds": [
      "settings.complex_modifications.parameters.basic.to_if_alone_timeout_milliseconds"
    ],
    "basic.to_if_held_down_threshold_milliseconds": [
      "settings.complex_modifications.parameters.basic.to_if_held_down_threshold_milliseconds"
    ],
    "check_for_updates": [
      "settings.update.automatic"
    ],
    "configured_devices_count": [
      "settings.changed_settings.setting.configured_devices_count"
    ],
    "delay_milliseconds_before_open_device": [
      "setting.delay_milliseconds_before_open_device"
    ],
    "delay_milliseconds_before_sleep_shortcut": [
      "setting.delay_milliseconds_before_sleep_shortcut"
    ],
    "enable_cgeventtap_fallback": [
      "setting.enable_cgeventtap_fallback"
    ],
    "enable_multitouch_extension": [
      "setting.enable_multitouch_extension"
    ],
    "enable_notification_window": [
      "settings.changed_settings.notification_window_prefix",
      "setting.enable_notification_window",
    ],
    "enabled_rules_count": [
      "settings.changed_settings.setting.enabled_rules_count"
    ],
    "external_editor_path": [
      "settings.changed_settings.setting.external_editor_path"
    ],
    "filter_useless_events_from_specific_devices": [
      "setting.filter_useless_events_from_specific_devices"
    ],
    "fn_function_keys_count": [
      "settings.changed_settings.setting.fn_function_keys_count"
    ],
    "ignore_pointing_device_events_by_default": [
      "settings.expert.modify_pointing_by_default"
    ],
    "indicate_sticky_modifier_keys_state": [
      "settings.changed_settings.notification_window_prefix",
      "setting.indicate_sticky_modifier_keys_state",
    ],
    "keyboard_type_v2": [
      "setting.keyboard_type_v2"
    ],
    "mouse_key_xy_scale": [
      "setting.mouse_key_xy_scale"
    ],
    "mouse_motion_to_scroll.speed": [
      "settings.complex_modifications.parameters.mouse_motion_to_scroll.speed"
    ],
    "notification_window_colors.dark.background_color": [
      "settings.changed_settings.notification_window_prefix",
      "section.dark",
      "setting.background_color",
    ],
    "notification_window_colors.dark.text_color": [
      "settings.changed_settings.notification_window_prefix",
      "section.dark",
      "setting.text_color",
    ],
    "notification_window_colors.light.background_color": [
      "settings.changed_settings.notification_window_prefix",
      "section.light",
      "setting.background_color",
    ],
    "notification_window_colors.light.text_color": [
      "settings.changed_settings.notification_window_prefix",
      "section.light",
      "setting.text_color",
    ],
    "notification_window_font_size": [
      "settings.changed_settings.notification_window_prefix",
      "setting.notification_window_font_size",
    ],
    "notification_window_position": [
      "settings.changed_settings.notification_window_prefix",
      "setting.notification_window_position",
    ],
    "notification_window_respect_screen_visible_frame": [
      "settings.changed_settings.notification_window_prefix",
      "setting.notification_window_respect_screen_visible_frame",
    ],
    "notification_window_show_icon": [
      "settings.changed_settings.notification_window_prefix",
      "setting.notification_window_show_icon",
    ],
    "reorder_same_timestamp_input_events_to_prioritize_modifiers": [
      "setting.reorder_same_timestamp_input_events_to_prioritize_modifiers"
    ],
    "rules_count": [
      "settings.changed_settings.setting.rules_count"
    ],
    "show_additional_menu_items": [
      "setting.show_additional_menu_items"
    ],
    "show_in_menu_bar": [
      "setting.show_in_menu_bar"
    ],
    "show_profile_name_in_menu_bar": [
      "setting.show_profile_name_in_menu_bar"
    ],
    "show_quit_confirmation_menu": [
      "setting.show_quit_confirmation_menu"
    ],
    "simple_modifications_count": [
      "settings.changed_settings.setting.simple_modifications_count"
    ],
    "text_color": [
      "setting.text_color"
    ],
    "ui_language": [
      "settings.changed_settings.setting.ui_language"
    ],
    "unsafe_ui": [
      "setting.unsafe_ui"
    ],
  ]

  let sections: [Section]

  init(json: String, locale: Locale, catalog: LocalizationCatalog = AppLanguage.catalog) throws {
    let object = try JSONSerialization.jsonObject(with: Data(json.utf8))
    guard let root = object as? [String: Any] else {
      throw CocoaError(.propertyListReadCorrupt)
    }
    func text(_ key: String) -> String {
      AppLanguage.text(key, locale: locale, catalog: catalog)
    }
    func label(_ key: String, localizationKey: String?) -> String {
      guard let localizationKey else { return key }
      let translated = text(localizationKey)
      return translated == localizationKey ? key : translated
    }
    func valueText(_ value: Any, key: String? = nil) -> String {
      if let number = value as? NSNumber {
        if CFGetTypeID(number) == CFBooleanGetTypeID() {
          // The UI describes modifying events, the inverse of the stored ignore flag.
          let enabled =
            (key == "ignore_pointing_device_events_by_default")
            ? !number.boolValue
            : number.boolValue
          return text(enabled ? "value.on" : "value.off")
        }
        return number.stringValue
      }
      if let string = value as? String { return string }
      if value is NSNull { return "null" }
      return String(describing: value)
    }

    var sections: [Section] = []
    func walk(_ value: Any, path: String, titles: [String]) {
      if var dictionary = value as? [String: Any] {
        // Display color settings alongside the other global notification settings.
        // Dotted keys preserve the original configuration paths in row IDs.
        if path == "global",
          let colors = dictionary["notification_window_colors"] as? [String: [String: Any]]
        {
          dictionary.removeValue(forKey: "notification_window_colors")
          for (theme, settings) in colors {
            for (key, value) in settings {
              dictionary["notification_window_colors.\(theme).\(key)"] = value
            }
          }
        }
        var rows: [Row] = []
        var children: [(String, Any)] = []
        for key in dictionary.keys.sorted() {
          guard let child = dictionary[key] else { continue }
          if child is [String: Any] || child is [Any] {
            children.append((key, child))
          } else {
            let settingLabel =
              Self.settingLocalizationKeys[key]?.map {
                label(key, localizationKey: $0)
              }.joined(separator: " / ") ?? key
            rows.append(
              Row(
                id: path.isEmpty ? key : path + "." + key,
                label: settingLabel,
                value: valueText(child, key: key)))
          }
        }
        rows.sort {
          let order = $0.label.compare(
            $1.label, options: [.caseInsensitive, .numeric], locale: locale)
          return order == .orderedSame ? $0.id < $1.id : order == .orderedAscending
        }
        if !rows.isEmpty {
          sections.append(Section(id: path, title: titles.joined(separator: " / "), rows: rows))
        }
        for (key, child) in children {
          walk(
            child, path: path.isEmpty ? key : path + "." + key,
            titles: titles + [label(key, localizationKey: Self.sectionLocalizationKeys[key])])
        }
      } else if let array = value as? [Any] {
        for (index, child) in array.enumerated() {
          walk(child, path: "\(path)[\(index)]", titles: titles + [String(index + 1)])
        }
      } else {
        sections.append(
          Section(
            id: path, title: titles.joined(separator: " / "),
            rows: [Row(id: path, label: titles.last ?? path, value: valueText(value))]))
      }
    }
    walk(root, path: "", titles: [])
    self.sections = sections
  }

  func text(version: String, systemVersion: String) -> String {
    var lines = ["Karabiner-Elements: \(version)", "macOS: \(systemVersion)"]
    for section in sections {
      lines.append("")
      lines.append(section.title)
      for row in section.rows {
        // Preserve full keys (including device indexes) regardless of display language.
        lines.append("\(row.label): \(row.value) [\(row.id)]")
      }
    }
    return lines.joined(separator: "\n")
  }

  // Project supported snapshot fields into a compact report before comparing them.
  // Keep this independent of locale so language changes only rebuild the display.
  static func makeJSON(snapshotData: Data) throws -> String {
    let snapshot = try object(JSONSerialization.jsonObject(with: snapshotData))
    let defaults = try object(snapshot["default_configuration"])
    let current = try summarize(snapshot, defaults: defaults)
    let baseline = try summarize(defaults, defaults: defaults)
    var result = SettingsJSONDiff.make(from: baseline, to: current) as? [String: Any] ?? [:]

    // Keep the enabled count, including zero, alongside a nonzero rule count.
    if var profile = result["selected_profile"] as? [String: Any],
      var complex = profile["complex_modifications"] as? [String: Any],
      complex["rules_count"] != nil
    {
      let currentProfile = try object(current["selected_profile"])
      complex["enabled_rules_count"] = try object(currentProfile["complex_modifications"])[
        "enabled_rules_count"]
      profile["complex_modifications"] = complex
      result["selected_profile"] = profile
    }
    let data = try JSONSerialization.data(
      withJSONObject: result, options: [.sortedKeys, .prettyPrinted])
    return String(decoding: data, as: UTF8.self)
  }

  private static func object(_ value: Any?) throws -> [String: Any] {
    guard let value = value as? [String: Any] else {
      throw CocoaError(.propertyListReadCorrupt)
    }
    return value
  }

  private static func rows(_ value: Any?) throws -> [[String: Any]] {
    guard let value = value as? [[String: Any]] else {
      throw CocoaError(.propertyListReadCorrupt)
    }
    return value
  }

  private static func mappings(_ value: Any?) throws -> [String: Any] {
    var result: [String: Any] = [:]
    for row in try rows(value) {
      // Ignore incomplete editor rows and indexes. The first complete mapping for
      // each source wins, matching the configuration's mapping serialization.
      guard let fromString = row["from_json_string"] as? String,
        let toString = row["to_json_string"] as? String,
        let from = try? JSONSerialization.jsonObject(with: Data(fromString.utf8)) as? [String: Any],
        !from.isEmpty,
        let to = try? JSONSerialization.jsonObject(with: Data(toString.utf8)) as? [Any],
        !to.isEmpty
      else { continue }
      let key = String(
        decoding: try JSONSerialization.data(withJSONObject: from, options: .sortedKeys),
        as: UTF8.self)
      if result[key] == nil { result[key] = to }
    }
    return result
  }

  private static func changedMappingsCount(_ value: Any?, defaults: Any?) throws -> Int {
    let current = try mappings(value)
    let baseline = try mappings(defaults)
    // Mapping removal must count too; SettingsJSONDiff intentionally omits removed keys.
    return Set(current.keys).union(baseline.keys).filter {
      !equal(current[$0], baseline[$0])
    }.count
  }

  private static func equal(_ lhs: Any?, _ rhs: Any?) -> Bool {
    guard let lhs = lhs as? NSObject, let rhs else { return false }
    return lhs.isEqual(rhs)
  }

  private static func deviceValues(_ value: Any?) throws -> [String: Any] {
    var device = try object(value)
    for key in ["simple_modifications", "fn_function_keys"] {
      device[key] = try mappings(device[key])
    }
    return device
  }

  static func summarize(_ snapshot: [String: Any], defaults: [String: Any]) throws
    -> [String: Any]
  {
    var profile = try object(snapshot["selected_profile"])
    let defaultProfile = try object(defaults["selected_profile"])
    for key in ["simple_modifications", "fn_function_keys"] {
      profile[key + "_count"] = try changedMappingsCount(
        profile[key], defaults: defaultProfile[key])
      profile.removeValue(forKey: key)
    }

    let devices = try object(profile["devices"])
    let defaultDevices = try object(defaultProfile["devices"])
    profile["configured_devices_count"] = try devices.filter { identifiers, device in
      try !equal(deviceValues(device), deviceValues(defaultDevices[identifiers]))
    }.count
    profile.removeValue(forKey: "devices")

    var complex = try object(profile["complex_modifications"])
    let rules = try rows(complex["rules"])
    complex["rules_count"] = rules.count
    complex["enabled_rules_count"] = rules.filter { $0["enabled"] as? Bool == true }.count
    complex.removeValue(forKey: "rules")

    // Keep actual karabiner.json parameter names in the report and copied text.
    var parameters = try object(complex["parameters"])
    for (snapshotKey, configurationKey) in [
      "basic_simultaneous_threshold_milliseconds": "basic.simultaneous_threshold_milliseconds",
      "basic_to_if_alone_timeout_milliseconds": "basic.to_if_alone_timeout_milliseconds",
      "basic_to_if_held_down_threshold_milliseconds":
        "basic.to_if_held_down_threshold_milliseconds",
      "basic_to_delayed_action_delay_milliseconds": "basic.to_delayed_action_delay_milliseconds",
      "mouse_motion_to_scroll_speed": "mouse_motion_to_scroll.speed",
    ] {
      parameters[configurationKey] = parameters.removeValue(forKey: snapshotKey)
    }
    complex["parameters"] = parameters
    profile["complex_modifications"] = complex

    return [
      "global": try object(snapshot["global_configuration"]),
      "machine_specific": try object(snapshot["machine_specific"]),
      "selected_profile": profile,
    ]
  }
}
