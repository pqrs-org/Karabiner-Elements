import Foundation

@main
@MainActor
struct LocalizationTests {
  static func main() throws {
    if CommandLine.arguments[1] == "--observe-auto-reload" {
      let source = URL(fileURLWithPath: CommandLine.arguments[2])
      let ready = URL(fileURLWithPath: CommandLine.arguments[3])
      let store = AppLocalization(source: source)
      let locale = Locale(identifier: "ja")
      precondition(
        AppLanguage.text("menu_bar_extra.settings", locale: locale, catalog: store.catalog) == "設定…"
      )
      try Data().write(to: ready)
      let deadline = Date().addingTimeInterval(10)
      while Date() < deadline {
        RunLoop.current.run(until: Date().addingTimeInterval(0.01))
        if AppLanguage.text("menu_bar_extra.settings", locale: locale, catalog: store.catalog)
          == "監視で更新…"
        {
          return
        }
      }
      preconditionFailure("Cross-process localization reload timed out")
    }
    let source = URL(fileURLWithPath: CommandLine.arguments[1])
    let catalog = try LocalizationCatalog(data: Data(contentsOf: source))
    precondition(AppLanguage.availableLanguages(catalog: catalog) == ["en", "ja"])
    let en = AppLanguage.locale(for: "en", catalog: catalog)
    let ja = AppLanguage.locale(for: "ja", catalog: catalog)
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: ["ja-JP"], catalog: catalog).identifier
        == "ja")
    precondition(
      AppLanguage.locale(for: "auto", preferredLanguages: ["fr-FR", "en-US"], catalog: catalog)
        .identifier == "en")
    precondition(
      AppLanguage.locale(for: "invalid", preferredLanguages: ["ja"], catalog: catalog).identifier
        == "en")
    precondition(AppLanguage.text("menu_bar_extra.settings", locale: ja, catalog: catalog) == "設定…")
    precondition(
      AppLanguage.text("menu_bar_extra.settings", locale: en, catalog: catalog) == "Settings…")

    let directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    defer { try? FileManager.default.removeItem(at: directory) }
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    let extendedURL = directory.appendingPathComponent("localizations.json")
    var strings = catalog.strings
    // A language can appear under any key, and need not have every translation.
    strings["test.literal"] = ["en": "100% %@ \n \"quoted\"", "fr": "Texte"]
    for (language, translation) in [("pt-BR", "Ajustes…"), ("zh-Hant", "設定（繁體）")] {
      strings["menu_bar_extra.settings"]![language] = translation
    }
    try write(strings, to: extendedURL)
    let extended = try LocalizationCatalog(data: Data(contentsOf: extendedURL))
    precondition(
      AppLanguage.availableLanguages(catalog: extended) == ["en", "fr", "ja", "pt-BR", "zh-Hant"])
    for (language, translation) in [("pt-BR", "Ajustes…"), ("zh-Hant", "設定（繁體）")] {
      let locale = AppLanguage.locale(for: language, catalog: extended)
      precondition(locale.identifier == language)
      precondition(
        AppLanguage.text("menu_bar_extra.settings", locale: locale, catalog: extended)
          == translation)
      precondition(
        AppLanguage.text("setting.enable_cgeventtap_fallback", locale: locale, catalog: extended)
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
    // The language picker uses native display names, including region/script variants.
    precondition(AppLanguage.displayName(for: "fr") == "français")
    precondition(AppLanguage.displayName(for: "ja") == "日本語")
    precondition(AppLanguage.displayName(for: "pt-BR") == "português (Brasil)")
    precondition(AppLanguage.displayName(for: "pt-PT") == "português (Portugal)")
    precondition(AppLanguage.displayName(for: "zh-Hant") == "繁體中文")

    // Both processes independently monitor the same test file.
    let watchedSource = directory.appendingPathComponent("watched.json")
    try write(catalog.strings, to: watchedSource)
    let watching = AppLocalization(source: watchedSource)
    let ready = directory.appendingPathComponent(UUID().uuidString)
    let observer = Process()
    observer.executableURL = URL(fileURLWithPath: CommandLine.arguments[0])
    observer.arguments = [
      "--observe-auto-reload", watchedSource.path,
      ready.path,
    ]
    try observer.run()
    defer { if observer.isRunning { observer.terminate() } }
    let readyDeadline = Date().addingTimeInterval(10)
    while !FileManager.default.fileExists(atPath: ready.path) && Date() < readyDeadline {
      RunLoop.current.run(until: Date().addingTimeInterval(0.01))
    }
    precondition(FileManager.default.fileExists(atPath: ready.path))
    var updatedStrings = catalog.strings
    updatedStrings["menu_bar_extra.settings"]!["ja"] = "監視で更新…"
    try write(updatedStrings, to: watchedSource)
    let deliveryDeadline = Date().addingTimeInterval(10)
    while observer.isRunning && Date() < deliveryDeadline {
      RunLoop.current.run(until: Date().addingTimeInterval(0.01))
    }
    precondition(!observer.isRunning)
    observer.waitUntilExit()
    precondition(observer.terminationStatus == 0)
    waitUntil {
      AppLanguage.text("menu_bar_extra.settings", locale: ja, catalog: watching.catalog) == "監視で更新…"
    }

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
      "invalid JSON", "[]", "{}", "null",
      #"{"key":"value"}"#, #"{"key":{"en":42}}"#, #"{"key":{"en":true}}"#,
      #"{"key":{"en":null}}"#, #"{"key":{"ja":"日本語"}}"#,
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
    try FileManager.default.removeItem(at: extendedURL)
    do {
      try reloading.reload()
      preconditionFailure("Missing JSON accepted")
    } catch {}
    precondition(reloading.catalog == good)
    // A process started before the JSON was installed can recover without restart.
    let initiallyMissing = AppLocalization(
      source: extendedURL, automaticallyReload: false)
    precondition(AppLanguage.availableLanguages(catalog: initiallyMissing.catalog) == ["en"])
    precondition(
      AppLanguage.text("unknown.key", locale: ja, catalog: initiallyMissing.catalog)
        == "unknown.key")
    try write(catalog.strings, to: extendedURL)
    try initiallyMissing.reload()
    try reloading.reload()
    precondition(initiallyMissing.catalog == catalog)
    precondition(reloading.catalog == catalog)
    // Observe in-place writes and atomic replacement, and continue after invalid JSON.
    var automatic: AppLocalization? = AppLocalization(
      source: extendedURL)
    var autoStrings = catalog.strings
    autoStrings["menu_bar_extra.settings"]!["ja"] = "自動更新…"
    autoStrings["menu_bar_extra.settings"]!["de"] = "Einstellungen…"
    try JSONEncoder().encode(autoStrings).write(to: extendedURL)
    waitUntil { automatic!.catalog.strings == autoStrings }
    precondition(automatic!.catalog.languages.contains("de"))
    let valid = automatic!.catalog
    try Data("invalid JSON".utf8).write(to: extendedURL)
    RunLoop.current.run(until: Date().addingTimeInterval(0.3))
    precondition(automatic!.catalog == valid)
    autoStrings["menu_bar_extra.settings"]!.removeValue(forKey: "de")
    try write(autoStrings, to: extendedURL)
    waitUntil { automatic!.catalog.strings == autoStrings }
    precondition(!automatic!.catalog.languages.contains("de"))
    // A second replacement verifies that monitoring follows the new file.
    try write(catalog.strings, to: extendedURL)
    waitUntil { automatic!.catalog == catalog }
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
    try JSONEncoder().encode(strings).write(to: url, options: .atomic)
  }
}
