import SwiftUI

struct FrontmostApplicationView: View {
  @ObservedObject var frontmostApplicationHistory = FrontmostApplicationHistory.shared

  var body: some View {
    VStack {
      VStack(alignment: .leading, spacing: 12.0) {
        Label(
          "Switch to the app whose bundle identifier you want to check.",
          systemImage: InfoBorder.icon
        )
        .modifier(InfoBorder())

        Button(
          action: {
            frontmostApplicationHistory.clear()
          },
          label: {
            Label("Clear", systemImage: "clear")
          })
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      ScrollViewReader { proxy in
        ScrollView {
          VStack(alignment: .leading, spacing: 0.0) {
            ForEach($frontmostApplicationHistory.entries) { $entry in
              HStack {
                VStack(alignment: .leading, spacing: 0.0) {
                  HStack(alignment: .center, spacing: 0) {
                    Text("Bundle Identifier: ")
                      .font(.caption)

                    Text(entry.bundleIdentifier)
                      .textSelection(.enabled)
                  }

                  HStack(alignment: .center, spacing: 0) {
                    Text("File Path: ")
                      .font(.caption)

                    Text(entry.filePath)
                      .textSelection(.enabled)
                  }
                }
                .frame(maxWidth: .infinity, alignment: .leading)

                if entry.bundleIdentifier.count > 0 || entry.filePath.count > 0 {
                  Button(
                    action: {
                      entry.copyToPasteboard()
                    },
                    label: {
                      Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
                    })
                }
              }
              .padding(.vertical, 6.0)
              .padding(.horizontal, 12.0)

              Divider().id("divider \(entry.id)")
            }
          }
        }
        .background(Color(NSColor.textBackgroundColor))
        .onAppear {
          if let last = frontmostApplicationHistory.entries.last {
            proxy.scrollTo("divider \(last.id)", anchor: .bottom)
          }
        }
        .onChange(of: frontmostApplicationHistory.entries) { newEntries in
          if let last = newEntries.last {
            proxy.scrollTo("divider \(last.id)", anchor: .bottom)
          }
        }
      }
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
  }
}
