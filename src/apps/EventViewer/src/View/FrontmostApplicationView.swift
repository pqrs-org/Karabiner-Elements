import SwiftUI

struct FrontmostApplicationView: View {
  @ObservedObject var frontmostApplicationHistory = LibKrbn.FrontmostApplicationHistory.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Label(
          "Switch to the app whose bundle identifier you want to check.",
          systemImage: InfoBorder.icon
        )
        .modifier(InfoBorder())
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

                Button(
                  action: {
                    let string =
                      "\(entry.bundleIdentifier)\n" + "\(entry.filePath)\n"

                    let pboard = NSPasteboard.general
                    pboard.clearContents()
                    pboard.writeObjects([string as NSString])
                  },
                  label: {
                    Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
                  })
              }
              .if(entry == frontmostApplicationHistory.entries.last) {
                $0.overlay(
                  RoundedRectangle(cornerRadius: 2)
                    .inset(by: -4)
                    .stroke(
                      Color.accentColor,
                      lineWidth: 2
                    )
                )
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
      .border(Color(NSColor.separatorColor), width: 2)
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
  }
}
