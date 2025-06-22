import SwiftUI

private func callback() {
  var bundleIdentifierBuffer = [Int8](repeating: 0, count: 1024)
  var filePathBuffer = [Int8](repeating: 0, count: 1024)

  if libkrbn_get_frontmost_application(
    &bundleIdentifierBuffer,
    bundleIdentifierBuffer.count,
    &filePathBuffer,
    filePathBuffer.count)
  {
    let bundleIdentifier = String(utf8String: bundleIdentifierBuffer) ?? ""
    let filePath = String(utf8String: filePathBuffer) ?? ""

    if bundleIdentifier == "org.pqrs.Karabiner-EventViewer" { return }
    if bundleIdentifier == "" && filePath == "" { return }

    Task { @MainActor in
      let e = FrontmostApplicationEntry()
      e.bundleIdentifier = bundleIdentifier
      e.filePath = filePath

      FrontmostApplicationHistory.shared.append(e)
    }
  }
}

@MainActor
public class FrontmostApplicationEntry: Identifiable, Equatable {
  nonisolated public let id = UUID()
  public var bundleIdentifier = ""
  public var filePath = ""

  public func copyToPasteboard() {
    let string =
      "\(bundleIdentifier)\n" + "\(filePath)\n"

    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }

  nonisolated public static func == (lhs: FrontmostApplicationEntry, rhs: FrontmostApplicationEntry)
    -> Bool
  {
    lhs.id == rhs.id
  }
}

@MainActor
public class FrontmostApplicationHistory: ObservableObject {
  public static let shared = FrontmostApplicationHistory()

  private let maxCount = 16

  @Published var entries: [FrontmostApplicationEntry] = []

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_frontmost_application_monitor()

    libkrbn_register_frontmost_application_changed_callback(callback)
    libkrbn_enqueue_callback(callback)
  }

  public func stop() {
    libkrbn_disable_frontmost_application_monitor()
  }

  public func append(_ entry: FrontmostApplicationEntry) {
    entries.append(entry)

    if entries.count > maxCount {
      entries.removeFirst()
    }
  }

  public func clear() {
    entries.removeAll()
  }
}
