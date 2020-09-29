import Cocoa

public class KeyResponder: NSView {
    @IBOutlet var eventQueue: EventQueue!

    override public func draw(_: NSRect) {
        NSGraphicsContext.saveGraphicsState()

        //
        // Draw area
        //

        let lineWidth = bounds.size.width / 40.0

        NSColor.white.withAlphaComponent(0.8).set()
        NSBezierPath(roundedRect: bounds, xRadius: 10.0, yRadius: 10.0).fill()

        NSColor.black.withAlphaComponent(0.6).set()
        let path = NSBezierPath(roundedRect: bounds, xRadius: 10, yRadius: 10)
        path.lineWidth = lineWidth
        path.stroke()

        //
        // Draw texts
        //

        let textRect = NSMakeRect(lineWidth * 2,
                                  lineWidth * 2,
                                  bounds.size.width - lineWidth * 4,
                                  bounds.size.height - lineWidth * 4)

        let attributes: [NSAttributedString.Key: Any] = [
            NSAttributedString.Key.font: NSFont.boldSystemFont(ofSize: textRect.size.width / 10),
            NSAttributedString.Key.foregroundColor: NSColor.black.withAlphaComponent(0.6),
        ]

        NSString("Mouse Area").draw(in: textRect, withAttributes: attributes)

        NSGraphicsContext.restoreGraphicsState()
    }

//
    // Key event handlers
//

    override public func keyDown(with _: NSEvent) {}

    override public func keyUp(with _: NSEvent) {}

    override public func flagsChanged(with _: NSEvent) {}

    override public func mouseDown(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func mouseUp(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func mouseDragged(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func rightMouseDown(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func rightMouseUp(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func rightMouseDragged(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func otherMouseDown(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func otherMouseUp(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func otherMouseDragged(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func scrollWheel(with event: NSEvent) {
        eventQueue.pushMouseEvent(event)
    }

    override public func swipe(with _: NSEvent) {}
}
