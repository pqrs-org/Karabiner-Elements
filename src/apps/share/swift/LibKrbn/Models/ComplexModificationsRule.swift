import Foundation

extension LibKrbn {
  @MainActor
  class ComplexModificationsRule: Identifiable, Equatable, ObservableObject {
    nonisolated let id = UUID()
    var index: Int
    var description: String
    var jsonString: String?

    init(
      index: Int,
      description: String,
      enabled: Bool,
      jsonString: String?
    ) {
      self.index = index
      self.description = description
      self.enabled = enabled
      self.jsonString = jsonString
    }

    nonisolated public static func == (lhs: ComplexModificationsRule, rhs: ComplexModificationsRule)
      -> Bool
    {
      lhs.id == rhs.id
    }

    @Published var enabled: Bool {
      didSet {
        libkrbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(
          index, enabled)

        Settings.shared.save()
      }
    }
  }
}
