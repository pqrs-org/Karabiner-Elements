import SwiftUI

private func callback(_ filePath: UnsafePointer<Int8>?,
                      _ context: UnsafeMutableRawPointer?)
{
    let path = String(cString: filePath!)
    let obj: NotificationWindowManager! = unsafeBitCast(context, to: NotificationWindowManager.self)

    let json = KarabinerKitJsonUtility.loadFile(path)
    if let dictionary = json as? [String: Any] {
        if let body = dictionary["body"] as? String {
            DispatchQueue.main.async { [weak obj] in
                guard let obj = obj else { return }

                NotificationMessage.shared.text = body.trimmingCharacters(in: .whitespacesAndNewlines)
                obj.updateWindowsVisibility()
            }
        }
    }
}

public class NotificationWindowManager {
    private var windows: [NSWindow] = []
    private var observers = KarabinerKitSmartObserverContainer()

    init() {
        let center = NotificationCenter.default
        let o = center.addObserver(forName: NSApplication.didChangeScreenParametersNotification,
                                   object: nil,
                                   queue: .main) { [weak self] _ in
            guard let self = self else { return }

            self.updateWindows()
        }

        observers.addObserver(o, notificationCenter: center)

        updateWindows()

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_notification_message_json_file_monitor(callback, obj)
    }

    deinit {
        libkrbn_disable_notification_message_json_file_monitor()
    }

    func updateWindows() {
        // The old window sometimes does not deallocated properly when screen count is decreased.
        // Thus, we hide the window before clear windows.
        windows.forEach {
            w in w.orderOut(self)
        }
        windows.removeAll()

        NSScreen.screens.forEach { screen in
            let window = NSWindow(
                contentRect: .zero,
                styleMask: [
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )

            window.contentView = NSHostingView(rootView: NotificationView(window: window))
            window.backgroundColor = NSColor.clear
            window.isOpaque = false
            window.level = .statusBar
            // window.ignoresMouseEvents = true
            window.collectionBehavior.insert(.canJoinAllSpaces)
            window.collectionBehavior.insert(.ignoresCycle)
            window.collectionBehavior.insert(.stationary)

            let screenFrame = screen.visibleFrame
            let windowFrame = window.frame
            let margin = CGFloat(10.0)
            window.setFrameOrigin(NSMakePoint(
                screenFrame.origin.x + screenFrame.size.width - windowFrame.width - margin,
                screenFrame.origin.y + margin
            ))

            windows.append(window)
        }

        updateWindowsVisibility()
    }

    func updateWindowsVisibility() {
        let hide = NotificationMessage.shared.text.isEmpty

        windows.forEach { w in
            if hide {
                w.orderOut(self)
            } else {
                w.orderFront(self)
            }
        }
    }
}
