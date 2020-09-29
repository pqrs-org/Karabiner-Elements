import Cocoa

public class InputMonitoringAlertWindowController: NSWindowController {
    @IBOutlet var parentWindow: NSWindow!

    public func show() {
        guard let window = window else { return }

        let frame = NSMakeRect(
            parentWindow.frame.origin.x + (parentWindow.frame.size.width / 2) - (window.frame.size.width / 2),
            parentWindow.frame.origin.y + (parentWindow.frame.size.height / 2) - (window.frame.size.height / 2),
            window.frame.size.width,
            window.frame.size.height
        )

        window.setFrame(frame, display: false)
        parentWindow.addChildWindow(window, ordered: .above)
    }

    @IBAction func openSystemPreferencesSecurity(_: NSButton) {
        NSApplication.shared.miniaturizeAll(self)

        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy")!
        NSWorkspace.shared.open(url)
    }
}
