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

  private enum KeyCharacter {
    static let returnKey = "\r"
    static let enter = "\u{3}"
    static let space = " "
  }

  private static let categoryExpansionAnimation = Animation.easeInOut(duration: 0.15)

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
                      toggleCategory(category.id)
                    } label: {
                      categoryLabel(
                        category,
                        expanded: expandedCategoryIDs.contains(category.id)
                      )
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
              .animation(Self.categoryExpansionAnimation, value: expandedCategoryIDs)
              .onChange(of: selectedItem) { item in
                guard let item else { return }
                proxy.scrollTo(item)
              }
            }
          }
        }
        .frame(width: 400, height: 440)
        .background {
          SimpleModificationPickerInteractionMonitor(
            focusRequest: listFocusRequest,
            keyHandler: handleKeyEvent,
            rowClickHandler: handleRowClick)
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

      Label {
        Text(verbatim: localizedLabel(category.name))
      } icon: {
        Image(
          systemName: category.include(label: label)
            ? "checkmark.circle" : "circle"
        )
        .foregroundStyle(category.include(label: label) ? Color.accentColor : Color.secondary)
      }
      .labelStyle(.titleAndIcon)

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
        _ = selectEntry(entry.id)
      } label: {
        Label {
          Text(verbatim: localizedLabel(entry.label))
        } icon: {
          Image(
            systemName: entry.label == label
              ? "checkmark.circle.fill" : "circle"
          )
          .foregroundStyle(entry.label == label ? Color.accentColor : Color.secondary)
        }
        .labelStyle(.titleAndIcon)
        .frame(maxWidth: .infinity, alignment: .leading)
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

  private func handleRowClick(_ row: Int) -> Bool {
    guard visibleItems.indices.contains(row) else { return false }

    switch visibleItems[row] {
    case .category(let categoryID):
      toggleCategory(categoryID)
      return true
    case .entry(_, let entryID):
      return selectEntry(entryID)
    }
  }

  private func handleKeyEvent(_ event: NSEvent, isEditingText: Bool) -> Bool {
    guard !isEditingText else { return false }

    let modifiers = event.modifierFlags.intersection([.command, .option, .control, .shift])
    if modifiers.isEmpty {
      switch event.specialKey {
      case .leftArrow:
        moveSelection(.left)
        return true
      case .rightArrow:
        moveSelection(.right)
        return true
      case .downArrow:
        moveSelection(.down)
        return true
      case .upArrow:
        moveSelection(.up)
        return true
      default:
        break
      }

      switch event.charactersIgnoringModifiers {
      case KeyCharacter.returnKey, KeyCharacter.enter, KeyCharacter.space:
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
      _ = selectEntry(entryID)
    case nil:
      break
    }
  }

  private func toggleCategory(_ categoryID: UUID) {
    selectedItem = .category(categoryID)
    setExpanded(!expandedCategoryIDs.contains(categoryID), categoryID: categoryID)
    listFocusRequest += 1
  }

  private func selectEntry(_ entryID: UUID) -> Bool {
    guard
      let entry = filteredCategories.lazy.flatMap(\.entries).first(where: { $0.id == entryID })
    else { return false }

    presented = false
    action(entry.json)
    return true
  }

  private func setExpanded(_ expanded: Bool, categoryID: UUID) {
    if expanded {
      expandedCategoryIDs.insert(categoryID)
    } else {
      expandedCategoryIDs.remove(categoryID)
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

/// Provides AppKit interaction hooks that SwiftUI `List` does not expose directly.
/// It gives the native list initial keyboard focus and routes keyboard events and clicks
/// anywhere in a native list row back to the SwiftUI selection logic.
@MainActor
private struct SimpleModificationPickerInteractionMonitor: NSViewRepresentable {
  var focusRequest: Int
  var keyHandler: @MainActor (NSEvent, Bool) -> Bool
  var rowClickHandler: @MainActor (Int) -> Bool

  func makeCoordinator() -> Coordinator {
    Coordinator(
      keyHandler: keyHandler,
      rowClickHandler: rowClickHandler)
  }

  func makeNSView(context: Context) -> MonitorView {
    let view = MonitorView(focusRequest: focusRequest)
    context.coordinator.install(for: view)
    return view
  }

  func updateNSView(_ view: MonitorView, context: Context) {
    context.coordinator.keyHandler = keyHandler
    context.coordinator.rowClickHandler = rowClickHandler
    if view.focusRequest != focusRequest {
      view.focusRequest = focusRequest
      view.focusList()
    }
  }

  static func dismantleNSView(_ view: MonitorView, coordinator: Coordinator) {
    coordinator.removeMonitor()
  }

  /// An invisible anchor view that accesses the popover's `NSWindow` and moves first-responder
  /// status to its native list when the view is attached or a new focus request arrives.
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

        let responder = Self.firstListView(in: window.contentView) ?? self
        window.initialFirstResponder = responder
        window.makeFirstResponder(responder)
      }
    }

    static func firstListView(in view: NSView?) -> NSView? {
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

  /// Owns the local event monitor for the representable's lifetime and forwards keyboard events
  /// and native list-row clicks to the current SwiftUI handlers.
  @MainActor
  final class Coordinator {
    var keyHandler: @MainActor (NSEvent, Bool) -> Bool
    var rowClickHandler: @MainActor (Int) -> Bool
    private var monitor: Any?

    init(
      keyHandler: @escaping @MainActor (NSEvent, Bool) -> Bool,
      rowClickHandler: @escaping @MainActor (Int) -> Bool
    ) {
      self.keyHandler = keyHandler
      self.rowClickHandler = rowClickHandler
    }

    func install(for view: NSView) {
      monitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .leftMouseDown]) {
        [weak self, weak view] event in
        let handled = MainActor.assumeIsolated {
          guard let self, let window = view?.window else { return false }

          if event.type == .leftMouseDown {
            guard event.window === window else { return false }
            if let itemIndex = self.itemIndex(at: event.locationInWindow, in: window) {
              return self.rowClickHandler(itemIndex)
            }
            return false
          }

          let isEditingText =
            (window.firstResponder as? NSTextView)?.isEditable == true
          return self.keyHandler(event, isEditingText)
        }
        return handled ? nil : event
      }
    }

    private func itemIndex(at windowPoint: NSPoint, in window: NSWindow) -> Int? {
      if let tableView = MonitorView.firstListView(in: window.contentView) as? NSTableView {
        let point = tableView.convert(windowPoint, from: nil)
        guard tableView.bounds.contains(point) else { return nil }
        let row = tableView.row(at: point)
        guard row >= 0 else { return nil }

        // SwiftUI List represents Section headers as group rows. Exclude those rows so
        // the resulting index matches the flattened PickerItem array.
        guard !isGroupRow(row, in: tableView) else { return nil }
        return (0..<row).reduce(into: 0) { itemIndex, precedingRow in
          if !isGroupRow(precedingRow, in: tableView) {
            itemIndex += 1
          }
        }
      }

      if let collectionView = MonitorView.firstListView(in: window.contentView) as? NSCollectionView
      {
        let point = collectionView.convert(windowPoint, from: nil)
        guard collectionView.bounds.contains(point) else { return nil }
        guard let indexPath = collectionView.indexPathForItem(at: point) else { return nil }

        return (0..<indexPath.section).reduce(indexPath.item) { itemIndex, section in
          itemIndex + collectionView.numberOfItems(inSection: section)
        }
      }

      return nil
    }

    private func isGroupRow(_ row: Int, in tableView: NSTableView) -> Bool {
      tableView.delegate?.tableView?(tableView, isGroupRow: row) ?? false
    }

    func removeMonitor() {
      if let monitor {
        NSEvent.removeMonitor(monitor)
        self.monitor = nil
      }
    }
  }
}
