#pragma once

#define BOOST_ASIO_HEADER_ONLY
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_REGEX_NO_LIB
#define BOOST_SYSTEM_NO_LIB

// Build error in boost/asio.hpp @ 1.63.0 with -Wshorten-64-to-32

#define BEGIN_BOOST_INCLUDE        \
  _Pragma("clang diagnostic push") \
      _Pragma("clang diagnostic ignored \"-Wshorten-64-to-32\"")

#define END_BOOST_INCLUDE \
  _Pragma("clang diagnostic pop")
