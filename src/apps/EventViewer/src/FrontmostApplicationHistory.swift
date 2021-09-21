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

        if obj.text.count > 4 * 1024 {
            obj.text = ""
        }

        obj.text +=
            "Bundle Identifier:  \(identifier)\n" +
            "File Path:          \(path)\n" +
            "\n"
    }
}

public class FrontmostApplicationHistory: ObservableObject {
    public static let shared = FrontmostApplicationHistory()

    @Published var text = ""

    init() {
        text = "Please switch to apps which you want to know Bundle Identifier.\n\n"

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_frontmost_application_monitor(callback, obj)
    }

    deinit {
        libkrbn_disable_frontmost_application_monitor()
    }
}
