import SwiftUI

struct VariablesView: View {
  @ObservedObject var evCoreServiceClient = EVCoreServiceClient.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        HStack(alignment: .center, spacing: 12.0) {
          Button(
            action: {
              let pboard = NSPasteboard.general
              pboard.clearContents()
              pboard.writeObjects([evCoreServiceClient.manipulatorEnvironmentStream.text as NSString])
            },
            label: {
              Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
            })

          Button(
            action: {
              resetVariables()
            },
            label: {
              Label("Reset variables", systemImage: "arrow.counterclockwise")
            })
        }
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      RealtimeText(
        stream: evCoreServiceClient.manipulatorEnvironmentStream,
        font: NSFont.monospacedSystemFont(
          ofSize: NSFont.preferredFont(forTextStyle: .callout).pointSize,
          weight: .regular)
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(Color(NSColor.textBackgroundColor))
      .border(Color(NSColor.separatorColor), width: 2)
      .padding(.leading, 2)  // Prevent the header underline from disappearing in NavigationSplitView.
    }
    .onAppear {
      evCoreServiceClient.manipulatorEnvironmentStart()
    }
    .onDisappear {
      evCoreServiceClient.manipulatorEnvironmentStop()
    }
  }

  private func resetVariables() {
    let jsonText = evCoreServiceClient.manipulatorEnvironmentStream.text

    guard let jsonData = jsonText.data(using: .utf8),
      let json = try? JSONSerialization.jsonObject(with: jsonData) as? [String: Any],
      let variables = json["variables"] as? [String: Any]
    else {
      return
    }

    for (name, _) in variables {
      libkrbn_core_service_client_async_set_variable(name, Int32(0))
    }
  }
}
