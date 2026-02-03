import Foundation

extension LibKrbn {
  @MainActor
  class ComplexModificationsRule: Identifiable, Equatable, ObservableObject {
    enum CodeType {
      case json
      case javascript
    }

    nonisolated let id = UUID()
    var index: Int
    var description: String
    var codeString: String?
    var codeType: libkrbn_complex_modifications_rule_code_type

    init(
      index: Int,
      description: String,
      enabled: Bool,
      codeString: String?,
      codeType: libkrbn_complex_modifications_rule_code_type
    ) {
      self.index = index
      self.description = description
      self.enabled = enabled
      self.codeString = codeString
      self.codeType = codeType
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
