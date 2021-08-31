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
            //
            // Main window
            //

            let mainWindow = NSWindow(
                contentRect: .zero,
                styleMask: [
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )

            mainWindow.contentView = NSHostingView(rootView: MainView())
            mainWindow.backgroundColor = NSColor.clear
            mainWindow.isOpaque = false
            mainWindow.level = .statusBar
            mainWindow.ignoresMouseEvents = true
            mainWindow.collectionBehavior.insert(.canJoinAllSpaces)
            mainWindow.collectionBehavior.insert(.ignoresCycle)
            mainWindow.collectionBehavior.insert(.stationary)

            let screenFrame = screen.visibleFrame
            let windowFrame = mainWindow.frame
            let margin = CGFloat(10.0)
            mainWindow.setFrameOrigin(NSMakePoint(
                screenFrame.origin.x + screenFrame.size.width - windowFrame.width - margin,
                screenFrame.origin.y + margin
            ))

            windows.append(mainWindow)

            //
            // Close button
            //

            let buttonWindow = NSWindow(
                contentRect: NSMakeRect(mainWindow.frame.origin.x + CGFloat(-8.0),
                                        mainWindow.frame.origin.y + CGFloat(36.0),
                                        CGFloat(24.0),
                                        CGFloat(24.0)),
                styleMask: [
                    .fullSizeContentView,
                ],
                backing: .buffered,
                defer: false
            )
            buttonWindow.contentView = NSHostingView(rootView: ButtonView(mainWindow: mainWindow, buttonWindow: buttonWindow))
            buttonWindow.backgroundColor = NSColor.clear
            buttonWindow.isOpaque = false
            buttonWindow.level = .statusBar
            buttonWindow.ignoresMouseEvents = false
            buttonWindow.collectionBehavior.insert(.canJoinAllSpaces)
            buttonWindow.collectionBehavior.insert(.ignoresCycle)
            buttonWindow.collectionBehavior.insert(.stationary)

            mainWindow.addChildWindow(buttonWindow, ordered: .above)

            windows.append(buttonWindow)
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
