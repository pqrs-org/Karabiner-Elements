import SwiftShell
import SwiftUI

public class SystemExtensions: ObservableObject {
  public static let shared = SystemExtensions()

  @Published var list = ""

  public func updateList() {
    list = run("/usr/bin/systemextensionsctl", "list").stdout
  }
}
