import SwiftUI

struct LogView: View {
    @ObservedObject var logMessages = LogMessages.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GeometryReader { _ in
                ScrollView {
                    logMessages.entries.map {
                        Text($0.text)
                    }.reduce(Text("")) { $0 + $1 }
                }
            }
        }
        .padding()
    }
}

struct LogView_Previews: PreviewProvider {
    static var previews: some View {
        LogView()
    }
}
