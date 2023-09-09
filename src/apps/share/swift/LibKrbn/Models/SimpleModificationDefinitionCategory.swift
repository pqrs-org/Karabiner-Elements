import Foundation

extension LibKrbn {
  struct SimpleModificationDefinitionCategory: Identifiable {
    var id = UUID()

    var name: String
    var entries: [SimpleModificationDefinitionEntry] = []

    init(
      _ name: String
    ) {
      self.name = name
    }

    func include(label: String) -> Bool {
      for e: SimpleModificationDefinitionEntry in entries where e.label == label {
        return true
      }
      return false
    }
  }
}
