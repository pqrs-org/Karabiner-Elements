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

                obj.setText(body)
            }
        }
    }
}

public class NotificationWindowManager {
    var text = ""
    var windowControllers: [NSWindowController] = []
    var observers = KarabinerKitSmartObserverContainer()

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
        // Thus, we hide the window before clear windowControllers.
        windowControllers.forEach { c in
            c.window?.orderOut(self)
        }
        windowControllers.removeAll()

        NSScreen.screens.forEach { screen in
            let controller = NSWindowController(windowNibName: "NotificationWindow")
            if let window = controller.window {
                setupWindow(window: window, screen: screen)
                windowControllers.append(controller)
            }
        }

        setNotificationText()
    }

    func setupWindow(window: NSWindow!,
                     screen: NSScreen!)
    {
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
            screenFrame.origin.x + screenFrame.size.width - windowFrame.size.width - margin,
            screenFrame.origin.y + margin
        ))
    }

    func setNotificationText() {
        windowControllers.forEach { controller in
            if text.isEmpty {
                controller.window?.orderOut(self)
            } else {
                let view = controller.window?.contentView as! NotificationWindowView
                view.text.stringValue = text

                controller.window?.orderFront(self)
            }
        }
    }

    func setText(_ text: String) {
        self.text = text

        DispatchQueue.main.async { [weak self] in
            guard let self = self else { return }

            self.setNotificationText()
        }
    }
}
