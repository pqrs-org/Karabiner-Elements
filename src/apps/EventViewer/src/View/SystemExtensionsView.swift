import SwiftUI

struct SystemExtensionsView: View {
  @ObservedObject var systemExtensions = SystemExtensions.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("System Extensions")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(action: {
              let pboard = NSPasteboard.general
              pboard.clearContents()
              pboard.writeObjects([systemExtensions.list as NSString])
            }) {
              Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
            }

            Button(action: {
              SystemExtensions.shared.updateList()
            }) {
              Label("Refresh", systemImage: "arrow.clockwise.circle")
            }
          }

          ScrollView {
            HStack {
              VStack(alignment: .leading) {
                Text(systemExtensions.list)
                  .lineLimit(nil)
                  .font(.custom("Menlo", size: 11.0))
                  .padding(5)
                  .macOS12EnableTextSelection()
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
    .onAppear {
      SystemExtensions.shared.updateList()
    }
  }
}

struct SystemExtensionsView_Previews: PreviewProvider {
  static var previews: some View {
    SystemExtensionsView()
  }
}
