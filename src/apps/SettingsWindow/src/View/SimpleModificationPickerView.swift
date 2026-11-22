import AppKit
import SwiftUI

struct SimpleModificationPickerView: View {
  private enum PickerButtonLayout {
    static let height: CGFloat = 24
    static let horizontalPadding: CGFloat = 4
    static let itemSpacing: CGFloat = 4
    static let indicatorWidth: CGFloat = 12

    static var textReservedWidth: CGFloat {
      horizontalPadding * 2 + itemSpacing + indicatorWidth
    }
  }

  private enum PickerItem: Hashable {
    case category(UUID)
    case entry(categoryID: UUID, entryID: UUID)
  }

  @AppLocalizationContext private var localized
  @State private var expandedCategoryIDs: Set<UUID> = []
  @State private var listFocusRequest = 0
  @State private var presented = false
  @State private var searchText = ""
  @State private var selectedItem: PickerItem?
  private(set) var categories: SimpleModificationDefinitionCategories
  private(set) var label: String
  private(set) var action: (_ json: String) -> Void
  private(set) var showUnsafe: Bool

  var body: some View {
    GeometryReader { buttonGeometry in
      Button {
        expandedCategoryIDs.removeAll()
        searchText = ""
        selectedItem = nil
        presented = true
      } label: {
        GeometryReader { labelGeometry in
          pickerButtonLabel(size: labelGeometry.size)
        }
      }
      .frame(width: buttonGeometry.size.width)
      .buttonStyle(.bordered)
      .popover(
        isPresented: $presented,
        attachmentAnchor: .rect(.bounds),
        arrowEdge: .bottom
      ) {
        VStack(spacing: 0) {
          TextField(
            localized("settings.key_picker.search"),
            text: $searchText
          )
          .textFieldStyle(.roundedBorder)
          .padding(10)
          .onChange(of: searchText) { _ in
            selectedItem = visibleItems.first
          }

          Divider()

          if !normalizedSearchText.isEmpty && filteredCategories.isEmpty {
            Text(localized("settings.key_picker.no_search_results"))
              .foregroundStyle(.secondary)
              .frame(maxWidth: .infinity, maxHeight: .infinity)
          } else {
            ScrollViewReader { proxy in
              List(selection: $selectedItem) {
                ForEach(filteredCategories) { category in
                  if normalizedSearchText.isEmpty {
                    Button {
                      selectedItem = .category(category.id)
                      toggle(category)
                      listFocusRequest += 1
                    } label: {
                      categoryLabel(
                        category,
                        expanded: expandedCategoryIDs.contains(category.id)
                      )
                      .contentShape(Rectangle())
                    }
                    .buttonStyle(.plain)
                    .id(PickerItem.category(category.id))
                    .tag(PickerItem.category(category.id))

                    if expandedCategoryIDs.contains(category.id) {
                      entryRows(category.entries, categoryID: category.id, indented: true)
                    }
                  } else {
                    Section {
                      entryRows(category.entries, categoryID: category.id)
                    } header: {
                      Text(verbatim: localizedLabel(category.name))
                    }
                  }
                }
              }
              .listStyle(.inset)
              .onChange(of: selectedItem) { item in
                guard let item else { return }
                proxy.scrollTo(item)
              }
            }
          }
        }
        .frame(width: 400, height: 440)
        .background {
          SimpleModificationPickerKeyEventMonitor(
            focusRequest: listFocusRequest,
            handler: handleKeyEvent)
        }
        .onAppear {
          selectedItem = visibleItems.first
        }
      }
    }
    .frame(height: PickerButtonLayout.height)
  }

  private func pickerButtonLabel(size: CGSize) -> some View {
    HStack(spacing: PickerButtonLayout.itemSpacing) {
      Text(verbatim: localizedLabel(label))
        .lineLimit(1)
        .truncationMode(.tail)
        // Reserve the explicit padding, spacing, and indicator width so long text truncates
        // without changing the trailing indicator position inside the bordered button.
        .frame(
          width: max(0, size.width - PickerButtonLayout.textReservedWidth),
          alignment: .leading)

      Image(systemName: "chevron.up.chevron.down")
        .font(.caption2)
        .foregroundStyle(.secondary)
        .frame(width: PickerButtonLayout.indicatorWidth)
    }
    .padding(.horizontal, PickerButtonLayout.horizontalPadding)
    .frame(
      width: size.width,
      height: size.height,
      alignment: .leading
    )
    .contentShape(Rectangle())
  }

  @ViewBuilder
  private func categoryLabel(
    _ category: SimpleModificationDefinitionCategory,
    expanded: Bool? = nil
  ) -> some View {
    HStack {
      if let expanded {
        Image(systemName: "chevron.right")
          .frame(width: 12)
          .foregroundStyle(.secondary)
          .rotationEffect(.degrees(expanded ? 90 : 0))
      }

      Image(
        systemName: category.include(label: label)
          ? "checkmark.circle" : "circle"
      )
      .foregroundStyle(category.include(label: label) ? Color.accentColor : Color.secondary)

      Text(verbatim: localizedLabel(category.name))

      Spacer()
    }
  }

  @ViewBuilder
  private func entryRows(
    _ entries: [SimpleModificationDefinitionEntry],
    categoryID: UUID,
    indented: Bool = false
  ) -> some View {
    ForEach(entries) { entry in
      Button {
        presented = false
        action(entry.json)
      } label: {
        HStack {
          Image(
            systemName: entry.label == label
              ? "checkmark.circle.fill" : "circle"
          )
          .foregroundStyle(entry.label == label ? Color.accentColor : Color.secondary)

          Text(verbatim: localizedLabel(entry.label))

          Spacer()
        }
        .contentShape(Rectangle())
        // The disclosure indicator offsets the category checkmark by about 20 points.
        // Indent one level beyond that checkmark so the child hierarchy is visible.
        .padding(.leading, indented ? 40 : 0)
      }
      .buttonStyle(.plain)
      .id(PickerItem.entry(categoryID: categoryID, entryID: entry.id))
      .tag(PickerItem.entry(categoryID: categoryID, entryID: entry.id))
    }
  }

  private var visibleItems: [PickerItem] {
    filteredCategories.flatMap { category in
      if normalizedSearchText.isEmpty {
        var items: [PickerItem] = [.category(category.id)]
        if expandedCategoryIDs.contains(category.id) {
          items.append(
            contentsOf: category.entries.map {
              .entry(categoryID: category.id, entryID: $0.id)
            })
        }
        return items
      }

      return category.entries.map {
        .entry(categoryID: category.id, entryID: $0.id)
      }
    }
  }

  private func moveSelection(_ direction: MoveCommandDirection) {
    switch direction {
    case .up:
      moveSelectionVertically(by: -1)
    case .down:
      moveSelectionVertically(by: 1)
    case .left:
      switch selectedItem {
      case .category(let categoryID):
        setExpanded(false, categoryID: categoryID)
      case .entry(let categoryID, _) where normalizedSearchText.isEmpty:
        selectedItem = .category(categoryID)
      case .entry:
        break
      case nil:
        break
      }
    case .right:
      let item = selectedItem ?? visibleItems.first
      selectedItem = item
      if case .category(let categoryID) = item {
        setExpanded(true, categoryID: categoryID)
      }
    default:
      break
    }
  }

  private func moveSelectionVertically(by offset: Int) {
    let items = visibleItems
    guard !items.isEmpty else { return }

    guard let selectedItem, let index = items.firstIndex(of: selectedItem) else {
      self.selectedItem = offset > 0 ? items.first : items.last
      return
    }

    let nextIndex = min(max(index + offset, items.startIndex), items.index(before: items.endIndex))
    self.selectedItem = items[nextIndex]
  }

  private func handleKeyEvent(_ event: NSEvent, isEditingText: Bool) -> Bool {
    guard !isEditingText else { return false }

    let modifiers = event.modifierFlags.intersection([.command, .option, .control, .shift])
    if modifiers.isEmpty {
      switch event.keyCode {
      case 123:
        moveSelection(.left)
        return true
      case 124:
        moveSelection(.right)
        return true
      case 125:
        moveSelection(.down)
        return true
      case 126:
        moveSelection(.up)
        return true
      case 36, 49, 76:
        activateSelectedItem()
        return true
      default:
        return false
      }
    }

    if modifiers == .control {
      switch event.charactersIgnoringModifiers {
      case "p":
        moveSelection(.up)
        return true
      case "n":
        moveSelection(.down)
        return true
      case "b":
        moveSelection(.left)
        return true
      case "f":
        moveSelection(.right)
        return true
      default:
        return false
      }
    }

    return false
  }

  private func activateSelectedItem() {
    switch selectedItem {
    case .category(let categoryID):
      setExpanded(!expandedCategoryIDs.contains(categoryID), categoryID: categoryID)
    case .entry(_, let entryID):
      guard
        let entry = filteredCategories.lazy.flatMap(\.entries).first(where: { $0.id == entryID })
      else { return }
      presented = false
      action(entry.json)
    case nil:
      break
    }
  }

  private func toggle(_ category: SimpleModificationDefinitionCategory) {
    setExpanded(!expandedCategoryIDs.contains(category.id), categoryID: category.id)
  }

  private func setExpanded(_ expanded: Bool, categoryID: UUID) {
    withAnimation(.easeInOut(duration: 0.15)) {
      if expanded {
        expandedCategoryIDs.insert(categoryID)
      } else {
        expandedCategoryIDs.remove(categoryID)
      }
    }
  }

  private var normalizedSearchText: String {
    searchText.trimmingCharacters(in: .whitespacesAndNewlines)
  }

  private var filteredCategories: [SimpleModificationDefinitionCategory] {
    let query = normalizedSearchText

    return categories.categories.compactMap { category in
      let categoryMatches = matches(category.name, query: query)
      var filteredCategory = category
      filteredCategory.entries = category.entries.filter { entry in
        guard !entry.unsafe || showUnsafe else { return false }
        return query.isEmpty || categoryMatches || matches(entry, query: query)
      }
      return filteredCategory.entries.isEmpty ? nil : filteredCategory
    }
  }

  private func matches(_ entry: SimpleModificationDefinitionEntry, query: String) -> Bool {
    matches(entry.label, query: query)
      || entry.json.localizedStandardContains(query)
  }

  private func matches(_ label: String, query: String) -> Bool {
    localized.allLocalizedStrings(for: label).contains {
      $0.localizedStandardContains(query)
    }
  }

  // Definitions contain either an explicit localization key or a literal key name.
  // Unknown strings (including custom JSON) are displayed verbatim by the lookup.
  private func localizedLabel(_ label: String) -> String {
    localized(label)
  }
}

@MainActor
private struct SimpleModificationPickerKeyEventMonitor: NSViewRepresentable {
  var focusRequest: Int
  var handler: @MainActor (NSEvent, Bool) -> Bool

  func makeCoordinator() -> Coordinator {
    Coordinator(handler: handler)
  }

  func makeNSView(context: Context) -> MonitorView {
    let view = MonitorView(focusRequest: focusRequest)
    context.coordinator.install(for: view)
    return view
  }

  func updateNSView(_ view: MonitorView, context: Context) {
    context.coordinator.handler = handler
    if view.focusRequest != focusRequest {
      view.focusRequest = focusRequest
      view.focusList()
    }
  }

  static func dismantleNSView(_ view: MonitorView, coordinator: Coordinator) {
    coordinator.removeMonitor()
  }

  @MainActor
  final class MonitorView: NSView {
    var focusRequest: Int

    override var acceptsFirstResponder: Bool { true }

    init(focusRequest: Int) {
      self.focusRequest = focusRequest
      super.init(frame: .zero)
    }

    required init?(coder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }

    override func viewDidMoveToWindow() {
      super.viewDidMoveToWindow()
      focusList()
    }

    func focusList() {
      // A popover automatically focuses its first text field after SwiftUI's onAppear.
      // Wait until the AppKit hierarchy is attached, then make the list the actual first
      // responder so arrow keys are routed as list navigation from the outset.
      DispatchQueue.main.async { [weak self] in
        guard let self, let window = self.window else { return }

        let responder = self.firstListView(in: window.contentView) ?? self
        window.initialFirstResponder = responder
        window.makeFirstResponder(responder)
      }
    }

    private func firstListView(in view: NSView?) -> NSView? {
      guard let view else { return nil }
      if view is NSTableView || view is NSCollectionView {
        return view
      }

      for subview in view.subviews {
        if let listView = firstListView(in: subview) {
          return listView
        }
      }

      return nil
    }
  }

  @MainActor
  final class Coordinator {
    var handler: @MainActor (NSEvent, Bool) -> Bool
    private var monitor: Any?

    init(handler: @escaping @MainActor (NSEvent, Bool) -> Bool) {
      self.handler = handler
    }

    func install(for view: NSView) {
      monitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) {
        [weak self, weak view] event in
        let handled = MainActor.assumeIsolated {
          guard let self, let window = view?.window else { return false }

          let isEditingText =
            (window.firstResponder as? NSTextView)?.isEditable == true
          return self.handler(event, isEditingText)
        }
        return handled ? nil : event
      }
    }

    func removeMonitor() {
      if let monitor {
        NSEvent.removeMonitor(monitor)
        self.monitor = nil
      }
    }
  }
}
