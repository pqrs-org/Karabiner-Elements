import Foundation

extension LibKrbn {
  struct ComplexModificationsRule: Identifiable {
    var id = UUID()
    var index: Int
    var description: String
    var jsonString: String?

    init(
      _ index: Int,
      _ description: String,
      _ jsonString: String?
    ) {
      self.index = index
      self.description = description
      self.jsonString = jsonString
    }
  }
}
