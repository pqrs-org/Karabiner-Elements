import Foundation

@main
@MainActor
struct ChangedSettingsSnapshotProcessor {
  static func main() throws {
    let input = FileHandle.standardInput.readDataToEndOfFile()
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase
    var configuration = try decoder.decode(SettingsConfiguration.self, from: input)
    configuration.changedSettingsJson = try ChangedSettings.makeJSON(snapshotData: input)

    // Exercise formatting from the same real snapshot, using a small isolated catalog.
    let catalog = try LocalizationCatalog(data: Data(#"{"value.on":{"en":"On"}}"#.utf8))
    let changes = try ChangedSettings(
      json: configuration.changedSettingsJson, locale: Locale(identifier: "en"), catalog: catalog)
    precondition(
      changes.text(version: "test", systemVersion: "test").contains("Karabiner-Elements: test"))

    // Report/default snapshot fields must not leak into configuration updates.
    let encoder = JSONEncoder()
    encoder.keyEncodingStrategy = .convertToSnakeCase
    // UI defaults must come from the nested C++ snapshot, even when current values differ.
    let rawSnapshot = try JSONSerialization.jsonObject(with: input) as! [String: Any]
    let rawDefaults = rawSnapshot["default_configuration"] as! [String: Any]
    // Compare every reportable default field (including summary counts and colors)
    // against the registered labels. Render all defaults so unchanged fields are checked too.
    let labelKeys = Set(ChangedSettings.settingLocalizationKeys.values.flatMap { $0 })
    let labelCatalog = try LocalizationCatalog(
      data: JSONSerialization.data(
        withJSONObject: Dictionary(
          uniqueKeysWithValues: labelKeys.map { ($0, ["en": "Registered"]) })))
    func missingLabels(_ summary: [String: Any]) throws -> [String] {
      let data = try JSONSerialization.data(withJSONObject: summary)
      let report = try ChangedSettings(
        json: String(decoding: data, as: UTF8.self), locale: Locale(identifier: "en"),
        catalog: labelCatalog)
      return report.sections.flatMap(\.rows).filter {
        !$0.label.components(separatedBy: " / ").allSatisfy { $0 == "Registered" }
      }.map(\.id)
    }
    let defaultSummary = try ChangedSettings.summarize(rawDefaults, defaults: rawDefaults)
    let missing = try missingLabels(defaultSummary)
    precondition(
      missing.isEmpty, "Missing settingLocalizationKeys entries: \(missing.joined(separator: ", "))"
    )

    // Prove a new, unregistered default field is detected, including nested color fields.
    var unregistered = defaultSummary
    var global = unregistered["global"] as! [String: Any]
    global["future_setting"] = true
    global["notification_window_colors"] = ["light": ["future_color": "#000000"]]
    unregistered["global"] = global
    let unregisteredKeys = try missingLabels(unregistered)
    precondition(
      Set(unregisteredKeys) == [
        "global.future_setting", "global.notification_window_colors.light.future_color",
      ])

    let rawDefaultProfile = rawDefaults["selected_profile"] as! [String: Any]
    let rawDefaultComplex = rawDefaultProfile["complex_modifications"] as! [String: Any]
    let defaults = configuration.defaultConfiguration
    func checkDefaults<T: Encodable>(_ value: T, _ raw: Any) throws {
      let json = try JSONSerialization.jsonObject(with: encoder.encode(value)) as! [String: Any]
      precondition(NSDictionary(dictionary: json).isEqual(to: raw as! [String: Any]))
    }
    try checkDefaults(defaults.globalConfiguration, rawDefaults["global_configuration"]!)
    try checkDefaults(defaults.machineSpecific, rawDefaults["machine_specific"]!)
    try checkDefaults(defaults.selectedProfile.parameters, rawDefaultProfile["parameters"]!)
    try checkDefaults(
      defaults.selectedProfile.complexModifications.parameters, rawDefaultComplex["parameters"]!)
    try checkDefaults(
      defaults.selectedProfile.virtualHidKeyboard, rawDefaultProfile["virtual_hid_keyboard"]!)
    precondition(
      defaults.selectedProfile.modifyPointingDeviceEventsByDefault
        == !(rawDefaultProfile["ignore_pointing_device_events_by_default"] as! Bool))

    let oldData = try encoder.encode(SettingsConfigurationUpdate(configuration))
    let oldJSON = try JSONSerialization.jsonObject(with: oldData)
    precondition((oldJSON as! [String: Any])["default_configuration"] == nil)
    let language = configuration.globalConfiguration.uiLanguage == "ja" ? "en" : "ja"
    configuration.globalConfiguration.uiLanguage = language
    try checkDefaults(
      configuration.defaultConfiguration.globalConfiguration, rawDefaults["global_configuration"]!)
    let newData = try encoder.encode(SettingsConfigurationUpdate(configuration))
    let newJSON = try JSONSerialization.jsonObject(with: newData)
    let patch = SettingsJSONDiff.make(from: oldJSON, to: newJSON) as! [String: Any]
    precondition(
      NSDictionary(dictionary: patch).isEqual(to: [
        "global_configuration": ["ui_language": language]
      ]))

    FileHandle.standardOutput.write(Data(configuration.changedSettingsJson.utf8))
  }
}
