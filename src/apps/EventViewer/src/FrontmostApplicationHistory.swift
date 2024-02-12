import SwiftUI

private func callback(
  _ bundleIdentifier: UnsafePointer<Int8>?,
  _ filePath: UnsafePointer<Int8>?,
  _ context: UnsafeMutableRawPointer?
) {
  if bundleIdentifier == nil { return }
  if filePath == nil { return }

  let identifier = String(cString: bundleIdentifier!)
  let path = String(cString: filePath!)

  if identifier == "org.pqrs.Karabiner-EventViewer" { return }

  Task { @MainActor in
    let e = FrontmostApplicationEntry()
    e.bundleIdentifier = identifier
    e.filePath = path

    FrontmostApplicationHistory.shared.append(e)
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
    libkrbn_enable_frontmost_application_monitor(callback, nil)
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
