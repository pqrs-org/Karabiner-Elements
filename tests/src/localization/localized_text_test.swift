import Foundation

@main
@MainActor
struct LocalizedTextTests {
  static func main() throws {
    let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    defer { try? FileManager.default.removeItem(at: directory) }
    try
      #"{"label":{"en":"Feature","ja":"機能"},"default":{"en":"(Default: {value})","ja":"（デフォルト：{value}）"},"fallback":{"en":"English only"}}"#
      .write(to: directory.appendingPathComponent("test.json"), atomically: true, encoding: .utf8)
    let catalog = try LocalizationCatalog(directory: directory)
    let en = Locale(identifier: "en")
    let ja = Locale(identifier: "ja")

    let combined = AppLocalizedText([
      "label",
      .init("default", arguments: ["value": "*literal*"]),
    ])
    precondition(
      combined.localizedString(locale: en, catalog: catalog) == "Feature(Default: *literal*)")
    precondition(combined.localizedString(locale: ja, catalog: catalog) == "機能（デフォルト：*literal*）")
    precondition(
      combined.localizedString(locale: en, catalog: catalog) == "Feature(Default: *literal*)")
    precondition(
      AppLocalizedText(["label", "\n", "fallback"])
        .localizedString(locale: ja, catalog: catalog) == "機能\nEnglish only")
    precondition(
      AppLocalizedText(["label", "label"])
        .localizedString(locale: en, catalog: catalog) == "FeatureFeature")
    precondition(
      AppLocalizedText(["label", " ", "fallback", "\n", "label"])
        .localizedString(locale: en, catalog: catalog) == "Feature English only\nFeature")
    precondition(AppLocalizedText([]).localizedString(locale: en, catalog: catalog).isEmpty)
    precondition(
      AppLocalizedText("default", arguments: ["value": "10"])
        .localizedString(locale: en, catalog: catalog) == "(Default: 10)")

    let constrained = AppLocalizedConstrainedText([
      "label", " ", .init("default", arguments: ["value": "*literal*"]), "\n", "fallback",
    ])
    precondition(
      constrained.localizedString(locale: en, catalog: catalog)
        == "Feature (Default: *literal*)\nEnglish only")
    precondition(
      constrained.localizedString(locale: ja, catalog: catalog)
        == "機能 （デフォルト：*literal*）\nEnglish only")
    precondition(
      AppLocalizedConstrainedText("default", arguments: ["value": "10"])
        .localizedString(locale: ja, catalog: catalog) == "（デフォルト：10）")
    precondition(
      AppLocalizedConstrainedText([]).localizedString(locale: en, catalog: catalog).isEmpty)

    print(
      "Default and constrained localized text: arrays, arguments, literal spaces/newlines, fallback and locale switching passed"
    )
  }
}
