import SwiftUI

struct FrontmostApplicationView: View {
    @ObservedObject var frontmostApplicationHistory = FrontmostApplicationHistory.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Frontmost application history")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Text("You can investigate a bundle identifier and a file path of the frontmost application.")

                    ScrollViewReader { proxy in
                        ScrollView {
                            HStack {
                                VStack(alignment: .leading) {
                                    Text(frontmostApplicationHistory.text)
                                        .lineLimit(nil)
                                        .font(.custom("Menlo", size: 11.0))

                                    Text("").id("frontmostApplicationHistoryButtom")
                                }

                                Spacer()
                            }
                            .background(Color(NSColor.textBackgroundColor))
                        }
                        .onReceive(frontmostApplicationHistory.$text) { _ in
                            proxy.scrollTo("frontmostApplicationHistoryButtom", anchor: .bottom)
                        }
                    }
                }
                .padding(6.0)
            }

            Spacer()
        }
        .padding()
    }
}

struct FrontmostApplicationView_Previews: PreviewProvider {
    static var previews: some View {
        FrontmostApplicationView()
    }
}
