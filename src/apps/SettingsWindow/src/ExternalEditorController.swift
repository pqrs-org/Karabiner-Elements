import AppKit
import Foundation
import UniformTypeIdentifiers

private func externalEditorFileUpdatedCallback() {
  Task { @MainActor in
    ExternalEditorController.shared.handleFileUpdated()
  }
}

@MainActor
final class ExternalEditorController: ObservableObject {
  static let shared = ExternalEditorController()

  private typealias ReloadHandler = (String) -> Void

  private var fileURL: URL?
  private var monitoredFileURL: URL?
  private var lastSyncedText: String?
  private var onReloadHandler: ReloadHandler?

  private func chooseEditorURLAsync() async -> URL? {
    await withCheckedContinuation { continuation in
      let panel = NSOpenPanel()
      panel.allowsMultipleSelection = false
      panel.canChooseDirectories = false
      panel.canChooseFiles = true
      panel.allowedContentTypes = [UTType.application]
      panel.prompt = "Choose"
      panel.begin { response in
        guard response == .OK, let url = panel.url else {
          continuation.resume(returning: nil)
          return
        }
        continuation.resume(returning: url)
      }
    }
  }

  func chooseEditor() {
    Task { @MainActor in
      if let url = await chooseEditorURLAsync() {
        LibKrbn.Settings.shared.externalEditorPath = url.path
      }
    }
  }

  func reset() {
    stopMonitoring()
    removeTemporaryFile()
    fileURL = nil
    lastSyncedText = nil
    onReloadHandler = nil
  }

  func openTitle() -> String {
    if let url = externalEditorURL() {
      let name = FileManager.default.displayName(atPath: url.path)
      return "Open in \(name)"
    }
    return "Open in external editor"
  }

  func openEditor(
    with text: String,
    onError: @escaping (String) -> Void,
    onReload: @escaping (String) -> Void
  ) {
    guard let url = ensureFileURL(onError: onError) else {
      return
    }

    Task { [weak self, url, text] in
      let errorHandler = onError
      let reloadHandler = onReload

      let writeResult: Result<Void, Error> = await Task.detached(priority: .utility) {
        do {
          try text.write(to: url, atomically: true, encoding: .utf8)
          return .success(())
        } catch {
          return .failure(error)
        }
      }.value

      switch writeResult {
      case .success:
        self?.lastSyncedText = text
        self?.startMonitoring(url: url, onError: errorHandler, onReload: reloadHandler)
        if let editorURL = self?.externalEditorURL() {
          NSWorkspace.shared.open(
            [url],
            withApplicationAt: editorURL,
            configuration: NSWorkspace.OpenConfiguration(),
            completionHandler: nil)
        } else {
          NSWorkspace.shared.open(url)
        }
      case .failure(let error):
        errorHandler(error.localizedDescription)
      }
    }
  }

  func stopMonitoring() {
    if let monitoredFileURL,
      let cString = monitoredFileURL.path.cString(using: .utf8)
    {
      libkrbn_unregister_file_updated_callback(cString, externalEditorFileUpdatedCallback)
    }
    monitoredFileURL = nil
    onReloadHandler = nil
  }

  func syncFromAppEditor(
    text: String,
    onError: @escaping (String) -> Void
  ) {
    guard let url = fileURL else {
      return
    }

    if text == lastSyncedText {
      return
    }

    Task { [weak self, url, text] in
      let errorHandler = onError

      let writeResult: Result<Void, Error> = await Task.detached(priority: .utility) {
        do {
          try text.write(to: url, atomically: true, encoding: .utf8)
          return .success(())
        } catch {
          return .failure(error)
        }
      }.value

      switch writeResult {
      case .success:
        self?.lastSyncedText = text
      case .failure(let error):
        errorHandler(error.localizedDescription)
      }
    }
  }

  private func ensureFileURL(onError: (String) -> Void) -> URL? {
    if let url = fileURL {
      return url
    }

    guard let directoryURL = userTmpDirectoryURL(onError: onError) else {
      return nil
    }

    do {
      try FileManager.default.createDirectory(
        at: directoryURL,
        withIntermediateDirectories: true
      )
    } catch {
      onError(error.localizedDescription)
      return nil
    }

    let url = directoryURL.appendingPathComponent("editor_\(UUID().uuidString).json")
    fileURL = url
    return url
  }

  private func userTmpDirectoryURL(onError: (String) -> Void) -> URL? {
    var buffer = [Int8](repeating: 0, count: 4 * 1024)
    libkrbn_get_user_tmp_directory(&buffer, buffer.count)
    let path = String(utf8String: buffer) ?? ""
    guard !path.isEmpty else {
      onError("Failed to get user tmp directory.")
      return nil
    }
    return URL(fileURLWithPath: path, isDirectory: true)
  }

  private func removeTemporaryFile() {
    guard let url = fileURL else {
      return
    }

    if FileManager.default.fileExists(atPath: url.path) {
      do {
        try FileManager.default.removeItem(at: url)
      } catch {
        // Best-effort cleanup for temp files.
      }
    }
  }

  private func externalEditorURL() -> URL? {
    let externalEditorPath = LibKrbn.Settings.shared.externalEditorPath
    if externalEditorPath.isEmpty {
      return nil
    }
    return URL(fileURLWithPath: externalEditorPath)
  }

  private func startMonitoring(
    url: URL,
    onError: @escaping (String) -> Void,
    onReload: @escaping (String) -> Void
  ) {
    if fileURL == url, monitoredFileURL != nil {
      onReloadHandler = onReload
      return
    }

    if fileURL != url || monitoredFileURL == nil {
      stopMonitoring()

      fileURL = url
      onReloadHandler = onReload
      monitoredFileURL = url

      libkrbn_enable_file_monitors()

      if let cString = url.path.cString(using: .utf8) {
        libkrbn_register_file_updated_callback(cString, externalEditorFileUpdatedCallback)
      } else {
        onError("Failed to watch file changes.")
      }
    }
  }

  func handleFileUpdated() {
    guard let url = fileURL else {
      return
    }

    // Capture the current handler and monitored state on the main actor to avoid
    // capturing non-Sendable values in a @Sendable closure.
    let currentHandler: ReloadHandler? = onReloadHandler
    let isMonitoring = (monitoredFileURL != nil)

    // If there's no handler or we're not monitoring anymore, nothing to do.
    guard currentHandler != nil, isMonitoring else {
      return
    }

    Task { [url] in
      guard let text = await Self.readFileWithRetry(url: url) else {
        return
      }

      await MainActor.run { [weak self] in
        guard let self else { return }
        let handler = self.onReloadHandler
        guard self.monitoredFileURL != nil, let handler else { return }

        if text != lastSyncedText {
          lastSyncedText = text
          handler(text)
        }
      }
    }
  }

  private nonisolated static func readFileWithRetry(url: URL) async -> String? {
    // Some editors save via atomic rename; allow brief retry if the file is temporarily unavailable.
    let attempts = 3
    for i in 0..<attempts {
      if let text = try? String(contentsOf: url, encoding: .utf8) {
        return text
      }

      if i < attempts - 1 {
        try? await Task.sleep(nanoseconds: 50_000_000)
      }
    }

    return nil
  }
}
