import Foundation

extension LibKrbn {
  struct ComplexModificationsRule: Identifiable {
    var id = UUID()
    var index: Int
    var description: String

    init(
      _ index: Int,
      _ description: String
    ) {
      self.index = index
      self.description = description
    }
  }
}
