import SwiftUI

struct FingerCountView: View {
  @ObservedObject private var fingerManager = FingerManager.shared

  var body: some View {
    HStack(alignment: .top) {
      let fingerCount = fingerManager.fingerCount
      let font = Font.callout.monospaced()

      VStack(alignment: .trailing) {
        AppLocalizedText("multitouch_extension.count.total")
        Text("\(fingerCount.totalCount)").font(font)
      }
      .padding(.horizontal, 10.0)

      VStack(alignment: .trailing) {
        AppLocalizedText("multitouch_extension.count.half")
        AppLocalizedText(
          "multitouch_extension.count.upper", arguments: ["count": String(fingerCount.upperHalfAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.lower", arguments: ["count": String(fingerCount.lowerHalfAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.left", arguments: ["count": String(fingerCount.leftHalfAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.right", arguments: ["count": String(fingerCount.rightHalfAreaCount)]
        ).font(font)
      }
      .padding(.horizontal, 10.0)

      VStack(alignment: .trailing) {
        AppLocalizedText("multitouch_extension.count.quarter")
        AppLocalizedText(
          "multitouch_extension.count.upper", arguments: ["count": String(fingerCount.upperQuarterAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.lower", arguments: ["count": String(fingerCount.lowerQuarterAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.left", arguments: ["count": String(fingerCount.leftQuarterAreaCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.right", arguments: ["count": String(fingerCount.rightQuarterAreaCount)]
        ).font(font)
      }
      .padding(.horizontal, 10.0)

      VStack(alignment: .trailing) {
        AppLocalizedText("multitouch_extension.count.palm_total")
        Text("\(fingerCount.totalPalmCount)").font(font)
      }

      VStack(alignment: .trailing) {
        AppLocalizedText("multitouch_extension.count.palm_half")
        AppLocalizedText(
          "multitouch_extension.count.upper", arguments: ["count": String(fingerCount.upperHalfAreaPalmCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.lower", arguments: ["count": String(fingerCount.lowerHalfAreaPalmCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.left", arguments: ["count": String(fingerCount.leftHalfAreaPalmCount)]
        ).font(font)
        AppLocalizedText(
          "multitouch_extension.count.right", arguments: ["count": String(fingerCount.rightHalfAreaPalmCount)]
        ).font(font)
      }
      .padding(.horizontal, 10.0)
    }
  }
}
