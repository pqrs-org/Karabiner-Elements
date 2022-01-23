import SwiftUI

struct ComplexModificationsView: View {
    @ObservedObject private var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            ScrollView {
                VStack(alignment: .leading, spacing: 0.0) {
                    // swiftformat:disable:next unusedArguments
                    ForEach($settings.complexModificationsRules) { $complexModificationRule in
                        HStack(alignment: .center, spacing: 0) {
                            Text(complexModificationRule.description)
                        }
                        .padding(6.0)

                        Divider()
                    }

                    Spacer()
                }
            }
            .background(Color(NSColor.textBackgroundColor))
        }
        .padding()
    }
}

struct ComplexModificationsView_Previews: PreviewProvider {
    static var previews: some View {
        ComplexModificationsView()
    }
}
