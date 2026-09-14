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
    let oldData = try encoder.encode(SettingsConfigurationUpdate(configuration))
    let oldJSON = try JSONSerialization.jsonObject(with: oldData)
    let language = configuration.globalConfiguration.uiLanguage == "ja" ? "en" : "ja"
    configuration.globalConfiguration.uiLanguage = language
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
