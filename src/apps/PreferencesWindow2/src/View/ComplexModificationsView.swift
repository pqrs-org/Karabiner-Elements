import SwiftUI

struct ComplexModificationsView: View {
    @ObservedObject private var settings = Settings.shared
    @State private var moveDisabled: Bool = true
    @State private var showingSheet = false

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            List {
                // swiftformat:disable:next unusedArguments
                ForEach($settings.complexModificationsRules) { $complexModificationRule in
                    VStack {
                        HStack(alignment: .center, spacing: 0) {
                            if settings.complexModificationsRules.count > 1 {
                                Image(systemName: "arrow.up.arrow.down.square.fill")
                                    .resizable(resizingMode: .stretch)
                                    .frame(width: 16.0, height: 16.0)
                                    .onHover { hovering in
                                        moveDisabled = !hovering
                                    }
                            }

                            Text(complexModificationRule.description)
                                .padding(.leading, 6.0)

                            Spacer()

                            Button(action: {
                                settings.removeComplexModificationsRule(complexModificationRule)
                            }) {
                                Label("Remove", systemImage: "minus.circle.fill")
                            }
                        }

                        Divider()
                    }
                    .moveDisabled(moveDisabled)
                }.onMove { indices, destination in
                    if let first = indices.first {
                        settings.moveComplexModificationsRule(first, destination)
                    }
                }
            }
            .background(Color(NSColor.textBackgroundColor))

            Spacer()

            if settings.complexModificationsRules.count > 1 {
                HStack {
                    Text("You can reorder list by dragging")
                    Image(systemName: "arrow.up.arrow.down.square.fill")
                        .resizable(resizingMode: .stretch)
                        .frame(width: 16.0, height: 16.0)
                    Text("icon")
                }
            }

            Button(action: {
                showingSheet = true
            }) {
                Label("Add rule", systemImage: "plus.circle.fill")
            }
        }
        .padding()
        .sheet(isPresented: $showingSheet) {
            ComplexModificationsAssetsView(showing: $showingSheet)
        }
    }
}

struct ComplexModificationsView_Previews: PreviewProvider {
    static var previews: some View {
        ComplexModificationsView()
    }
}
