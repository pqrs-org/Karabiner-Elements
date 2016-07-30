#pragma once

class session final {
public:
  static uid_t get_user_id(void) {
    auto dictionary = CGSessionCopyCurrentDictionary();
    if (!dictionary) {
      return -1;
    }

    uid_t user_id;
    auto number = static_cast<CFNumberRef>(CFDictionaryGetValue(dictionary, kCGSessionUserIDKey));
    CFNumberGetValue(number, kCFNumberIntType, &user_id);

    CFRelease(dictionary);

    return user_id;
  }
};
