// -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*-

@import Cocoa;

#define weakify(VAR) \
  autoreleasepool {} \
  __weak __typeof(VAR) VAR##_weak_ = VAR;

#define strongify(VAR)                              \
  autoreleasepool {}                                \
  _Pragma("clang diagnostic push");                 \
  _Pragma("clang diagnostic ignored \"-Wshadow\""); \
  __strong typeof(VAR) VAR = VAR##_weak_;           \
  _Pragma("clang diagnostic pop");
