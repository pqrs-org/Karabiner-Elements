import SwiftUI

struct NotificationView: View {
  @ObservedObject private var state = ConsoleUserServerUIState.shared
  @Environment(\.colorScheme) private var colorScheme
  @State private var opacity = 1.0
  @State private var applicationIcon = Self.makeApplicationIcon()

  let maximumWidth: CGFloat

  private let maximumTextCharactersPerLine: CGFloat = 20.0
  private let horizontalSpacing: CGFloat = 8.0
  private let iconWidth: CGFloat = 48.0
  var body: some View {
    HStack(alignment: .top, spacing: horizontalSpacing) {
      if state.notificationWindowSettings.showIcon {
        Image(nsImage: applicationIcon)
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: iconWidth, height: iconWidth)
      }
      Text(state.notificationMessage)
        .font(.system(size: CGFloat(state.notificationWindowSettings.fontSize)))
        .multilineTextAlignment(.leading)
        .frame(width: textWidth, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(textColor)
    }
    .padding(.leading, state.notificationWindowSettings.showIcon ? 4.0 : 6.0)
    .padding(.trailing, state.notificationWindowSettings.showIcon ? 12.0 : 6.0)
    .padding(.vertical, state.notificationWindowSettings.showIcon ? 4.0 : 2.0)
    .background(
      RoundedRectangle(cornerRadius: 12)
        .fill(backgroundColor)
    )
    .opacity(opacity)
    .whenHovered { hover in
      opacity = hover ? 0.2 : 1.0
    }
    .onChange(of: state.appIconNumber) { _ in
      applicationIcon = Self.makeApplicationIcon()
    }
  }

  private var theme: UIStatePayload.NotificationWindowSettings.Colors.Theme {
    switch colorScheme {
    case .dark:
      state.notificationWindowSettings.colors.dark
    default:
      state.notificationWindowSettings.colors.light
    }
  }

  private var backgroundColor: Color {
    theme.backgroundColor == "system"
      ? Color(nsColor: .windowBackgroundColor)
      : Color(colorString: theme.backgroundColor)
  }

  private var textColor: Color {
    theme.textColor == "system"
      ? Color(nsColor: .labelColor)
      : Color(colorString: theme.textColor)
  }

  private var textWidth: CGFloat {
    let font = NSFont.systemFont(ofSize: CGFloat(state.notificationWindowSettings.fontSize))
    let maximumTextWidth = min(
      font.pointSize * maximumTextCharactersPerLine,
      maximumAvailableTextWidth)
    let singleLineWidth =
      state.notificationMessage
      .components(separatedBy: .newlines)
      .map {
        ceil(($0 as NSString).size(withAttributes: [.font: font]).width)
      }
      .max() ?? 0.0

    return min(singleLineWidth, maximumTextWidth)
  }

  private var maximumAvailableTextWidth: CGFloat {
    let iconAndSpacingWidth =
      state.notificationWindowSettings.showIcon ? iconWidth + horizontalSpacing : 0.0
    let horizontalPadding = 6.0 + (state.notificationWindowSettings.showIcon ? 12.0 : 6.0)
    return max(1.0, maximumWidth - iconAndSpacingWidth - horizontalPadding)
  }

  private static func makeApplicationIcon() -> NSImage {
    NSWorkspace.shared
      .icon(forFile: Bundle.main.bundlePath)
      .removingTransparentPadding()
  }
}
