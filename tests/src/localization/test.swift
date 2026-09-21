import Foundation

@main
@MainActor
struct LocalizationTests {
  static func main() throws {
    let source = URL(fileURLWithPath: CommandLine.arguments[1])
    let catalog = try LocalizationCatalog(file: source)
    precondition(AppLanguage.availableLanguages(catalog: catalog) == ["en", "ja"])
    let en = AppLanguage.locale(for: "en", catalog: catalog)
    let ja = AppLanguage.locale(for: "ja", catalog: catalog)
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: ["ja-JP"], catalog: catalog).identifier
        == "ja")
    precondition(
      AppLanguage.locale(for: "", preferredLanguages: ["ja-JP"], catalog: catalog).identifier
        == "ja")
    precondition(
      AppLanguage.locale(for: "ja", preferredLanguages: ["en-US"], catalog: catalog).identifier
        == "ja")
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: [], catalog: catalog).identifier == "en")
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: ["fr-FR", "en-US"], catalog: catalog)
        .identifier == "en")
    precondition(
      AppLanguage.locale(for: "invalid", preferredLanguages: ["ja"], catalog: catalog).identifier
        == "en")
    precondition(AppLanguage.text("menu_bar_extra.settings", locale: ja, catalog: catalog) == "設定…")
    precondition(
      AppLanguage.text("menu_bar_extra.settings", locale: en, catalog: catalog) == "Settings…")

    // Named placeholders can move with the translation. This synthetic catalog
    // tests substitution behavior independently of the application's wording.
    let templates = try LocalizationCatalog(
      data: Data(
        #"""
        {
            "test.arguments": {
                "en": "{name}: {count} ({name})",
                "fr": "{count} : {name} ({name})"
            },
            "test.fallback": {"en": "Value: {value}"}
        }
        """#.utf8))
    let arguments = ["name": "😀 %@ {count}", "count": "2"]
    // Inserted text is literal: percent signs and placeholder-like contents are
    // never interpreted again, including when a placeholder occurs twice.
    precondition(
      AppLanguage.text("test.arguments", locale: en, catalog: templates, arguments: arguments)
        == "😀 %@ {count}: 2 (😀 %@ {count})")
    precondition(
      AppLanguage.text(
        "test.arguments", locale: Locale(identifier: "fr"), catalog: templates, arguments: arguments
      )
        == "2 : 😀 %@ {count} (😀 %@ {count})")
    // English fallback still substitutes arguments for a partial language.
    precondition(
      AppLanguage.text(
        "test.fallback", locale: Locale(identifier: "fr"), catalog: templates,
        arguments: ["value": "100%"])
        == "Value: 100%")
    // Missing arguments remain visible instead of being silently discarded.
    precondition(
      AppLanguage.text("test.fallback", locale: en, catalog: templates) == "Value: {value}")

    let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    defer { try? FileManager.default.removeItem(at: directory) }
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    let extendedSource = directory.appendingPathComponent("extended", isDirectory: true)
    try FileManager.default.createDirectory(at: extendedSource, withIntermediateDirectories: true)
    let extendedURL = extendedSource.appendingPathComponent("general.json")
    var strings = catalog.strings
    // A language can appear under any key, and need not have every translation.
    strings["test.literal"] = ["en": "100% %@ \n \"quoted\"", "fr": "Texte"]
    for (language, translation) in [("pt-BR", "Ajustes…"), ("zh-Hant", "設定（繁體）")] {
      strings["menu_bar_extra.settings"]![language] = translation
    }
    try write(strings, to: extendedURL)
    let extended = try LocalizationCatalog(file: extendedURL)
    precondition(
      AppLanguage.availableLanguages(catalog: extended) == ["en", "fr", "ja", "pt-BR", "zh-Hant"])
    for (language, translation) in [("pt-BR", "Ajustes…"), ("zh-Hant", "設定（繁體）")] {
      let locale = AppLanguage.locale(for: language, catalog: extended)
      precondition(locale.identifier == language)
      precondition(
        AppLanguage.text("menu_bar_extra.settings", locale: locale, catalog: extended)
          == translation)
      precondition(
        AppLanguage.text(
          "settings.expert.enable_cgeventtap_fallback", locale: locale, catalog: extended)
          == "Enable CGEventTap fallback")
    }
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: ["pt-BR"], catalog: extended).identifier
        == "pt-BR")
    precondition(
      AppLanguage.text("test.literal", locale: ja, catalog: extended)
        == strings["test.literal"]!["en"])
    precondition(
      AppLanguage.text(
        "menu_bar_extra.settings", locale: Locale(identifier: "fr"), catalog: extended)
        == "Settings…")
    precondition(AppLanguage.text("unknown.key", locale: ja, catalog: extended) == "unknown.key")
    // Replacing the file only takes effect on reload, including the language list.
    let reloading = AppLocalization(
      source: extendedURL, automaticallyReload: false)
    precondition(
      AppLanguage.text("menu_bar_extra.settings", locale: ja, catalog: reloading.catalog) == "設定…")
    strings["menu_bar_extra.settings"]!["ja"] = "更新した設定…"
    strings["menu_bar_extra.settings"]!["de"] = "Einstellungen…"
    strings["test.literal"]!.removeValue(forKey: "fr")
    try write(strings, to: extendedURL)
    precondition(reloading.catalog == extended)
    try reloading.reload()
    precondition(
      AppLanguage.text("menu_bar_extra.settings", locale: ja, catalog: reloading.catalog)
        == "更新した設定…"
    )
    precondition(
      AppLanguage.availableLanguages(catalog: reloading.catalog) == [
        "de", "en", "ja", "pt-BR", "zh-Hant",
      ])
    let good = reloading.catalog
    // Malformed and structurally invalid JSON must preserve the previous catalog.
    for invalid in [
      "invalid JSON", "{}",
      #"{"key":{"en":42}}"#, #"{"key":{"en":["text"]}}"#,
      #"{"key":{"ja":"日本語"}}"#,
      #"{"":{"en":"empty key"}}"#, #"{"key":{"en":"English","":"invalid language"}}"#,
      #"{"key":{"en":"English","auto":"reserved language"}}"#,
      #"{"key":{"en":"English","pt_BR":"invalid language tag"}}"#,
    ] {
      try Data(invalid.utf8).write(to: extendedURL)
      do {
        try reloading.reload()
        preconditionFailure("Invalid translations accepted: \(invalid)")
      } catch {}
      precondition(reloading.catalog == good)
    }
    try write(catalog.strings, to: extendedURL)
    // Observe in-place writes and atomic replacement, and continue after invalid JSON.
    var automatic: AppLocalization? = AppLocalization(
      source: extendedURL)
    var autoStrings = catalog.strings
    autoStrings["menu_bar_extra.settings"]!["ja"] = "自動更新…"
    autoStrings["menu_bar_extra.settings"]!["de"] = "Einstellungen…"
    // Keep one writer open across a partial write and a failed reload.
    let data = try JSONEncoder().encode(autoStrings)
    let split = data.count / 2
    try data.prefix(split).write(to: extendedURL)
    let writer = try FileHandle(forWritingTo: extendedURL)
    defer { try? writer.close() }
    try writer.seekToEnd()
    RunLoop.current.run(until: Date().addingTimeInterval(0.3))
    precondition(automatic!.catalog == catalog)
    try writer.write(contentsOf: data.suffix(from: split))
    waitUntil { automatic!.catalog.strings == autoStrings }
    precondition(automatic!.catalog.languages.contains("de"))
    autoStrings["menu_bar_extra.settings"]!.removeValue(forKey: "de")
    try write(autoStrings, to: extendedURL)
    waitUntil { automatic!.catalog.strings == autoStrings }
    precondition(!automatic!.catalog.languages.contains("de"))
    // Replacement also works, and subsequent overwrites follow the new file.
    try JSONEncoder().encode(catalog.strings).write(to: extendedURL, options: .atomic)
    waitUntil { automatic!.catalog == catalog }
    try write(autoStrings, to: extendedURL)
    waitUntil { automatic!.catalog.strings == autoStrings }
    // Missing files recover automatically when the catalog is installed again.
    try FileManager.default.removeItem(at: extendedURL)
    RunLoop.current.run(until: Date().addingTimeInterval(0.3))
    precondition(automatic!.catalog.strings == autoStrings)
    let missing = AppLocalization(source: extendedURL)
    precondition(missing.catalog == .empty)
    try write(catalog.strings, to: extendedURL)
    waitUntil { automatic!.catalog == catalog && missing.catalog == catalog }

    weak let released = automatic
    automatic = nil
    precondition(released == nil)
    print("Localization tests passed")
  }

  private static func waitUntil(_ condition: () -> Bool) {
    let deadline = Date().addingTimeInterval(5)
    while !condition() && Date() < deadline {
      RunLoop.current.run(until: Date().addingTimeInterval(0.01))
    }
    precondition(condition(), "Automatic reload timed out")
  }

  private static func write(_ strings: [String: [String: String]], to url: URL) throws {
    try JSONEncoder().encode(strings).write(to: url)
  }
}
