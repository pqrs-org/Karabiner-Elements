import Combine
import Darwin
import Foundation

struct LocalizationCatalog: Equatable, Sendable {
  let strings: [String: [String: String]]
  let languages: [String]

  static let empty = LocalizationCatalog()

  private init() {
    strings = [:]
    languages = ["en"]
  }

  init(file: URL) throws {
    try self.init(data: Data(contentsOf: file))
  }

  init(data: Data) throws {
    let strings = try JSONDecoder().decode([String: [String: String]].self, from: data)
    guard !strings.isEmpty else { throw CocoaError(.fileReadCorruptFile) }

    var languages: Set<String> = []
    for (key, translations) in strings {
      // English is the fallback for every key; additional languages may be partial.
      guard !key.isEmpty, translations["en"] != nil else {
        throw CocoaError(.fileReadCorruptFile)
      }

      for language in translations.keys {
        guard language != "auto",
          language.range(of: "^[a-z]{2,8}(-[A-Za-z0-9]{1,8})*$", options: .regularExpression) != nil
        else { throw CocoaError(.fileReadCorruptFile) }

        languages.insert(language)
      }
    }

    self.strings = strings
    self.languages = languages.sorted()
  }
}

@MainActor
final class AppLocalization: ObservableObject {
  private static let defaultSource = URL(
    fileURLWithPath:
      "/Library/Application Support/org.pqrs/Karabiner-Elements/localizations.json")

  private static let bundledSource = URL(
    fileURLWithPath:
      "/Applications/Karabiner-Elements.app/Contents/Resources/localizations.json")

  private static let currentVersion =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String

  static let shared = AppLocalization(
    fallbackSource: bundledSource,
    expectedSourceVersion: currentVersion)

  @Published private(set) var catalog = LocalizationCatalog.empty

  private var fileMonitors: [DispatchSourceFileSystemObject] = []
  private var reloadTask: Task<Void, Never>?
  private let automaticallyReload: Bool
  private let source: URL
  private let fallbackSource: URL?
  private let sourceVersionFile: URL
  private let expectedSourceVersion: String?

  init(
    source: URL = defaultSource,
    fallbackSource: URL? = nil,
    expectedSourceVersion: String? = nil,
    automaticallyReload: Bool = true
  ) {
    self.source = source
    self.fallbackSource = fallbackSource
    sourceVersionFile = source.deletingLastPathComponent()
      .appendingPathComponent("localizations.source-version")
    self.expectedSourceVersion = expectedSourceVersion
    self.automaticallyReload = automaticallyReload

    do {
      try reload()
    } catch {
      NSLog("Unable to load localizations: %@", error.localizedDescription)
    }
  }

  deinit {
    fileMonitors.forEach { $0.cancel() }
    reloadTask?.cancel()
  }

  func reload() throws {
    // Watch before reading so a subsequent write can recover from missing or invalid JSON.
    if automaticallyReload {
      startMonitoring()
    }

    let updated: LocalizationCatalog
    if !runtimeSourceVersionMatches, let fallbackSource {
      updated = try LocalizationCatalog(file: fallbackSource)
    } else {
      do {
        updated = try LocalizationCatalog(file: source)
      } catch {
        guard catalog == .empty, let fallbackSource else { throw error }
        updated = try LocalizationCatalog(file: fallbackSource)
      }
    }

    if updated != catalog {
      catalog = updated
    }
  }

  private var runtimeSourceVersionMatches: Bool {
    guard let expectedSourceVersion else { return true }
    guard let data = try? Data(contentsOf: sourceVersionFile),
      let sourceVersion = String(data: data, encoding: .utf8)
    else { return false }

    return sourceVersion == expectedSourceVersion
  }

  private func startMonitoring() {
    fileMonitors.forEach { $0.cancel() }
    fileMonitors.removeAll()

    // Watch the file for in-place writes and its parent for creation or replacement.
    for url in [
      source,  // /Library/Application Support/org.pqrs/Karabiner-Elements/localizations.json
      source.deletingLastPathComponent(),  // /Library/Application Support/org.pqrs/Karabiner-Elements
    ] {
      let descriptor = open(url.path, O_EVTONLY | O_CLOEXEC)
      guard descriptor >= 0 else { continue }
      let monitor = DispatchSource.makeFileSystemObjectSource(
        fileDescriptor: descriptor, eventMask: [.write, .rename, .delete], queue: .main)
      monitor.setCancelHandler { close(descriptor) }
      monitor.setEventHandler { [weak self] in
        Task { @MainActor [weak self] in
          self?.scheduleReload()
        }
      }
      fileMonitors.append(monitor)
      monitor.activate()
    }
  }

  private func scheduleReload() {
    reloadTask?.cancel()
    reloadTask = Task { @MainActor [weak self] in
      // Coalesce consecutive writes so a save normally produces one reload.
      do {
        try await Task.sleep(for: .milliseconds(100))
      } catch { return }
      do {
        try self?.reload()
      } catch {
        NSLog("Unable to automatically reload localizations: %@", error.localizedDescription)
      }
    }
  }
}
