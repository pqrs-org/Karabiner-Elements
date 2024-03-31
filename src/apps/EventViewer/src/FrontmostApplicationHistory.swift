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
    let bundleIdentifier = String(cString: bundleIdentifierBuffer)
    let filePath = String(cString: filePathBuffer)

    if bundleIdentifier == "org.pqrs.Karabiner-EventViewer" { return }

    Task { @MainActor in
      let e = FrontmostApplicationEntry()
      e.bundleIdentifier = bundleIdentifier
      e.filePath = filePath

      FrontmostApplicationHistory.shared.append(e)
    }
  }
}

public class FrontmostApplicationEntry: Identifiable, Equatable {
  public var id = UUID()
  public var bundleIdentifier = ""
  public var filePath = ""

  public func copyToPasteboard() {
    let string =
      "\(bundleIdentifier)\n" + "\(filePath)\n"

    let pboard = NSPasteboard.general
    pboard.clearContents()
    pboard.writeObjects([string as NSString])
  }

  public static func == (lhs: FrontmostApplicationEntry, rhs: FrontmostApplicationEntry) -> Bool {
    lhs.id == rhs.id
  }
}

public class FrontmostApplicationHistory: ObservableObject {
  public static let shared = FrontmostApplicationHistory()

  private let maxCount = 16

  @Published var entries: [FrontmostApplicationEntry] = []

  private init() {
    libkrbn_enable_frontmost_application_monitor()
    libkrbn_register_frontmost_application_changed_callback(callback)
  }

  deinit {
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
