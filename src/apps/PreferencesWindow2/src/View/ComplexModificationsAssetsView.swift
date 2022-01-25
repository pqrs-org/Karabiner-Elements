import SwiftUI

struct ComplexModificationsAssetsView: View {
    @Binding var showing: Bool
    @ObservedObject private var assetFiles = ComplexModificationsAssetFiles.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            Text("files \(assetFiles.files.count)")
            List {
                // swiftformat:disable:next unusedArguments
                ForEach($assetFiles.files) { $assetFile in
                    VStack {
                        HStack(alignment: .center, spacing: 0) {
                            Text(assetFile.title)
                                .padding(.leading, 6.0)

                            Spacer()
                        }

                        Divider()
                    }
                }
            }
            .background(Color(NSColor.textBackgroundColor))

            Spacer()

            Button(action: {
                showing = false
            }) {
                Label("Close", systemImage: "xmark")
                    .frame(minWidth: 0, maxWidth: .infinity)
            }
            .padding(.top, 24.0)
        }
        .padding()
        .frame(width: 1000, height: 600)
        .onAppear {
            assetFiles.updateFiles()
        }
    }
}

struct ComplexModificationsAssetsView_Previews: PreviewProvider {
    @State static var showing = true

    static var previews: some View {
        ComplexModificationsAssetsView(showing: $showing)
    }
}
