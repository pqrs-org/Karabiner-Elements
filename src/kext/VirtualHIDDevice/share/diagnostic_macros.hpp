#pragma once

#define BEGIN_IOKIT_INCLUDE        \
  _Pragma("clang diagnostic push") \
      _Pragma("clang diagnostic ignored \"-Winconsistent-missing-override\"")
      _Pragma("clang diagnostic ignored \"-W#warnings\"")

#define END_IOKIT_INCLUDE \
  _Pragma("clang diagnostic pop")
