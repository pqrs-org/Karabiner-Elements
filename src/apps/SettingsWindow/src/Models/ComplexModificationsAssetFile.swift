import Foundation

struct ComplexModificationsAssetFile: Identifiable, Decodable {
  var id: String { filePath }
  var filePath: String
  var title: String
  var userFile: Bool
  var importedAt: Date
  var assetRules: [ComplexModificationsAssetRule]

  private enum CodingKeys: String, CodingKey {
    case filePath
    case title
    case userFile
    case importedAt
    case assetRules
  }

  public func match(_ search: String) -> Bool {
    if title.range(of: search, options: .caseInsensitive) != nil {
      return true
    }

    for assetRule in assetRules {
      if assetRule.description.range(of: search, options: .caseInsensitive) != nil
        || assetRule.descriptionNotes.contains(where: {
          $0.range(of: search, options: .caseInsensitive) != nil
        })
      {
        return true
      }
    }

    return false
  }
}
