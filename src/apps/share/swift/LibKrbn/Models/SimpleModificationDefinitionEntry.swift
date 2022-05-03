import Foundation

extension LibKrbn {
  struct SimpleModificationDefinitionEntry: Identifiable {
    var id = UUID()

    var label: String
    var json: String

    init(
      _ label: String,
      _ json: String
    ) {
      self.label = label
      self.json = json
    }
  }
}
