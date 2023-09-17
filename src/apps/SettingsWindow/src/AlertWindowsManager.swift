import AppKit
import Foundation
import SwiftUI

public class AlertWindowsManager {
  static let shared = AlertWindowsManager()

  var parentWindow: NSWindow?
  var driverNotActivatedAlertWindow: NSWindow?
  var driverVersionMismatchedAlertWindow: NSWindow?
  var inputMonitoringPermissionsAlertWindow: NSWindow?

  //
  // driverNotActivatedAlertWindow
  //

  public func showDriverNotActivatedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverNotActivatedAlertWindow == nil {
        let view = DriverNotActivatedAlertView(
          parentWindow: parentWindow,
          onCloseButtonPressed: { [weak self] in
            guard let self = self else { return }

            self.hideDriverNotActivatedAlertWindow()
          })

        driverNotActivatedAlertWindow = NSPanel(
          contentRect: .zero,
          styleMask: [
            .titled,
            .closable,
            .fullSizeContentView,
          ],
          backing: .buffered,
          defer: false
        )
        driverNotActivatedAlertWindow!.contentView = NSHostingView(rootView: view)

        parentWindow.beginSheet(driverNotActivatedAlertWindow!) { [weak self] _ in
          guard let self = self else { return }

          self.driverNotActivatedAlertWindow = nil
        }
      }
    }
  }

  public func hideDriverNotActivatedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverNotActivatedAlertWindow != nil {
        parentWindow.endSheet(driverNotActivatedAlertWindow!)
      }
    }
  }

  //
  // driverVersionMismatchedAlertWindow
  //

  public func showDriverVersionMismatchedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverVersionMismatchedAlertWindow == nil {
        let view = DriverVersionMismatchedAlertView(onCloseButtonPressed: { [weak self] in
          guard let self = self else { return }

          self.hideDriverVersionMismatchedAlertWindow()
        })

        driverVersionMismatchedAlertWindow = NSPanel(
          contentRect: .zero,
          styleMask: [
            .titled,
            .closable,
            .fullSizeContentView,
          ],
          backing: .buffered,
          defer: false
        )
        driverVersionMismatchedAlertWindow!.contentView = NSHostingView(rootView: view)

        parentWindow.beginSheet(driverVersionMismatchedAlertWindow!) { [weak self] _ in
          guard let self = self else { return }

          self.driverVersionMismatchedAlertWindow = nil
        }
      }
    }
  }

  public func hideDriverVersionMismatchedAlertWindow() {
    if let parentWindow = parentWindow {
      if driverVersionMismatchedAlertWindow != nil {
        parentWindow.endSheet(driverVersionMismatchedAlertWindow!)
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
