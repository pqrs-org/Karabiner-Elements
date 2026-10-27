import Foundation

struct ComplexModificationsAssetRule: Identifiable, Decodable {
  var id = UUID()
  var filePath: String
  var ruleIndex: Int
  var description: String
  var descriptionNotes: [String]
  // Used to determine whether this asset rule is enabled in the selected profile.
  // It represents the manipulators for JSON rules and the source for JavaScript rules.
  var comparisonKey: String

  private enum CodingKeys: String, CodingKey {
    case filePath
    case ruleIndex
    case description
    case descriptionNotes
    case comparisonKey
  }
}
