import Foundation

extension LibKrbn {
  struct KeyboardType: Identifiable {
    enum NamedType: Int {
      case ansi = 40
      case iso = 41
      case jis = 42
    }

    var id = UUID()
    var countryCode: Int
    var keyboardType: Int

    init(_ countryCode: Int, _ keyboardType: Int) {
      self.countryCode = countryCode
      self.keyboardType = keyboardType
    }
  }
}
