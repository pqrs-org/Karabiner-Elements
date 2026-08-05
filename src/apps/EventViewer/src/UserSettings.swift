import SwiftUI

final class UserSettings: ObservableObject {
  @AppStorage("kForceStayTop") var forceStayTop = false
  @AppStorage("kShowInAllSpaces") var showInAllSpaces = false
}
