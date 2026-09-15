import AppKit
import SwiftUI

@main @MainActor struct Test {
  static func main() {
    let app = NSApplication.shared
    app.setActivationPolicy(.prohibited)
    var pressed = false
    var updates = 0
    let coordinator = OptionKeyObserver.Coordinator(
      isPressed: Binding(
        get: { pressed },
        set: {
          pressed = $0
          updates += 1
        }))
    coordinator.start()
    coordinator.start()
    func send(_ flags: NSEvent.ModifierFlags) {
      let event = NSEvent.keyEvent(
        with: .flagsChanged, location: .zero, modifierFlags: flags, timestamp: 0, windowNumber: 0,
        context: nil, characters: "", charactersIgnoringModifiers: "", isARepeat: false, keyCode: 58
      )!
      app.sendEvent(event)
    }
    send(.option)
    precondition(pressed && updates == 1)
    send([.option, .shift])
    precondition(pressed && updates == 2)
    send(.shift)
    precondition(!pressed && updates == 3)
    send(.option)
    NotificationCenter.default.post(name: NSApplication.didResignActiveNotification, object: app)
    precondition(!pressed)
    coordinator.stop()
    let count = updates
    send(.option)
    RunLoop.current.run(until: Date(timeIntervalSinceNow: 0.1))
    precondition(updates == count, "Observer updated after removal")
    print(
      "Option press/release, combined modifiers, app deactivation, duplicate-start prevention and cleanup passed."
    )
  }
}
