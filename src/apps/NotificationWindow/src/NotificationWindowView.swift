public class NotificationWindowView: NSView {
    @IBOutlet var closeButton: NSImageView!
    @IBOutlet var text: NSTextField!

    override public func draw(_: CGRect) {
        NSGraphicsContext.saveGraphicsState()

        NSColor.windowBackgroundColor.set()
        NSBezierPath(roundedRect: bounds, xRadius: 12, yRadius: 12).fill()

        NSGraphicsContext.restoreGraphicsState()
    }

    @IBAction func hideWindow(_: Any) {
        window?.orderOut(self)
    }
}
