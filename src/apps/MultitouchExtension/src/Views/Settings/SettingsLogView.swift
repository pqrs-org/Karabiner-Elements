import SwiftUI

@MainActor
final class VariableUpdateLog: ObservableObject {
  struct Entry: Identifiable {
    let id = UUID()
    let date: Date
    let fingerCount: FingerCount
  }

  static let shared = VariableUpdateLog()

  @Published private(set) var entries: [Entry] = []

  private let maximumEntryCount = 9

  func append(_ fingerCount: FingerCount) {
    entries.insert(
      Entry(
        date: Date(),
        fingerCount: fingerCount),
      at: 0)

    if entries.count > maximumEntryCount {
      entries.removeLast(entries.count - maximumEntryCount)
    }
  }

  func clear() {
    entries.removeAll()
  }
}

struct SettingsLogView: View {
  @ObservedObject private var variableUpdateLog = VariableUpdateLog.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 10) {
      HStack {
        Text("Variable update requests")
          .font(.headline)

        Spacer()

        Button("Clear") {
          variableUpdateLog.clear()
        }
        .disabled(variableUpdateLog.entries.isEmpty)
      }

      Divider()

      if variableUpdateLog.entries.isEmpty {
        Text("No variable updates")
          .foregroundStyle(.secondary)
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      } else {
        VStack(spacing: 0) {
          VariableUpdateLogTableHeader()

          ForEach(Array(variableUpdateLog.entries.enumerated()), id: \.element.id) { index, entry in
            VariableUpdateLogTableRow(
              entry: entry,
              isEven: index.isMultiple(of: 2))
          }
        }
        .font(.system(.caption, design: .monospaced))
        .frame(maxHeight: .infinity, alignment: .top)
      }
    }
    .frame(height: 400)
  }
}

private enum VariableUpdateLogTableLayout {
  static let timeWidth: CGFloat = 86
  static let totalWidth: CGFloat = 38
  static let directionWidth: CGFloat = 33
  static let headerRowHeight: CGFloat = 24
  static let dataRowHeight: CGFloat = 30

  static let halfWidth = directionWidth * 4
  static let quarterWidth = directionWidth * 4
  static let fingerWidth = totalWidth + halfWidth + quarterWidth
  static let palmWidth = totalWidth + halfWidth
}

private struct VariableUpdateLogTableHeader: View {
  var body: some View {
    HStack(alignment: .top, spacing: 0) {
      textTableCell(
        "Time",
        width: VariableUpdateLogTableLayout.timeWidth,
        height: VariableUpdateLogTableLayout.headerRowHeight * 3)

      VStack(spacing: 0) {
        HStack(spacing: 0) {
          textTableCell("Finger", width: VariableUpdateLogTableLayout.fingerWidth)
          textTableCell("Palm", width: VariableUpdateLogTableLayout.palmWidth)
        }

        HStack(spacing: 0) {
          textTableCell("total", width: VariableUpdateLogTableLayout.totalWidth)
          textTableCell("half", width: VariableUpdateLogTableLayout.halfWidth)
          textTableCell("quarter", width: VariableUpdateLogTableLayout.quarterWidth)
          textTableCell("total", width: VariableUpdateLogTableLayout.totalWidth)
          textTableCell("half", width: VariableUpdateLogTableLayout.halfWidth)
        }

        HStack(spacing: 0) {
          textTableCell("", width: VariableUpdateLogTableLayout.totalWidth)
          directionHeaders()
          directionHeaders()
          textTableCell("", width: VariableUpdateLogTableLayout.totalWidth)
          directionHeaders()
        }
      }
    }
    .fontWeight(.semibold)
  }

  private func directionHeaders() -> some View {
    ForEach(["up", "down", "left", "right"], id: \.self) { label in
      textTableCell(label, width: VariableUpdateLogTableLayout.directionWidth)
    }
  }
}

private struct VariableUpdateLogTableRow: View {
  let entry: VariableUpdateLog.Entry
  let isEven: Bool

  var body: some View {
    let count = entry.fingerCount
    let backgroundColor = Color.secondary.opacity(isEven ? 0.03 : 0.1)

    HStack(spacing: 0) {
      textTableCell(
        entry.date.formatted(
          .dateTime
            .hour()
            .minute()
            .second()
            .secondFraction(.fractional(3))),
        width: VariableUpdateLogTableLayout.timeWidth,
        height: VariableUpdateLogTableLayout.dataRowHeight,
        backgroundColor: backgroundColor)

      numberTableCell(count.totalCount, backgroundColor: backgroundColor)
      numberTableCell(count.upperHalfAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.lowerHalfAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.leftHalfAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.rightHalfAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.upperQuarterAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.lowerQuarterAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.leftQuarterAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.rightQuarterAreaCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.totalPalmCount, backgroundColor: backgroundColor)
      numberTableCell(count.upperHalfAreaPalmCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.lowerHalfAreaPalmCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.leftHalfAreaPalmCount, direction: true, backgroundColor: backgroundColor)
      numberTableCell(count.rightHalfAreaPalmCount, direction: true, backgroundColor: backgroundColor)
    }
  }

  private func numberTableCell(
    _ value: Int,
    direction: Bool = false,
    backgroundColor: Color
  ) -> some View {
    textTableCell(
      String(value),
      width: direction
        ? VariableUpdateLogTableLayout.directionWidth
        : VariableUpdateLogTableLayout.totalWidth,
      height: VariableUpdateLogTableLayout.dataRowHeight,
      backgroundColor: backgroundColor)
  }
}

private func textTableCell(
  _ text: String,
  width: CGFloat,
  height: CGFloat = VariableUpdateLogTableLayout.headerRowHeight,
  backgroundColor: Color = Color.secondary.opacity(0.14)
) -> some View {
  Text(text)
    .lineLimit(1)
    .minimumScaleFactor(0.75)
    .frame(width: width, height: height)
    .background(backgroundColor)
    .overlay {
      Rectangle()
        .stroke(Color.secondary.opacity(0.25), lineWidth: 0.5)
    }
}
