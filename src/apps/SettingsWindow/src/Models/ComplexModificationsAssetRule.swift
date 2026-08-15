import Foundation

struct ComplexModificationsAssetRule: Identifiable, Decodable {
  var id = UUID()
  var fileIndex: Int
  var ruleIndex: Int
  var description: String
  var descriptionNotes: [String]

  private enum CodingKeys: String, CodingKey {
    case fileIndex
    case ruleIndex
    case description
    case descriptionNotes
  }

  init(_ fileIndex: Int, _ ruleIndex: Int, _ description: String, _ descriptionNotes: [String]) {
    self.fileIndex = fileIndex
    self.ruleIndex = ruleIndex
    self.description = description
    self.descriptionNotes = descriptionNotes
  }
}
