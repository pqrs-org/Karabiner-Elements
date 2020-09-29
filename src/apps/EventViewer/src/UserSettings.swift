import Foundation

final class UserSettings: ObservableObject {
    static let shared = UserSettings()

    @UserDefault("kForceStayTop", defaultValue: false)
    var forceStayTop: Bool

    @UserDefault("kShowInAllSpaces", defaultValue: false)
    var showInAllSpaces: Bool

    @UserDefault("kShowHex", defaultValue: false)
    var showHex: Bool
}
