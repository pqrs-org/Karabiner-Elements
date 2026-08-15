import Foundation

struct ComplexModificationsAssetFile: Identifiable, Decodable {
  var id = UUID()
  var index: Int
  var title: String
  var userFile: Bool
  var importedAt: Date
  var assetRules: [ComplexModificationsAssetRule]

  private enum CodingKeys: String, CodingKey {
    case index
    case title
    case userFile
    case importedAt
    case assetRules
  }

  init(
    _ index: Int,
    _ title: String,
    _ userFile: Bool,
    _ importedAt: Date,
    _ assetRules: [ComplexModificationsAssetRule]
  ) {
    self.index = index
    self.title = title
    self.userFile = userFile
    self.importedAt = importedAt
    self.assetRules = assetRules
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
