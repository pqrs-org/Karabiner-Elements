import Foundation

struct ComplexModificationsAssetRule: Identifiable, Decodable {
  var id = UUID()
  var filePath: String
  var ruleIndex: Int
  var description: String
  var descriptionNotes: [String]

  private enum CodingKeys: String, CodingKey {
    case filePath
    case ruleIndex
    case description
    case descriptionNotes
  }

  init(_ filePath: String, _ ruleIndex: Int, _ description: String, _ descriptionNotes: [String]) {
    self.filePath = filePath
    self.ruleIndex = ruleIndex
    self.description = description
    self.descriptionNotes = descriptionNotes
  }
}
