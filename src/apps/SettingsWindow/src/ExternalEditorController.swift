import AppKit
import Foundation
import UniformTypeIdentifiers

@MainActor
final class ExternalEditorController: ObservableObject {
  static let shared = ExternalEditorController()

  private typealias ReloadHandler = (String) -> Void

  private var fileURL: URL?
  private var monitoredFileURL: URL?
  private var fileMonitorTask: Task<Void, Never>?
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
        Settings.shared.externalEditorPath = url.path
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
    fileExtension: String,
    onError: @escaping (String) -> Void,
    onReload: @escaping (String) -> Void
  ) {
    guard let url = ensureFileURL(fileExtension: fileExtension, onError: onError) else {
      return
    }

    Task { [weak self, url, text] in
      let errorHandler = onError
      let reloadHandler = onReload

      // Prepare .prettierrc.json for external editors.
      krbn_save_prettierrc()

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
        self?.startMonitoring(url: url, onReload: reloadHandler)
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
    fileMonitorTask?.cancel()
    fileMonitorTask = nil
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

  private func ensureFileURL(fileExtension: String, onError: (String) -> Void) -> URL? {
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

    let url = directoryURL.appendingPathComponent("editor_\(UUID().uuidString).\(fileExtension)")
    fileURL = url
    return url
  }

  private func userTmpDirectoryURL(onError: (String) -> Void) -> URL? {
    var buffer = [Int8](repeating: 0, count: 4 * 1024)
    krbn_get_user_tmp_directory(&buffer, buffer.count)
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
    let externalEditorPath = Settings.shared.externalEditorPath
    if externalEditorPath.isEmpty {
      return nil
    }
    return URL(fileURLWithPath: externalEditorPath)
  }

  private func startMonitoring(
    url: URL,
    onReload: @escaping (String) -> Void
  ) {
    if fileURL == url,
      monitoredFileURL != nil,
      fileMonitorTask != nil
    {
      onReloadHandler = onReload
      return
    }

    if fileURL != url || monitoredFileURL == nil {
      stopMonitoring()

      fileURL = url
      onReloadHandler = onReload
      monitoredFileURL = url

      // Polling is sufficient for this single small temporary file and continues to work when an
      // editor saves by atomically replacing the file. It also needs no monitor restoration after
      // the system wakes from sleep.
      fileMonitorTask = Task { @MainActor [weak self] in
        while !Task.isCancelled {
          do {
            try await Task.sleep(for: .milliseconds(500))
          } catch {
            return
          }

          guard let self, monitoredFileURL == url else { return }
          await pollFile(url: url)
        }
      }
    }
  }

  private func pollFile(url: URL) async {
    guard let text = await Self.readFileWithRetry(url: url),
      monitoredFileURL == url,
      let handler = onReloadHandler
    else {
      return
    }

    if text != lastSyncedText {
      lastSyncedText = text
      handler(text)
    }
  }

  private nonisolated static func readFileWithRetry(url: URL) async -> String? {
    // Some editors save via atomic rename; allow brief retry if the file is temporarily unavailable.
    let attempts = 3
    for i in 0..<attempts {
      guard !Task.isCancelled else { return nil }

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
