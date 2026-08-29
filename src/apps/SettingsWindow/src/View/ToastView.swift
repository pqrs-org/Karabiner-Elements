import SwiftUI

struct SettingsToast: Identifiable, Equatable {
  let id = UUID()
  let message: String
}

struct ToastView: View {
  let toast: SettingsToast
  let onDismiss: () -> Void

  var body: some View {
    HStack(spacing: 12) {
      Label(
        toast.message,
        systemImage: WarningBorder.icon
      )
      .font(.callout)

      Spacer()

      Button(action: onDismiss) {
        Image(systemName: "xmark")
      }
      .buttonStyle(.plain)
      .accessibilityLabel("Dismiss")
    }
    .foregroundStyle(Color.warningForeground)
    .modifier(WarningBorder(padding: 20))
    .background(Color.warningBackground, in: RoundedRectangle(cornerRadius: 8))
    .frame(maxWidth: 700)
    .task(id: toast.id) {
      do {
        try await Task.sleep(for: .seconds(4))
      } catch {
        return
      }

      onDismiss()
    }
  }
}
