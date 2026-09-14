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

  init(directory: URL) throws {
    try self.init(resources: Self.resourceURLs(in: directory))
  }

  fileprivate init(resources: [URL]) throws {
    var strings: [String: [String: String]] = [:]
    var languages: Set<String> = []
    for url in resources where url.pathExtension == "json" && !url.hasDirectoryPath {
      let catalog = try Self(data: Data(contentsOf: url))
      for (key, value) in catalog.strings {
        guard strings[key] == nil else {
          throw NSError(
            domain: "LocalizationCatalog", code: 1,
            userInfo: [
              NSLocalizedDescriptionKey: "Duplicate translation key: \(key) (\(url.path))"
            ])
        }
        strings[key] = value
      }
      languages.formUnion(catalog.languages)
    }
    guard !strings.isEmpty else { throw CocoaError(.fileReadCorruptFile) }
    self.strings = strings
    self.languages = languages.sorted()
  }

  // Include directories so additions/removals are observed, and files so in-place
  // writes are observed. Hidden files and symbolic links are not resources.
  fileprivate static func resourceURLs(in directory: URL) throws -> [URL] {
    let values = try directory.resourceValues(forKeys: [.isDirectoryKey, .isSymbolicLinkKey])
    guard values.isDirectory == true, values.isSymbolicLink != true else {
      throw CocoaError(.fileReadCorruptFile)
    }
    var result = [URL(fileURLWithPath: directory.path, isDirectory: true)]
    for url in try FileManager.default.contentsOfDirectory(
      at: directory,
      includingPropertiesForKeys: [.isDirectoryKey, .isRegularFileKey, .isSymbolicLinkKey],
      options: .skipsHiddenFiles
    ).sorted(by: { $0.path < $1.path }) {
      let values = try url.resourceValues(forKeys: [
        .isDirectoryKey, .isRegularFileKey, .isSymbolicLinkKey,
      ])
      if values.isSymbolicLink == true { continue }
      if values.isDirectory == true {
        result += try resourceURLs(in: url)
      } else if values.isRegularFile == true && url.pathExtension == "json" {
        result.append(url)
      }
    }
    return result
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
  static let shared = AppLocalization()

  @Published private(set) var catalog = LocalizationCatalog.empty

  private var fileMonitors: [DispatchSourceFileSystemObject] = []
  private var reloadTask: Task<Void, Never>?
  private let automaticallyReload: Bool
  private let source: URL

  init(
    source: URL = URL(
      fileURLWithPath:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/localizations"),
    automaticallyReload: Bool = true
  ) {
    self.source = source
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
    let resources = try LocalizationCatalog.resourceURLs(in: source)
    // Refresh watches even when parsing fails, so correcting a newly added invalid
    // file triggers another reload. Publish only after the entire catalog is valid.
    if automaticallyReload {
      startMonitoring(resources)
    }
    let updated = try LocalizationCatalog(resources: resources)
    if updated != catalog {
      catalog = updated
    }
  }

  private func startMonitoring(_ resources: [URL]) {
    fileMonitors.forEach { $0.cancel() }
    fileMonitors.removeAll()
    for url in resources {
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
