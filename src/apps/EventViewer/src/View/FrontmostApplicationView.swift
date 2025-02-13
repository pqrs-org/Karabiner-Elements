import SwiftUI

struct FrontmostApplicationView: View {
  @ObservedObject var frontmostApplicationHistory = FrontmostApplicationHistory.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("Frontmost application history")) {
        VStack(alignment: .leading, spacing: 12.0) {
          Text(
            "You can investigate a bundle identifier and a file path of the frontmost application.")

          Label(
            "Please switch to apps which you want to know Bundle Identifier.",
            systemImage: "info.circle.fill")

          if frontmostApplicationHistory.entries.count == 0 {
            Divider()
            Spacer()
          } else {
            HStack(alignment: .center, spacing: 12.0) {
              Button(
                action: {
                  frontmostApplicationHistory.clear()
                },
                label: {
                  Label("Clear", systemImage: "clear")
                })

              Spacer()
            }

            ScrollViewReader { proxy in
              ScrollView {
                VStack(alignment: .leading, spacing: 0.0) {
                  ForEach($frontmostApplicationHistory.entries) { $entry in
                    HStack {
                      VStack {
                        HStack(alignment: .center, spacing: 0) {
                          Text("Bundle Identifier: ")
                            .font(.caption)

                          Text(entry.bundleIdentifier)
                            .textSelection(.enabled)

                          Spacer()
                        }

                        HStack(alignment: .center, spacing: 0) {
                          Text("File Path: ")
                            .font(.caption)

                          Text(entry.filePath)
                            .textSelection(.enabled)

                          Spacer()
                        }
                      }

                      Spacer()

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

                  Spacer()
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
        }
        .padding()
      }
    }
    .padding()
  }
}

struct FrontmostApplicationView_Previews: PreviewProvider {
  static var previews: some View {
    FrontmostApplicationView()
  }
}
