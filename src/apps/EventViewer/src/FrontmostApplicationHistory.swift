import SwiftUI

private func callback(_ bundleIdentifier: UnsafePointer<Int8>?,
                      _ filePath: UnsafePointer<Int8>?,
                      _ context: UnsafeMutableRawPointer?)
{
    if bundleIdentifier == nil { return }
    if filePath == nil { return }

    let identifier = String(cString: bundleIdentifier!)
    let path = String(cString: filePath!)
    let obj: FrontmostApplicationHistory! = unsafeBitCast(context, to: FrontmostApplicationHistory.self)

    if identifier == "org.pqrs.Karabiner-EventViewer" { return }

    DispatchQueue.main.async { [weak obj] in
        guard let obj = obj else { return }

        let e = FrontmostApplicationEntry()
        e.bundleIdentifier = identifier
        e.filePath = path

        obj.append(e)
    }
}

public class FrontmostApplicationEntry: Identifiable, Equatable {
    public var id = UUID()
    public var bundleIdentifier = ""
    public var filePath = ""

    public static func == (lhs: FrontmostApplicationEntry, rhs: FrontmostApplicationEntry) -> Bool {
        lhs.id == rhs.id
    }
}

public class FrontmostApplicationHistory: ObservableObject {
    public static let shared = FrontmostApplicationHistory()

    let maxCount = 16

    @Published var entries: [FrontmostApplicationEntry] = []

    init() {
        clear()

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_frontmost_application_monitor(callback, obj)
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

        // Fill with empty entries to avoid SwiftUI List rendering corruption at FrontmostApplicationView.swift.
        while entries.count < maxCount {
            entries.append(FrontmostApplicationEntry())
        }
    }
}
