#pragma once

class KernelVersion final {
public:
  static int getMajorReleaseVersion(void) {
    char buffer[32];
    size_t size = sizeof(buffer);
    if (sysctlbyname("kern.osrelease", buffer, &size, nullptr, 0) == 0) {
      return toInt(buffer);
    }

    return -1;
  }

private:
  static int toInt(const char* string) {
    int result = 0;

    if (string) {
      auto p = string;
      int sign = 1;
      if (*p == '-') {
        sign = -1;
        ++p;
      }
      while (*p) {
        if ('1' <= *p && *p <= '9') {
          result *= 10;
          result += (*p - '1') + 1;
        } else if (*p == '0') {
          result *= 10;
        } else {
          break;
        }
        ++p;
      }
      result *= sign;
    }

    return result;
  };
};
