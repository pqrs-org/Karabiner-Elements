import SwiftUI

struct ComplexModificationsRuleDescriptionView: View {
  let description: String
  let descriptionNotes: [String]

  var body: some View {
    VStack(alignment: .leading, spacing: 2.0) {
      Text(description)

      ForEach(descriptionNotes.indices, id: \.self) { index in
        Text(descriptionNotes[index])
          .font(.caption)
          .foregroundColor(.secondary)
      }
    }
  }
}
