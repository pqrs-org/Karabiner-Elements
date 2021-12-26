import SwiftUI

private func callback(_ logLines: UnsafeMutableRawPointer?,
                      _ context: UnsafeMutableRawPointer?)
{
    if logLines == nil { return }
    if context == nil { return }

    let obj: LogMessages! = unsafeBitCast(context, to: LogMessages.self)

    var logMessageEntries: [LogMessageEntry] = []
    let size = libkrbn_log_lines_get_size(logLines)
    for i in 0 ..< size {
        let line = libkrbn_log_lines_get_line(logLines, i)
        if line != nil {
            logMessageEntries.append(LogMessageEntry(String(cString: line!) + "\n"))
        }
    }

    DispatchQueue.main.async { [weak obj] in
        guard let obj = obj else { return }

        obj.append(logMessageEntries)
    }
}

public class LogMessageEntry: Identifiable, Equatable {
    public var id = UUID()
    public var text = ""
    public var foregroundColor = Color.primary
    public var backgroundColor = Color.clear

    init(_ text: String) {
        self.text = text
    }

    public static func == (lhs: LogMessageEntry, rhs: LogMessageEntry) -> Bool {
        lhs.id == rhs.id
    }
}

public class LogMessages: ObservableObject {
    public static let shared = LogMessages()

    let maxCount = 256

    @Published var entries: [LogMessageEntry] = []
    @Published var currentTimeString = ""

    private var timer: Timer?

    init() {
        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_log_monitor(callback, obj)

        //
        // Create timer
        //

        timer = Timer.scheduledTimer(withTimeInterval: 1.0,
                                     repeats: true)
        { [weak self] (_: Timer) in
            guard let self = self else { return }

            let formatter = DateFormatter()
            formatter.locale = Locale(identifier: "en_US_POSIX")
            formatter.dateFormat = "[yyyy-MM-dd HH:mm:ss]"
            self.currentTimeString = formatter.string(from: Date())
        }
    }

    deinit {
        libkrbn_disable_log_monitor()
    }

    public func append(_ newEntries: [LogMessageEntry]) {
        entries += newEntries

        if entries.count > maxCount {
            entries.removeFirst()
        }
    }
}
