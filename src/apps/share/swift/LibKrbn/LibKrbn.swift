public struct LibKrbn {
  public static func deviceDetailsJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_device_details_json_file_path(&buffer, buffer.count)
    return String(cString: buffer)
  }

  public static func grabberStateJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_grabber_state_json_file_path(&buffer, buffer.count)
    return String(cString: buffer)
  }

  public static func manipulatorEnvironmentJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_manipulator_environment_json_file_path(&buffer, buffer.count)
    return String(cString: buffer)
  }

  public static func notificationMessageJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_notification_message_json_file_path(&buffer, buffer.count)
    return String(cString: buffer)
  }

  public static func observerStateJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_observer_state_json_file_path(&buffer, buffer.count)
    return String(cString: buffer)
  }
}
