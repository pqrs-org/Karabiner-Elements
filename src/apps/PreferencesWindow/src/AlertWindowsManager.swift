import AppKit
import Foundation
import SwiftUI

public class AlertWindowsManager {
  static let shared = AlertWindowsManager()

  var parentWindow: NSWindow?
  var driverNotLoadedAlertWindow: NSWindow?
  var driverVersionNotMatchedAlertWindow: NSWindow?
  var inputMonitoringPermissionsAlertWindow: NSWindow?

  //
  // driverNotLoadedAlertWindow
  //

  public func showDriverNotLoadedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverNotLoadedAlertWindow == nil {
        let view = DriverNotLoadedAlertView(
          parentWindow: parentWindow,
          onCloseButtonPressed: { [weak self] in
            guard let self = self else { return }

            self.hideDriverNotLoadedAlertWindow()
          })

        driverNotLoadedAlertWindow = NSPanel(
          contentRect: .zero,
          styleMask: [
            .titled,
            .closable,
            .fullSizeContentView,
          ],
          backing: .buffered,
          defer: false
        )
        driverNotLoadedAlertWindow!.contentView = NSHostingView(rootView: view)

        parentWindow.beginSheet(driverNotLoadedAlertWindow!) { [weak self] _ in
          guard let self = self else { return }

          self.driverNotLoadedAlertWindow = nil
        }
      }
    }
  }

  public func hideDriverNotLoadedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverNotLoadedAlertWindow != nil {
        parentWindow.endSheet(driverNotLoadedAlertWindow!)
      }
    }
  }

  //
  // driverVersionNotMatchedAlertWindow
  //

  public func showDriverVersionNotMatchedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverVersionNotMatchedAlertWindow == nil {
        let view = DriverVersionNotMatchedAlertView(onCloseButtonPressed: { [weak self] in
          guard let self = self else { return }

          self.hideDriverVersionNotMatchedAlertWindow()
        })

        driverVersionNotMatchedAlertWindow = NSPanel(
          contentRect: .zero,
          styleMask: [
            .titled,
            .closable,
            .fullSizeContentView,
          ],
          backing: .buffered,
          defer: false
        )
        driverVersionNotMatchedAlertWindow!.contentView = NSHostingView(rootView: view)

        parentWindow.beginSheet(driverVersionNotMatchedAlertWindow!) { [weak self] _ in
          guard let self = self else { return }

          self.driverVersionNotMatchedAlertWindow = nil
        }
      }
    }
  }

  public func hideDriverVersionNotMatchedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverVersionNotMatchedAlertWindow != nil {
        parentWindow.endSheet(driverVersionNotMatchedAlertWindow!)
      }
    }
  }

  //
  // inputMonitoringPermissionsAlertWindow
  //

  public func showInputMonitoringPermissionsAlertWindow() {
    if let parentWindow = parentWindow {
      if inputMonitoringPermissionsAlertWindow == nil {
        let view = InputMonitoringPermissionsAlertView(
          parentWindow: parentWindow,
          onCloseButtonPressed: { [weak self] in
            guard let self = self else { return }

            self.hideInputMonitoringPermissionsAlertWindow()
          })

        inputMonitoringPermissionsAlertWindow = NSPanel(
          contentRect: .zero,
          styleMask: [
            .titled,
            .closable,
            .fullSizeContentView,
          ],
          backing: .buffered,
          defer: false
        )
        inputMonitoringPermissionsAlertWindow!.contentView = NSHostingView(rootView: view)

        parentWindow.beginSheet(inputMonitoringPermissionsAlertWindow!) { [weak self] _ in
          guard let self = self else { return }

          self.inputMonitoringPermissionsAlertWindow = nil
        }
      }
    }
  }

  public func hideInputMonitoringPermissionsAlertWindow() {
    if let parentWindow = parentWindow {
      if inputMonitoringPermissionsAlertWindow != nil {
        parentWindow.endSheet(inputMonitoringPermissionsAlertWindow!)
      }
    }
  }
}
