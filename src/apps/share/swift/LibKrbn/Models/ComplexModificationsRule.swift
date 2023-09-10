import Foundation

extension LibKrbn {
  struct ComplexModificationsRule: Identifiable, Equatable {
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

    public static func == (lhs: ComplexModificationsRule, rhs: ComplexModificationsRule) -> Bool {
      lhs.id == rhs.id
    }
  }
}
