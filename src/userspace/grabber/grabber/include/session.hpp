#pragma once

class session final {
public:
  static bool get_user_id(uid_t& user_id) {
    auto dictionary = CGSessionCopyCurrentDictionary();
    if (!dictionary) {
      return false;
    }

    bool result = false;
    auto number = static_cast<CFNumberRef>(CFDictionaryGetValue(dictionary, kCGSessionUserIDKey));
    if (number) {
      CFNumberGetValue(number, kCFNumberIntType, &user_id);
      result = true;
    }

    CFRelease(dictionary);

    return result;
  }
};
