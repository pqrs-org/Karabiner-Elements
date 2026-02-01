import AppKit
import Foundation
import UniformTypeIdentifiers

@MainActor
final class ExternalEditorController: ObservableObject {

  private var fileURL: URL?
  private var fileMonitor: DispatchSourceFileSystemObject?
  private var fileDescriptor: Int32 = -1
  private var directoryMonitor: DispatchSourceFileSystemObject?
  private var directoryDescriptor: Int32 = -1
  private var lastSyncedText: String?

  @MainActor
  deinit {
    stopMonitoring()
    removeTemporaryFile()
  }

  func reset() {
    stopMonitoring()
    removeTemporaryFile()
    fileURL = nil
    lastSyncedText = nil
  }

  func chooseEditor() {
    let panel = NSOpenPanel()
    panel.allowsMultipleSelection = false
    panel.canChooseDirectories = false
    panel.canChooseFiles = true
    panel.allowedContentTypes = [UTType.application]
    panel.prompt = "Choose"

    panel.begin { response in
      guard response == .OK, let url = panel.url else {
        return
      }
      LibKrbn.Settings.shared.externalEditorPath = url.path
    }
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

    do {
      try text.write(to: url, atomically: true, encoding: .utf8)
      lastSyncedText = text
      startMonitoring(url: url, onError: onError, onReload: onReload)

      if let editorURL = externalEditorURL() {
        NSWorkspace.shared.open(
          [url],
          withApplicationAt: editorURL,
          configuration: NSWorkspace.OpenConfiguration(),
          completionHandler: nil)
      } else {
        NSWorkspace.shared.open(url)
      }
    } catch {
      onError(error.localizedDescription)
    }
  }

  func reload(
    onError: @escaping (String) -> Void,
    onReload: @escaping (String) -> Void
  ) {
    guard let url = fileURL else {
      return
    }

    do {
      let text = try String(contentsOf: url, encoding: .utf8)
      lastSyncedText = text
      onReload(text)
      startMonitoring(url: url, onError: onError, onReload: onReload)
    } catch {
      onError(error.localizedDescription)
    }
  }

  func stopMonitoring() {
    if let monitor = fileMonitor {
      fileDescriptor = -1
      monitor.cancel()
      fileMonitor = nil
    }
    if fileDescriptor >= 0 {
      close(fileDescriptor)
      fileDescriptor = -1
    }

    if let monitor = directoryMonitor {
      directoryDescriptor = -1
      monitor.cancel()
      directoryMonitor = nil
    }
    if directoryDescriptor >= 0 {
      close(directoryDescriptor)
      directoryDescriptor = -1
    }
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

    do {
      try text.write(to: url, atomically: true, encoding: .utf8)
      lastSyncedText = text
    } catch {
      onError(error.localizedDescription)
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
    if fileURL != url || fileMonitor == nil {
      stopMonitoring()

      fileURL = url

      let fd = open(url.path, O_EVTONLY)
      fileDescriptor = fd
      if fd < 0 {
        onError("Failed to watch file changes.")
        return
      }

      let deliver: (String) -> Void = { [weak self] text in
        DispatchQueue.main.async {
          guard let self else {
            return
          }
          self.handleExternalEditorText(text, onReload: onReload)
        }
      }

      self.fileMonitor = Self.makeFileMonitor(
        url: url,
        fileDescriptor: fd,
        deliver: deliver
      )
      self.fileMonitor?.resume()

      startDirectoryMonitoring(url: url, onError: onError, onReload: onReload, deliver: deliver)
    }
  }

  private nonisolated static func makeFileMonitor(
    url: URL,
    fileDescriptor: Int32,
    deliver: @escaping (String) -> Void
  ) -> DispatchSourceFileSystemObject {
    let fileMonitor = DispatchSource.makeFileSystemObjectSource(
      fileDescriptor: fileDescriptor,
      eventMask: [.write, .extend, .attrib, .rename, .delete],
      queue: DispatchQueue.global(qos: .utility)
    )
    fileMonitor.setEventHandler {
      // Some editors save via atomic rename; allow brief retry if the file is temporarily unavailable.
      let attempts = 3
      for i in 0..<attempts {
        if let text = try? String(contentsOf: url, encoding: .utf8) {
          deliver(text)
          break
        } else {
          // backoff ~50ms between attempts
          if i < attempts - 1 {
            usleep(50_000)
          }
        }
      }
    }
    fileMonitor.setCancelHandler {
      if fileDescriptor >= 0 {
        close(fileDescriptor)
      }
    }
    return fileMonitor
  }

  private func startDirectoryMonitoring(
    url: URL,
    onError: @escaping (String) -> Void,
    onReload: @escaping (String) -> Void,
    deliver: @escaping (String) -> Void
  ) {
    let directoryURL = url.deletingLastPathComponent()
    let fd = open(directoryURL.path, O_EVTONLY)
    directoryDescriptor = fd
    if fd < 0 {
      onError("Failed to watch directory changes.")
      return
    }

    let monitor = Self.makeDirectoryMonitor(
      url: url,
      fileDescriptor: fd,
      deliver: deliver
    )
    directoryMonitor = monitor
    monitor.resume()
  }

  private nonisolated static func makeDirectoryMonitor(
    url: URL,
    fileDescriptor: Int32,
    deliver: @escaping (String) -> Void
  ) -> DispatchSourceFileSystemObject {
    let monitor = DispatchSource.makeFileSystemObjectSource(
      fileDescriptor: fileDescriptor,
      eventMask: [.write, .rename, .delete, .attrib],
      queue: DispatchQueue.global(qos: .utility)
    )
    monitor.setEventHandler {
      // Directory changes may indicate the file was replaced via atomic save.
      // Try reading the target URL; if it exists, deliver its content.
      let attempts = 3
      for i in 0..<attempts {
        if let text = try? String(contentsOf: url, encoding: .utf8) {
          deliver(text)
          break
        } else {
          if i < attempts - 1 {
            usleep(50_000)
          }
        }
      }
    }
    monitor.setCancelHandler {
      if fileDescriptor >= 0 {
        close(fileDescriptor)
      }
    }
    return monitor
  }

  private func handleExternalEditorText(
    _ text: String,
    onReload: @escaping (String) -> Void
  ) {
    if text != lastSyncedText {
      lastSyncedText = text
      onReload(text)
    }
  }
}
