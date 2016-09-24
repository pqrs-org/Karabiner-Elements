#pragma once

#include "boost_defs.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <iomanip>

class spdlog_utility {
public:
  static boost::optional<uint64_t> get_sort_key(const std::string& line) {
    // line == "[2016-09-22 20:18:37.649] [grabber] [info] version 0.90.36"
    // return 20160922201837649

    // We can parse time strictly by using boost::posix_time::time_from_string.
    // But we cannot use it with boost header only.
    // So we use this rough way.

    if (line.size() < strlen("[0000-00-00 00:00:00.000]")) {
      return boost::none;
    }

    if (line.empty()) {
      return boost::none;
    }

    if (line[0] != '[') {
      return boost::none;
    }

    std::string result_string(4 + 2 + 2 +     // years,months,days
                                  2 + 2 + 2 + // hours,minutes,seconds
                                  3,          // milliseconds
                              '0');
    size_t line_pos = 1;
    size_t result_pos = 0;

    // years
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // months
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // days
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // hours
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // minutes
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // seconds
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    // milliseconds
    ++line_pos;
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];
    result_string[result_pos++] = line[line_pos++];

    try {
      return boost::lexical_cast<uint64_t>(result_string);
    } catch (...) {
    }
    return boost::none;
  }
};
