import SwiftUI

struct FingerCountView: View {
  @ObservedObject private var fingerManager = FingerManager.shared

  var body: some View {
    VStack(alignment: .trailing) {
      let fingerCount = fingerManager.fingerCount
      let font = Font.custom("Menlo", size: 12.0)

      Text("total: \(fingerCount.totalCount)").font(font)
      Text("half > upper: \(fingerCount.upperHalfAreaCount)").font(font)
      Text("half > lower: \(fingerCount.lowerHalfAreaCount)").font(font)
      Text("half >  left: \(fingerCount.leftHalfAreaCount)").font(font)
      Text("half > right: \(fingerCount.rightHalfAreaCount)").font(font)
      Text("quarter > upper: \(fingerCount.upperQuarterAreaCount)").font(font)
      Text("quarter > lower: \(fingerCount.lowerQuarterAreaCount)").font(font)
      Text("quarter >  left: \(fingerCount.leftQuarterAreaCount)").font(font)
      Text("quarter > right: \(fingerCount.rightQuarterAreaCount)").font(font)
    }
  }
}
