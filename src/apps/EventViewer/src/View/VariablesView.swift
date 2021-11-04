import SwiftUI

struct VariablesView: View {
    @ObservedObject var variablesJsonString = VariablesJsonString.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Internal variables of Karabiner-Elements")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Button(action: {
                        let pboard = NSPasteboard.general
                        pboard.clearContents()
                        pboard.writeObjects([variablesJsonString.text as NSString])
                    }) {
                        Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
                    }

                    ScrollView {
                        HStack {
                            VStack(alignment: .leading) {
                                Text(variablesJsonString.text)
                                    .lineLimit(nil)
                                    .font(.custom("Menlo", size: 11.0))
                                    .padding(5)
                            }

                            Spacer()
                        }
                        .background(Color(NSColor.textBackgroundColor))
                    }
                }
                .padding(6.0)
            }

            Spacer()
        }
        .padding()
    }
}

struct VariablesView_Previews: PreviewProvider {
    static var previews: some View {
        VariablesView()
    }
}
