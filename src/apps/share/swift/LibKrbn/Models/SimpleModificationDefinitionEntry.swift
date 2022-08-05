import Foundation

extension LibKrbn {
  struct SimpleModificationDefinitionEntry: Identifiable {
    var id = UUID()

    var label: String
    var json: String
    var unsafe: Bool

    init(
      _ label: String,
      _ json: String,
      _ unsafe: Bool
    ) {
      self.label = label
      self.json = json
      self.unsafe = unsafe
    }
  }
}
