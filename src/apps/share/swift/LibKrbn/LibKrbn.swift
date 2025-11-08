public struct LibKrbn {
  public static func devicesJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_devices_json_file_path(&buffer, buffer.count)
    return String(utf8String: buffer) ?? ""
  }

  public static func coreServiceStateJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_core_service_state_json_file_path(&buffer, buffer.count)
    return String(utf8String: buffer) ?? ""
  }

  public static func notificationMessageJsonFilePath() -> String {
    var buffer = [Int8](repeating: 0, count: 1024)
    libkrbn_get_notification_message_json_file_path(&buffer, buffer.count)
    return String(utf8String: buffer) ?? ""
  }
}
