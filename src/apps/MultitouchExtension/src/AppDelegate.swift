import AppKit

//
// C methods
//

private func setGrabberVariable(_ count: FingerCount, _ sync: Bool) {
  /*
  static FingerCount previousFingerCount;
  static bool previousFingerCountInitialized = false;

  if (!previousFingerCountInitialized) {
    previousFingerCountInitialized = true;
    previousFingerCount.upperQuarterAreaCount = -1;
    previousFingerCount.lowerQuarterAreaCount = -1;
    previousFingerCount.leftQuarterAreaCount = -1;
    previousFingerCount.rightQuarterAreaCount = -1;
    previousFingerCount.upperHalfAreaCount = -1;
    previousFingerCount.lowerHalfAreaCount = -1;
    previousFingerCount.leftHalfAreaCount = -1;
    previousFingerCount.rightHalfAreaCount = -1;
    previousFingerCount.totalCount = -1;
  }

#define ENTRY(COUNT_NAME, VARIABLE_NAME) \
  { count.COUNT_NAME, &(previousFingerCount.COUNT_NAME), VARIABLE_NAME }

  struct {
    int count;
    int* previousCount;
    const char* name;
  } entries[] = {
      ENTRY(upperQuarterAreaCount, "multitouch_extension_finger_count_upper_quarter_area"),
      ENTRY(lowerQuarterAreaCount, "multitouch_extension_finger_count_lower_quarter_area"),
      ENTRY(leftQuarterAreaCount, "multitouch_extension_finger_count_left_quarter_area"),
      ENTRY(rightQuarterAreaCount, "multitouch_extension_finger_count_right_quarter_area"),
      ENTRY(upperHalfAreaCount, "multitouch_extension_finger_count_upper_half_area"),
      ENTRY(lowerHalfAreaCount, "multitouch_extension_finger_count_lower_half_area"),
      ENTRY(leftHalfAreaCount, "multitouch_extension_finger_count_left_half_area"),
      ENTRY(rightHalfAreaCount, "multitouch_extension_finger_count_right_half_area"),
      ENTRY(totalCount, "multitouch_extension_finger_count_total"),
  };

#undef ENTRY

  for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
    if (*(entries[i].previousCount) != entries[i].count) {
      if (sync) {
        libkrbn_grabber_client_sync_set_variable(entries[i].name, entries[i].count);
      } else {
        libkrbn_grabber_client_async_set_variable(entries[i].name, entries[i].count);
      }

      *(entries[i].previousCount) = entries[i].count;
    }
  }
  */
}

private func enable() {
  Task { @MainActor in
    MultitouchDeviceManager.shared.registerWakeNotification()

    // sleep until devices are settled.
    try await Task.sleep(nanoseconds: NSEC_PER_SEC)

    MultitouchDeviceManager.shared.setCallback(true)

    setGrabberVariable(FingerCount(), false)
  }
}

private func disable() {
  Task { @MainActor in
    MultitouchDeviceManager.shared.setCallback(false)
  }
}

//
// AppDelegate
//

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate {
  private var activity: NSObjectProtocol?

  public func applicationDidFinishLaunching(_: Notification) {
    ProcessInfo.processInfo.enableSuddenTermination()

    libkrbn_initialize()

    KarabinerKit.setup()
    KarabinerKit.observeConsoleUserServerIsDisabledNotification()

    NSApplication.shared.disableRelaunchOnLogin()

    //
    // Handle kHideIconInDock
    //

    if !UserSettings.shared.hideIconInDock {
      var psn = ProcessSerialNumber(highLongOfPSN: 0, lowLongOfPSN: UInt32(kCurrentProcess))
      TransformProcessType(
        &psn, ProcessApplicationTransformState(kProcessTransformToForegroundApplication))
    }

    //
    // Handle --start-at-login
    //

    if CommandLine.arguments.contains("--start-at-login") {
      if !UserSettings.shared.startAtLogin {
        NSApplication.shared.terminate(self)
      }
    }

    //
    // Handle --show-ui
    //

    if CommandLine.arguments.contains("--show-ui") {
      SettingsWindowManager.shared.show()
    }

    //
    // Prepare observers
    //

    NotificationCenter.default.addObserver(
      forName: FingerStatusManager.fingerStateChanged,
      object: nil,
      queue: .main
    ) { _ in
      setGrabberVariable(FingerStatusManager.shared.fingerCount, false)
    }

    //
    // Enable grabber_client
    //

    libkrbn_enable_grabber_client(
      enable,
      disable,
      disable)

    MultitouchDeviceManager.shared.registerIONotification()

    //
    // Disable App Nap
    //

    activity = ProcessInfo.processInfo.beginActivity(
      options: .userInitiated,
      reason: "Disable App Nap in order to receive multitouch events even if this app is background"
    )
  }

  public func applicationWillTerminate(_: Notification) {
    if let a = activity {
      ProcessInfo.processInfo.endActivity(a)
      activity = nil
    }

    MultitouchDeviceManager.shared.setCallback(false)

    setGrabberVariable(FingerCount(), true)

    libkrbn_terminate()
  }

  public func applicationShouldHandleReopen(
    _: NSApplication,
    hasVisibleWindows _: Bool
  ) -> Bool {
    SettingsWindowManager.shared.show()
    return true
  }
}
