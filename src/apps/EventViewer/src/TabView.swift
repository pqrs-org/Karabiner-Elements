import Cocoa

class TabView: NSTabView {
    override var acceptsFirstResponder: Bool {
        // Disable to become key view in order to avoid
        // grabbing control-tab when Full Keyboard Access is enabled.

        return false
    }
}
