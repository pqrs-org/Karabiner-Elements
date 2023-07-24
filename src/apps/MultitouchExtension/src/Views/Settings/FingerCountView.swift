import SwiftUI

struct FingerCountView: View {
  @ObservedObject private var fingerManager = FingerManager.shared

  var body: some View {
    HStack(alignment: .top) {
      let fingerCount = fingerManager.fingerCount
      let font = Font.custom("Menlo", size: 11.0)

      VStack(alignment: .trailing) {
        Text("total")
        Text("\(fingerCount.totalCount)").font(font)
      }
      .padding(.horizontal, 10.0)

      VStack(alignment: .trailing) {
        Text("half")
        Text("upper: \(fingerCount.upperHalfAreaCount)").font(font)
        Text("lower: \(fingerCount.lowerHalfAreaCount)").font(font)
        Text("left: \(fingerCount.leftHalfAreaCount)").font(font)
        Text("right: \(fingerCount.rightHalfAreaCount)").font(font)
      }
      .padding(.horizontal, 10.0)

      VStack(alignment: .trailing) {
        Text("quarter")
        Text("upper: \(fingerCount.upperQuarterAreaCount)").font(font)
        Text("lower: \(fingerCount.lowerQuarterAreaCount)").font(font)
        Text("left: \(fingerCount.leftQuarterAreaCount)").font(font)
        Text("right: \(fingerCount.rightQuarterAreaCount)").font(font)
      }
      .padding(.horizontal, 10.0)
    }
  }
}
