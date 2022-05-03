import Foundation

extension LibKrbn {
  struct Profile: Identifiable {
    var id = UUID()
    var index: Int
    var name: String
    var selected: Bool

    init(_ index: Int, _ name: String, _ selected: Bool) {
      self.index = index
      self.name = name
      self.selected = selected
    }
  }
}
