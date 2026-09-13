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
final class AppLocalization: NSObject, ObservableObject {
  static let shared = AppLocalization()

  @Published private(set) var catalog = LocalizationCatalog.empty

  private var fileMonitor: DispatchSourceFileSystemObject?
  private var reloadTask: Task<Void, Never>?
  private let automaticallyReload: Bool
  private let source: URL
  private let sender = UUID().uuidString
  private let reloadNotification: Notification.Name

  init(
    source: URL = URL(
      fileURLWithPath:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/localizations.json"),
    observeNotifications: Bool = true,
    automaticallyReload: Bool = true,
    reloadNotification: Notification.Name = Notification.Name(
      "org.pqrs.Karabiner-Elements.localizations.reload")
  ) {
    self.source = source
    self.automaticallyReload = automaticallyReload
    self.reloadNotification = reloadNotification
    super.init()
    do {
      try reload()
    } catch {
      NSLog("Unable to load localizations: %@", error.localizedDescription)
    }
    if observeNotifications {
      DistributedNotificationCenter.default().addObserver(
        self, selector: #selector(receiveReload(_:)), name: reloadNotification,
        object: nil, suspensionBehavior: .deliverImmediately)
    }
  }

  deinit {
    fileMonitor?.cancel()
    reloadTask?.cancel()
    DistributedNotificationCenter.default().removeObserver(self)
  }

  func reload() throws {
    // Reopen the path to follow an editor/installer replacing the file. If it is
    // absent, a later manual reload can start monitoring again.
    if automaticallyReload { startMonitoring() }
    // Decode and validate the entire file before replacing the current translations.
    let updated = try LocalizationCatalog(data: Data(contentsOf: source))
    if updated != catalog { catalog = updated }
  }

  private func startMonitoring() {
    fileMonitor?.cancel()
    fileMonitor = nil
    let descriptor = open(source.path, O_EVTONLY | O_CLOEXEC)
    guard descriptor >= 0 else { return }
    let monitor = DispatchSource.makeFileSystemObjectSource(
      fileDescriptor: descriptor, eventMask: [.write, .rename, .delete], queue: .main)
    monitor.setCancelHandler { close(descriptor) }
    monitor.setEventHandler { [weak self] in
      Task { @MainActor [weak self] in
        self?.scheduleReload()
      }
    }
    fileMonitor = monitor
    monitor.activate()
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

  func reloadAndNotify() throws {
    try reload()
    DistributedNotificationCenter.default().postNotificationName(
      reloadNotification, object: sender, userInfo: nil, deliverImmediately: true)
  }

  // Distributed notifications arrive on the main thread. The notification only
  // requests a reload: never accept a resource path from another process.
  @objc private func receiveReload(_ notification: Notification) {
    guard notification.object as? String != sender else { return }
    do {
      try reload()
    } catch {
      NSLog("Unable to reload localizations: %@", error.localizedDescription)
    }
  }
}
