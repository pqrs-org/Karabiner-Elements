import Foundation

extension LibKrbn {
  public struct KeyboardType: Identifiable {
    enum NamedType: Int {
      case ansi = 40
      case iso = 41
      case jis = 42
    }

    public private(set) var id = UUID()
    public private(set) var countryCode: Int
    public var keyboardType: Int

    init(_ countryCode: Int, _ keyboardType: Int) {
      self.countryCode = countryCode
      self.keyboardType = keyboardType
    }
  }
}
