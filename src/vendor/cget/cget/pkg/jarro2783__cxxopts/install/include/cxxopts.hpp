/*

Copyright (c) 2014-2022 Jarryd Beck

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// vim: ts=2:sw=2:expandtab

#ifndef CXXOPTS_HPP_INCLUDED
#define CXXOPTS_HPP_INCLUDED

#include <cstring>
#include <exception>
#include <limits>
#include <initializer_list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <algorithm>
#include <locale>

#ifdef CXXOPTS_NO_EXCEPTIONS
#include <iostream>
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  if (__GNUC__ * 10 + __GNUC_MINOR__) < 49
#    define CXXOPTS_NO_REGEX true
#  endif
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define CXXOPTS_LINKONCE_CONST	__declspec(selectany) extern
#define CXXOPTS_LINKONCE		__declspec(selectany) extern
#else
#define CXXOPTS_LINKONCE_CONST	
#define CXXOPTS_LINKONCE		
#endif

#ifndef CXXOPTS_NO_REGEX
#  include <regex>
#endif  // CXXOPTS_NO_REGEX

// Nonstandard before C++17, which is coincidentally what we also need for <optional>
#ifdef __has_include
#  if __has_include(<optional>)
#    include <optional>
#    ifdef __cpp_lib_optional
#      define CXXOPTS_HAS_OPTIONAL
#    endif
#  endif
#endif

#if __cplusplus >= 201603L
#define CXXOPTS_NODISCARD [[nodiscard]]
#else
#define CXXOPTS_NODISCARD
#endif

#ifndef CXXOPTS_VECTOR_DELIMITER
#define CXXOPTS_VECTOR_DELIMITER ','
#endif

#define CXXOPTS__VERSION_MAJOR 3
#define CXXOPTS__VERSION_MINOR 1
#define CXXOPTS__VERSION_PATCH 1

#if (__GNUC__ < 10 || (__GNUC__ == 10 && __GNUC_MINOR__ < 1)) && __GNUC__ >= 6
  #define CXXOPTS_NULL_DEREF_IGNORE
#endif

#if defined(__GNUC__)
#define DO_PRAGMA(x) _Pragma(#x)
#define CXXOPTS_DIAGNOSTIC_PUSH DO_PRAGMA(GCC diagnostic push)
#define CXXOPTS_DIAGNOSTIC_POP DO_PRAGMA(GCC diagnostic pop)
#define CXXOPTS_IGNORE_WARNING(x) DO_PRAGMA(GCC diagnostic ignored x)
#else
// define other compilers here if needed
#define CXXOPTS_DIAGNOSTIC_PUSH
#define CXXOPTS_DIAGNOSTIC_POP
#define CXXOPTS_IGNORE_WARNING(x)
#endif

#ifdef CXXOPTS_NO_RTTI
#define CXXOPTS_RTTI_CAST static_cast
#else
#define CXXOPTS_RTTI_CAST dynamic_cast
#endif

namespace cxxopts {
static constexpr struct {
  uint8_t major, minor, patch;
} version = {
  CXXOPTS__VERSION_MAJOR,
  CXXOPTS__VERSION_MINOR,
  CXXOPTS__VERSION_PATCH
};
} // namespace cxxopts

//when we ask cxxopts to use Unicode, help strings are processed using ICU,
//which results in the correct lengths being computed for strings when they
//are formatted for the help output
//it is necessary to make sure that <unicode/unistr.h> can be found by the
//compiler, and that icu-uc is linked in to the binary.

#ifdef CXXOPTS_USE_UNICODE
#include <unicode/unistr.h>

namespace cxxopts {

using String = icu::UnicodeString;

inline
String
toLocalString(std::string s)
{
  return icu::UnicodeString::fromUTF8(std::move(s));
}

// GNU GCC with -Weffc++ will issue a warning regarding the upcoming class, we want to silence it:
// warning: base class 'class std::enable_shared_from_this<cxxopts::Value>' has accessible non-virtual destructor
CXXOPTS_DIAGNOSTIC_PUSH
CXXOPTS_IGNORE_WARNING("-Wnon-virtual-dtor")
// This will be ignored under other compilers like LLVM clang.
class UnicodeStringIterator
{
  public:

  using iterator_category = std::forward_iterator_tag;
  using value_type = int32_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  UnicodeStringIterator(const icu::UnicodeString* string, int32_t pos)
  : s(string)
  , i(pos)
  {
  }

  value_type
  operator*() const
  {
    return s->char32At(i);
  }

  bool
  operator==(const UnicodeStringIterator& rhs) const
  {
    return s == rhs.s && i == rhs.i;
  }

  bool
  operator!=(const UnicodeStringIterator& rhs) const
  {
    return !(*this == rhs);
  }

  UnicodeStringIterator&
  operator++()
  {
    ++i;
    return *this;
  }

  UnicodeStringIterator
  operator+(int32_t v)
  {
    return UnicodeStringIterator(s, i + v);
  }

  private:
  const icu::UnicodeString* s;
  int32_t i;
};
CXXOPTS_DIAGNOSTIC_POP

inline
String&
stringAppend(String&s, String a)
{
  return s.append(std::move(a));
}

inline
String&
stringAppend(String& s, std::size_t n, UChar32 c)
{
  for (std::size_t i = 0; i != n; ++i)
  {
    s.append(c);
  }

  return s;
}

template <typename Iterator>
String&
stringAppend(String& s, Iterator begin, Iterator end)
{
  while (begin != end)
  {
    s.append(*begin);
    ++begin;
  }

  return s;
}

inline
std::size_t
stringLength(const String& s)
{
  return s.length();
}

inline
std::string
toUTF8String(const String& s)
{
  std::string result;
  s.toUTF8String(result);

  return result;
}

inline
bool
empty(const String& s)
{
  return s.isEmpty();
}

} // namespace cxxopts

namespace std {

inline
cxxopts::UnicodeStringIterator
begin(const icu::UnicodeString& s)
{
  return cxxopts::UnicodeStringIterator(&s, 0);
}

inline
cxxopts::UnicodeStringIterator
end(const icu::UnicodeString& s)
{
  return cxxopts::UnicodeStringIterator(&s, s.length());
}

} // namespace std

//ifdef CXXOPTS_USE_UNICODE
#else

namespace cxxopts {

using String = std::string;

template <typename T>
T
toLocalString(T&& t)
{
  return std::forward<T>(t);
}

inline
std::size_t
stringLength(const String& s)
{
  return s.length();
}

inline
String&
stringAppend(String&s, const String& a)
{
  return s.append(a);
}

inline
String&
stringAppend(String& s, std::size_t n, char c)
{
  return s.append(n, c);
}

template <typename Iterator>
String&
stringAppend(String& s, Iterator begin, Iterator end)
{
  return s.append(begin, end);
}

template <typename T>
std::string
toUTF8String(T&& t)
{
  return std::forward<T>(t);
}

inline
bool
empty(const std::string& s)
{
  return s.empty();
}

} // namespace cxxopts

//ifdef CXXOPTS_USE_UNICODE
#endif

namespace cxxopts {

namespace {
#ifdef _WIN32
CXXOPTS_LINKONCE_CONST std::string LQUOTE("\'");
CXXOPTS_LINKONCE_CONST std::string RQUOTE("\'");
#else
CXXOPTS_LINKONCE_CONST std::string LQUOTE("‘");
CXXOPTS_LINKONCE_CONST std::string RQUOTE("’");
#endif
} // namespace

// GNU GCC with -Weffc++ will issue a warning regarding the upcoming class, we
// want to silence it: warning: base class 'class
// std::enable_shared_from_this<cxxopts::Value>' has accessible non-virtual
// destructor This will be ignored under other compilers like LLVM clang.
CXXOPTS_DIAGNOSTIC_PUSH
CXXOPTS_IGNORE_WARNING("-Wnon-virtual-dtor")

// some older versions of GCC warn under this warning
CXXOPTS_IGNORE_WARNING("-Weffc++")
class Value : public std::enable_shared_from_this<Value>
{
  public:

  virtual ~Value() = default;

  virtual
  std::shared_ptr<Value>
  clone() const = 0;

  virtual void
  parse(const std::string& text) const = 0;

  virtual void
  parse() const = 0;

  virtual bool
  has_default() const = 0;

  virtual bool
  is_container() const = 0;

  virtual bool
  has_implicit() const = 0;

  virtual std::string
  get_default_value() const = 0;

  virtual std::string
  get_implicit_value() const = 0;

  virtual std::shared_ptr<Value>
  default_value(const std::string& value) = 0;

  virtual std::shared_ptr<Value>
  implicit_value(const std::string& value) = 0;

  virtual std::shared_ptr<Value>
  no_implicit_value() = 0;

  virtual bool
  is_boolean() const = 0;
};

CXXOPTS_DIAGNOSTIC_POP

namespace exceptions {

class exception : public std::exception
{
  public:
  explicit exception(std::string  message)
  : m_message(std::move(message))
  {
  }

  CXXOPTS_NODISCARD
  const char*
  what() const noexcept override
  {
    return m_message.c_str();
  }

  private:
  std::string m_message;
};

class specification : public exception
{
  public:

  explicit specification(const std::string& message)
  : exception(message)
  {
  }
};

class parsing : public exception
{
  public:
  explicit parsing(const std::string& message)
  : exception(message)
  {
  }
};

class option_already_exists : public specification
{
  public:
  explicit option_already_exists(const std::string& option)
  : specification("Option " + LQUOTE + option + RQUOTE + " already exists")
  {
  }
};

class invalid_option_format : public specification
{
  public:
  explicit invalid_option_format(const std::string& format)
  : specification("Invalid option format " + LQUOTE + format + RQUOTE)
  {
  }
};

class invalid_option_syntax : public parsing {
  public:
  explicit invalid_option_syntax(const std::string& text)
  : parsing("Argument " + LQUOTE + text + RQUOTE +
            " starts with a - but has incorrect syntax")
  {
  }
};

class no_such_option : public parsing
{
  public:
  explicit no_such_option(const std::string& option)
  : parsing("Option " + LQUOTE + option + RQUOTE + " does not exist")
  {
  }
};

class missing_argument : public parsing
{
  public:
  explicit missing_argument(const std::string& option)
  : parsing(
      "Option " + LQUOTE + option + RQUOTE + " is missing an argument"
    )
  {
  }
};

class option_requires_argument : public parsing
{
  public:
  explicit option_requires_argument(const std::string& option)
  : parsing(
      "Option " + LQUOTE + option + RQUOTE + " requires an argument"
    )
  {
  }
};

class gratuitous_argument_for_option : public parsing
{
  public:
  gratuitous_argument_for_option
  (
    const std::string& option,
    const std::string& arg
  )
  : parsing(
      "Option " + LQUOTE + option + RQUOTE +
      " does not take an argument, but argument " +
      LQUOTE + arg + RQUOTE + " given"
    )
  {
  }
};

class requested_option_not_present : public parsing
{
  public:
  explicit requested_option_not_present(const std::string& option)
  : parsing("Option " + LQUOTE + option + RQUOTE + " not present")
  {
  }
};

class option_has_no_value : public exception
{
  public:
  explicit option_has_no_value(const std::string& option)
  : exception(
      !option.empty() ?
      ("Option " + LQUOTE + option + RQUOTE + " has no value") :
      "Option has no value")
  {
  }
};

class incorrect_argument_type : public parsing
{
  public:
  explicit incorrect_argument_type
  (
    const std::string& arg
  )
  : parsing(
      "Argument " + LQUOTE + arg + RQUOTE + " failed to parse"
    )
  {
  }
};

} // namespace exceptions


template <typename T>
void throw_or_mimic(const std::string& text)
{
  static_assert(std::is_base_of<std::exception, T>::value,
                "throw_or_mimic only works on std::exception and "
                "deriving classes");

#ifndef CXXOPTS_NO_EXCEPTIONS
  // If CXXOPTS_NO_EXCEPTIONS is not defined, just throw
  throw T{text};
#else
  // Otherwise manually instantiate the exception, print what() to stderr,
  // and exit
  T exception{text};
  std::cerr << exception.what() << std::endl;
  std::exit(EXIT_FAILURE);
#endif
}

using OptionNames = std::vector<std::string>;

namespace values {

namespace parser_tool {

struct IntegerDesc
{
  std::string negative = "";
  std::string base     = "";
  std::string value    = "";
};
struct ArguDesc {
  std::string arg_name  = "";
  bool        grouping  = false;
  bool        set_value = false;
  std::string value     = "";
};

#ifdef CXXOPTS_NO_REGEX
inline IntegerDesc SplitInteger(const std::string &text)
{
  if (text.empty())
  {
    throw_or_mimic<exceptions::incorrect_argument_type>(text);
  }
  IntegerDesc desc;
  const char *pdata = text.c_str();
  if (*pdata == '-')
  {
    pdata += 1;
    desc.negative = "-";
  }
  if (strncmp(pdata, "0x", 2) == 0)
  {
    pdata += 2;
    desc.base = "0x";
  }
  if (*pdata != '\0')
  {
    desc.value = std::string(pdata);
  }
  else
  {
    throw_or_mimic<exceptions::incorrect_argument_type>(text);
  }
  return desc;
}

inline bool IsTrueText(const std::string &text)
{
  const char *pdata = text.c_str();
  if (*pdata == 't' || *pdata == 'T')
  {
    pdata += 1;
    if (strncmp(pdata, "rue\0", 4) == 0)
    {
      return true;
    }
  }
  else if (strncmp(pdata, "1\0", 2) == 0)
  {
    return true;
  }
  return false;
}

inline bool IsFalseText(const std::string &text)
{
  const char *pdata = text.c_str();
  if (*pdata == 'f' || *pdata == 'F')
  {
    pdata += 1;
    if (strncmp(pdata, "alse\0", 5) == 0)
    {
      return true;
    }
  }
  else if (strncmp(pdata, "0\0", 2) == 0)
  {
    return true;
  }
  return false;
}

inline OptionNames split_option_names(const std::string &text)
{
  OptionNames split_names;

  std::string::size_type token_start_pos = 0;
  auto length = text.length();

  if (length == 0)
  {
    throw_or_mimic<exceptions::invalid_option_format>(text);
  }

  while (token_start_pos < length) {
    const auto &npos = std::string::npos;
    auto next_non_space_pos = text.find_first_not_of(' ', token_start_pos);
    if (next_non_space_pos == npos) {
      throw_or_mimic<exceptions::invalid_option_format>(text);
    }
    token_start_pos = next_non_space_pos;
    auto next_delimiter_pos = text.find(',', token_start_pos);
    if (next_delimiter_pos == token_start_pos) {
      throw_or_mimic<exceptions::invalid_option_format>(text);
    }
    if (next_delimiter_pos == npos) {
      next_delimiter_pos = length;
    }
    auto token_length = next_delimiter_pos - token_start_pos;
    // validate the token itself matches the regex /([:alnum:][-_[:alnum:]]*/
    {
      const char* option_name_valid_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "_-.?";

      if (!std::isalnum(text[token_start_pos], std::locale::classic()) ||
          text.find_first_not_of(option_name_valid_chars, token_start_pos) < next_delimiter_pos) {
        throw_or_mimic<exceptions::invalid_option_format>(text);
      }
    }
    split_names.emplace_back(text.substr(token_start_pos, token_length));
    token_start_pos = next_delimiter_pos + 1;
  }
  return split_names;
}

inline ArguDesc ParseArgument(const char *arg, bool &matched)
{
  ArguDesc argu_desc;
  const char *pdata = arg;
  matched = false;
  if (strncmp(pdata, "--", 2) == 0)
  {
    pdata += 2;
    if (isalnum(*pdata, std::locale::classic()))
    {
      argu_desc.arg_name.push_back(*pdata);
      pdata += 1;
      while (isalnum(*pdata, std::locale::classic()) || *pdata == '-' || *pdata == '_')
      {
        argu_desc.arg_name.push_back(*pdata);
        pdata += 1;
      }
      if (argu_desc.arg_name.length() > 1)
      {
        if (*pdata == '=')
        {
          argu_desc.set_value = true;
          pdata += 1;
          if (*pdata != '\0')
          {
            argu_desc.value = std::string(pdata);
          }
          matched = true;
        }
        else if (*pdata == '\0')
        {
          matched = true;
        }
      }
    }
  }
  else if (strncmp(pdata, "-", 1) == 0)
  {
    pdata += 1;
    argu_desc.grouping = true;
    while (isalnum(*pdata, std::locale::classic()))
    {
      argu_desc.arg_name.push_back(*pdata);
      pdata += 1;
    }
    matched = !argu_desc.arg_name.empty() && *pdata == '\0';
  }
  return argu_desc;
}

#else  // CXXOPTS_NO_REGEX

namespace {
CXXOPTS_LINKONCE
std::basic_regex<char> integer_pattern
  ("(-)?(0x)?([0-9a-zA-Z]+)|((0x)?0)");
CXXOPTS_LINKONCE
std::basic_regex<char> truthy_pattern
  ("(t|T)(rue)?|1");
CXXOPTS_LINKONCE
std::basic_regex<char> falsy_pattern
  ("(f|F)(alse)?|0");
CXXOPTS_LINKONCE
std::basic_regex<char> option_matcher
  ("--([[:alnum:]][-_[:alnum:]\\.]+)(=(.*))?|-([[:alnum:]].*)");
CXXOPTS_LINKONCE
std::basic_regex<char> option_specifier
  ("([[:alnum:]][-_[:alnum:]\\.]*)(,[ ]*[[:alnum:]][-_[:alnum:]]*)*");
CXXOPTS_LINKONCE
std::basic_regex<char> option_specifier_separator(", *");

} // namespace

inline IntegerDesc SplitInteger(const std::string &text)
{
  std::smatch match;
  std::regex_match(text, match, integer_pattern);

  if (match.length() == 0)
  {
    throw_or_mimic<exceptions::incorrect_argument_type>(text);
  }

  IntegerDesc desc;
  desc.negative = match[1];
  desc.base = match[2];
  desc.value = match[3];

  if (match.length(4) > 0)
  {
    desc.base = match[5];
    desc.value = "0";
    return desc;
  }

  return desc;
}

inline bool IsTrueText(const std::string &text)
{
  std::smatch result;
  std::regex_match(text, result, truthy_pattern);
  return !result.empty();
}

inline bool IsFalseText(const std::string &text)
{
  std::smatch result;
  std::regex_match(text, result, falsy_pattern);
  return !result.empty();
}

// Gets the option names specified via a single, comma-separated string,
// and returns the separate, space-discarded, non-empty names
// (without considering which or how many are single-character)
inline OptionNames split_option_names(const std::string &text)
{
  if (!std::regex_match(text.c_str(), option_specifier))
  {
    throw_or_mimic<exceptions::invalid_option_format>(text);
  }

  OptionNames split_names;

  constexpr int use_non_matches { -1 };
  auto token_iterator = std::sregex_token_iterator(
    text.begin(), text.end(), option_specifier_separator, use_non_matches);
  std::copy(token_iterator, std::sregex_token_iterator(), std::back_inserter(split_names));
  return split_names;
}

inline ArguDesc ParseArgument(const char *arg, bool &matched)
{
  std::match_results<const char*> result;
  std::regex_match(arg, result, option_matcher);
  matched = !result.empty();

  ArguDesc argu_desc;
  if (matched) {
    argu_desc.arg_name = result[1].str();
    argu_desc.set_value = result[2].length() > 0;
    argu_desc.value = result[3].str();
    if (result[4].length() > 0)
    {
      argu_desc.grouping = true;
      argu_desc.arg_name = result[4].str();
    }
  }

  return argu_desc;
}

#endif  // CXXOPTS_NO_REGEX
#undef CXXOPTS_NO_REGEX
} // namespace parser_tool

namespace detail {

template <typename T, bool B>
struct SignedCheck;

template <typename T>
struct SignedCheck<T, true>
{
  template <typename U>
  void
  operator()(bool negative, U u, const std::string& text)
  {
    if (negative)
    {
      if (u > static_cast<U>((std::numeric_limits<T>::min)()))
      {
        throw_or_mimic<exceptions::incorrect_argument_type>(text);
      }
    }
    else
    {
      if (u > static_cast<U>((std::numeric_limits<T>::max)()))
      {
        throw_or_mimic<exceptions::incorrect_argument_type>(text);
      }
    }
  }
};

template <typename T>
struct SignedCheck<T, false>
{
  template <typename U>
  void
  operator()(bool, U, const std::string&) const {}
};

template <typename T, typename U>
void
check_signed_range(bool negative, U value, const std::string& text)
{
  SignedCheck<T, std::numeric_limits<T>::is_signed>()(negative, value, text);
}

} // namespace detail

template <typename R, typename T>
void
checked_negate(R& r, T&& t, const std::string&, std::true_type)
{
  // if we got to here, then `t` is a positive number that fits into
  // `R`. So to avoid MSVC C4146, we first cast it to `R`.
  // See https://github.com/jarro2783/cxxopts/issues/62 for more details.
  r = static_cast<R>(-static_cast<R>(t-1)-1);
}

template <typename R, typename T>
void
checked_negate(R&, T&&, const std::string& text, std::false_type)
{
  throw_or_mimic<exceptions::incorrect_argument_type>(text);
}

template <typename T>
void
integer_parser(const std::string& text, T& value)
{
  parser_tool::IntegerDesc int_desc = parser_tool::SplitInteger(text);

  using US = typename std::make_unsigned<T>::type;
  constexpr bool is_signed = std::numeric_limits<T>::is_signed;

  const bool          negative    = int_desc.negative.length() > 0;
  const uint8_t       base        = int_desc.base.length() > 0 ? 16 : 10;
  const std::string & value_match = int_desc.value;

  US result = 0;

  for (char ch : value_match)
  {
    US digit = 0;

    if (ch >= '0' && ch <= '9')
    {
      digit = static_cast<US>(ch - '0');
    }
    else if (base == 16 && ch >= 'a' && ch <= 'f')
    {
      digit = static_cast<US>(ch - 'a' + 10);
    }
    else if (base == 16 && ch >= 'A' && ch <= 'F')
    {
      digit = static_cast<US>(ch - 'A' + 10);
    }
    else
    {
      throw_or_mimic<exceptions::incorrect_argument_type>(text);
    }

    const US next = static_cast<US>(result * base + digit);
    if (result > next)
    {
      throw_or_mimic<exceptions::incorrect_argument_type>(text);
    }

    result = next;
  }

  detail::check_signed_range<T>(negative, result, text);

  if (negative)
  {
    checked_negate<T>(value, result, text, std::integral_constant<bool, is_signed>());
  }
  else
  {
    value = static_cast<T>(result);
  }
}

template <typename T>
void stringstream_parser(const std::string& text, T& value)
{
  std::stringstream in(text);
  in >> value;
  if (!in) {
    throw_or_mimic<exceptions::incorrect_argument_type>(text);
  }
}

template <typename T,
         typename std::enable_if<std::is_integral<T>::value>::type* = nullptr
         >
void parse_value(const std::string& text, T& value)
{
    integer_parser(text, value);
}

inline
void
parse_value(const std::string& text, bool& value)
{
  if (parser_tool::IsTrueText(text))
  {
    value = true;
    return;
  }

  if (parser_tool::IsFalseText(text))
  {
    value = false;
    return;
  }

  throw_or_mimic<exceptions::incorrect_argument_type>(text);
}

inline
void
parse_value(const std::string& text, std::string& value)
{
  value = text;
}

// The fallback parser. It uses the stringstream parser to parse all types
// that have not been overloaded explicitly.  It has to be placed in the
// source code before all other more specialized templates.
template <typename T,
         typename std::enable_if<!std::is_integral<T>::value>::type* = nullptr
         >
void
parse_value(const std::string& text, T& value) {
  stringstream_parser(text, value);
}

template <typename T>
void
parse_value(const std::string& text, std::vector<T>& value)
{
  if (text.empty()) {
    T v;
    parse_value(text, v);
    value.emplace_back(std::move(v));
    return;
  }
  std::stringstream in(text);
  std::string token;
  while(!in.eof() && std::getline(in, token, CXXOPTS_VECTOR_DELIMITER)) {
    T v;
    parse_value(token, v);
    value.emplace_back(std::move(v));
  }
}

#ifdef CXXOPTS_HAS_OPTIONAL
template <typename T>
void
parse_value(const std::string& text, std::optional<T>& value)
{
  T result;
  parse_value(text, result);
  value = std::move(result);
}
#endif

inline
void parse_value(const std::string& text, char& c)
{
  if (text.length() != 1)
  {
    throw_or_mimic<exceptions::incorrect_argument_type>(text);
  }

  c = text[0];
}

template <typename T>
struct type_is_container
{
  static constexpr bool value = false;
};

template <typename T>
struct type_is_container<std::vector<T>>
{
  static constexpr bool value = true;
};

template <typename T>
class abstract_value : public Value
{
  using Self = abstract_value<T>;

  public:
  abstract_value()
  : m_result(std::make_shared<T>())
  , m_store(m_result.get())
  {
  }

  explicit abstract_value(T* t)
  : m_store(t)
  {
  }

  ~abstract_value() override = default;

  abstract_value& operator=(const abstract_value&) = default;

  abstract_value(const abstract_value& rhs)
  {
    if (rhs.m_result)
    {
      m_result = std::make_shared<T>();
      m_store = m_result.get();
    }
    else
    {
      m_store = rhs.m_store;
    }

    m_default = rhs.m_default;
    m_implicit = rhs.m_implicit;
    m_default_value = rhs.m_default_value;
    m_implicit_value = rhs.m_implicit_value;
  }

  void
  parse(const std::string& text) const override
  {
    parse_value(text, *m_store);
  }

  bool
  is_container() const override
  {
    return type_is_container<T>::value;
  }

  void
  parse() const override
  {
    parse_value(m_default_value, *m_store);
  }

  bool
  has_default() const override
  {
    return m_default;
  }

  bool
  has_implicit() const override
  {
    return m_implicit;
  }

  std::shared_ptr<Value>
  default_value(const std::string& value) override
  {
    m_default = true;
    m_default_value = value;
    return shared_from_this();
  }

  std::shared_ptr<Value>
  implicit_value(const std::string& value) override
  {
    m_implicit = true;
    m_implicit_value = value;
    return shared_from_this();
  }

  std::shared_ptr<Value>
  no_implicit_value() override
  {
    m_implicit = false;
    return shared_from_this();
  }

  std::string
  get_default_value() const override
  {
    return m_default_value;
  }

  std::string
  get_implicit_value() const override
  {
    return m_implicit_value;
  }

  bool
  is_boolean() const override
  {
    return std::is_same<T, bool>::value;
  }

  const T&
  get() const
  {
    if (m_store == nullptr)
    {
      return *m_result;
    }
    return *m_store;
  }

  protected:
  std::shared_ptr<T> m_result{};
  T* m_store{};

  bool m_default = false;
  bool m_implicit = false;

  std::string m_default_value{};
  std::string m_implicit_value{};
};

template <typename T>
class standard_value : public abstract_value<T>
{
  public:
  using abstract_value<T>::abstract_value;

  CXXOPTS_NODISCARD
  std::shared_ptr<Value>
  clone() const override
  {
    return std::make_shared<standard_value<T>>(*this);
  }
};

template <>
class standard_value<bool> : public abstract_value<bool>
{
  public:
  ~standard_value() override = default;

  standard_value()
  {
    set_default_and_implicit();
  }

  explicit standard_value(bool* b)
  : abstract_value(b)
  {
    m_implicit = true;
    m_implicit_value = "true";
  }

  std::shared_ptr<Value>
  clone() const override
  {
    return std::make_shared<standard_value<bool>>(*this);
  }

  private:

  void
  set_default_and_implicit()
  {
    m_default = true;
    m_default_value = "false";
    m_implicit = true;
    m_implicit_value = "true";
  }
};

} // namespace values

template <typename T>
std::shared_ptr<Value>
value()
{
  return std::make_shared<values::standard_value<T>>();
}

template <typename T>
std::shared_ptr<Value>
value(T& t)
{
  return std::make_shared<values::standard_value<T>>(&t);
}

class OptionAdder;

CXXOPTS_NODISCARD
inline
const std::string&
first_or_empty(const OptionNames& long_names)
{
  static const std::string empty{""};
  return long_names.empty() ? empty : long_names.front();
}

class OptionDetails
{
  public:
  OptionDetails
  (
    std::string short_,
    OptionNames long_,
    String desc,
    std::shared_ptr<const Value> val
  )
  : m_short(std::move(short_))
  , m_long(std::move(long_))
  , m_desc(std::move(desc))
  , m_value(std::move(val))
  , m_count(0)
  {
    m_hash = std::hash<std::string>{}(first_long_name() + m_short);
  }

  OptionDetails(const OptionDetails& rhs)
  : m_desc(rhs.m_desc)
  , m_value(rhs.m_value->clone())
  , m_count(rhs.m_count)
  {
  }

  OptionDetails(OptionDetails&& rhs) = default;

  CXXOPTS_NODISCARD
  const String&
  description() const
  {
    return m_desc;
  }

  CXXOPTS_NODISCARD
  const Value&
  value() const {
      return *m_value;
  }

  CXXOPTS_NODISCARD
  std::shared_ptr<Value>
  make_storage() const
  {
    return m_value->clone();
  }

  CXXOPTS_NODISCARD
  const std::string&
  short_name() const
  {
    return m_short;
  }

  CXXOPTS_NODISCARD
  const std::string&
  first_long_name() const
  {
    return first_or_empty(m_long);
  }

  CXXOPTS_NODISCARD
  const std::string&
  essential_name() const
  {
    return m_long.empty() ? m_short : m_long.front();
  }

  CXXOPTS_NODISCARD
  const OptionNames &
  long_names() const
  {
    return m_long;
  }

  std::size_t
  hash() const
  {
    return m_hash;
  }

  private:
  std::string m_short{};
  OptionNames m_long{};
  String m_desc{};
  std::shared_ptr<const Value> m_value{};
  int m_count;

  std::size_t m_hash{};
};

struct HelpOptionDetails
{
  std::string s;
  OptionNames l;
  String desc;
  bool has_default;
  std::string default_value;
  bool has_implicit;
  std::string implicit_value;
  std::string arg_help;
  bool is_container;
  bool is_boolean;
};

struct HelpGroupDetails
{
  std::string name{};
  std::string description{};
  std::vector<HelpOptionDetails> options{};
};

class OptionValue
{
  public:
  void
  parse
  (
    const std::shared_ptr<const OptionDetails>& details,
    const std::string& text
  )
  {
    ensure_value(details);
    ++m_count;
    m_value->parse(text);
    m_long_names = &details->long_names();
  }

  void
  parse_default(const std::shared_ptr<const OptionDetails>& details)
  {
    ensure_value(details);
    m_default = true;
    m_long_names = &details->long_names();
    m_value->parse();
  }

  void
  parse_no_value(const std::shared_ptr<const OptionDetails>& details)
  {
    m_long_names = &details->long_names();
  }

#if defined(CXXOPTS_NULL_DEREF_IGNORE)
CXXOPTS_DIAGNOSTIC_PUSH
CXXOPTS_IGNORE_WARNING("-Wnull-dereference")
#endif

  CXXOPTS_NODISCARD
  std::size_t
  count() const noexcept
  {
    return m_count;
  }

#if defined(CXXOPTS_NULL_DEREF_IGNORE)
CXXOPTS_DIAGNOSTIC_POP
#endif

  // TODO: maybe default options should count towards the number of arguments
  CXXOPTS_NODISCARD
  bool
  has_default() const noexcept
  {
    return m_default;
  }

  template <typename T>
  const T&
  as() const
  {
    if (m_value == nullptr) {
        throw_or_mimic<exceptions::option_has_no_value>(
            m_long_names == nullptr ? "" : first_or_empty(*m_long_names));
    }

    return CXXOPTS_RTTI_CAST<const values::standard_value<T>&>(*m_value).get();
  }

  private:
  void
  ensure_value(const std::shared_ptr<const OptionDetails>& details)
  {
    if (m_value == nullptr)
    {
      m_value = details->make_storage();
    }
  }


  const OptionNames * m_long_names = nullptr;
  // Holding this pointer is safe, since OptionValue's only exist in key-value pairs,
  // where the key has the string we point to.
  std::shared_ptr<Value> m_value{};
  std::size_t m_count = 0;
  bool m_default = false;
};

class KeyValue
{
  public:
  KeyValue(std::string key_, std::string value_)
  : m_key(std::move(key_))
  , m_value(std::move(value_))
  {
  }

  CXXOPTS_NODISCARD
  const std::string&
  key() const
  {
    return m_key;
  }

  CXXOPTS_NODISCARD
  const std::string&
  value() const
  {
    return m_value;
  }

  template <typename T>
  T
  as() const
  {
    T result;
    values::parse_value(m_value, result);
    return result;
  }

  private:
  std::string m_key;
  std::string m_value;
};

using ParsedHashMap = std::unordered_map<std::size_t, OptionValue>;
using NameHashMap = std::unordered_map<std::string, std::size_t>;

class ParseResult
{
  public:
  class Iterator
  {
    public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = KeyValue;
    using difference_type = void;
    using pointer = const KeyValue*;
    using reference = const KeyValue&;

    Iterator() = default;
    Iterator(const Iterator&) = default;

// GCC complains about m_iter not being initialised in the member
// initializer list 
CXXOPTS_DIAGNOSTIC_PUSH
CXXOPTS_IGNORE_WARNING("-Weffc++")
    Iterator(const ParseResult *pr, bool end=false)
    : m_pr(pr)
    {
      if (end)
      {
        m_sequential = false;
        m_iter = m_pr->m_defaults.end();
      }
      else
      {
        m_sequential = true;
        m_iter = m_pr->m_sequential.begin();

        if (m_iter == m_pr->m_sequential.end())
        {
          m_sequential = false;
          m_iter = m_pr->m_defaults.begin();
        }
      }
    }
CXXOPTS_DIAGNOSTIC_POP

    Iterator& operator++()
    {
      ++m_iter;
      if(m_sequential && m_iter == m_pr->m_sequential.end())
      {
        m_sequential = false;
        m_iter = m_pr->m_defaults.begin();
        return *this;
      }
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator retval = *this;
      ++(*this);
      return retval;
    }

    bool operator==(const Iterator& other) const
    {
      return (m_sequential == other.m_sequential) && (m_iter == other.m_iter);
    }

    bool operator!=(const Iterator& other) const
    {
      return !(*this == other);
    }

    const KeyValue& operator*()
    {
      return *m_iter;
    }

    const KeyValue* operator->()
    {
      return m_iter.operator->();
    }

    private:
    const ParseResult* m_pr;
    std::vector<KeyValue>::const_iterator m_iter;
    bool m_sequential = true;
  };

  ParseResult() = default;
  ParseResult(const ParseResult&) = default;

  ParseResult(NameHashMap&& keys, ParsedHashMap&& values, std::vector<KeyValue> sequential,
          std::vector<KeyValue> default_opts, std::vector<std::string>&& unmatched_args)
  : m_keys(std::move(keys))
  , m_values(std::move(values))
  , m_sequential(std::move(sequential))
  , m_defaults(std::move(default_opts))
  , m_unmatched(std::move(unmatched_args))
  {
  }

  ParseResult& operator=(ParseResult&&) = default;
  ParseResult& operator=(const ParseResult&) = default;

  Iterator
  begin() const
  {
    return Iterator(this);
  }

  Iterator
  end() const
  {
    return Iterator(this, true);
  }

  std::size_t
  count(const std::string& o) const
  {
    auto iter = m_keys.find(o);
    if (iter == m_keys.end())
    {
      return 0;
    }

    auto viter = m_values.find(iter->second);

    if (viter == m_values.end())
    {
      return 0;
    }

    return viter->second.count();
  }

  const OptionValue&
  operator[](const std::string& option) const
  {
    auto iter = m_keys.find(option);

    if (iter == m_keys.end())
    {
      throw_or_mimic<exceptions::requested_option_not_present>(option);
    }

    auto viter = m_values.find(iter->second);

    if (viter == m_values.end())
    {
      throw_or_mimic<exceptions::requested_option_not_present>(option);
    }

    return viter->second;
  }

  const std::vector<KeyValue>&
  arguments() const
  {
    return m_sequential;
  }

  const std::vector<std::string>&
  unmatched() const
  {
    return m_unmatched;
  }

  const std::vector<KeyValue>&
  defaults() const
  {
    return m_defaults;
  }

  const std::string
  arguments_string() const
  {
    std::string result;
    for(const auto& kv: m_sequential)
    {
      result += kv.key() + " = " + kv.value() + "\n";
    }
    for(const auto& kv: m_defaults)
    {
      result += kv.key() + " = " + kv.value() + " " + "(default)" + "\n";
    }
    return result;
  }

  private:
  NameHashMap m_keys{};
  ParsedHashMap m_values{};
  std::vector<KeyValue> m_sequential{};
  std::vector<KeyValue> m_defaults{};
  std::vector<std::string> m_unmatched{};
};

struct Option
{
  Option
  (
    std::string opts,
    std::string desc,
    std::shared_ptr<const Value>  value = ::cxxopts::value<bool>(),
    std::string arg_help = ""
  )
  : opts_(std::move(opts))
  , desc_(std::move(desc))
  , value_(std::move(value))
  , arg_help_(std::move(arg_help))
  {
  }

  std::string opts_;
  std::string desc_;
  std::shared_ptr<const Value> value_;
  std::string arg_help_;
};

using OptionMap = std::unordered_map<std::string, std::shared_ptr<OptionDetails>>;
using PositionalList = std::vector<std::string>;
using PositionalListIterator = PositionalList::const_iterator;

class OptionParser
{
  public:
  OptionParser(const OptionMap& options, const PositionalList& positional, bool allow_unrecognised)
  : m_options(options)
  , m_positional(positional)
  , m_allow_unrecognised(allow_unrecognised)
  {
  }

  ParseResult
  parse(int argc, const char* const* argv);

  bool
  consume_positional(const std::string& a, PositionalListIterator& next);

  void
  checked_parse_arg
  (
    int argc,
    const char* const* argv,
    int& current,
    const std::shared_ptr<OptionDetails>& value,
    const std::string& name
  );

  void
  add_to_option(OptionMap::const_iterator iter, const std::string& option, const std::string& arg);

  void
  parse_option
  (
    const std::shared_ptr<OptionDetails>& value,
    const std::string& name,
    const std::string& arg = ""
  );

  void
  parse_default(const std::shared_ptr<OptionDetails>& details);

  void
  parse_no_value(const std::shared_ptr<OptionDetails>& details);

  private:

  void finalise_aliases();

  const OptionMap& m_options;
  const PositionalList& m_positional;

  std::vector<KeyValue> m_sequential{};
  std::vector<KeyValue> m_defaults{};
  bool m_allow_unrecognised;

  ParsedHashMap m_parsed{};
  NameHashMap m_keys{};
};

class Options
{
  public:

  explicit Options(std::string program_name, std::string help_string = "")
  : m_program(std::move(program_name))
  , m_help_string(toLocalString(std::move(help_string)))
  , m_custom_help("[OPTION...]")
  , m_positional_help("positional parameters")
  , m_show_positional(false)
  , m_allow_unrecognised(false)
  , m_width(76)
  , m_tab_expansion(false)
  , m_options(std::make_shared<OptionMap>())
  {
  }

  Options&
  positional_help(std::string help_text)
  {
    m_positional_help = std::move(help_text);
    return *this;
  }

  Options&
  custom_help(std::string help_text)
  {
    m_custom_help = std::move(help_text);
    return *this;
  }

  Options&
  show_positional_help()
  {
    m_show_positional = true;
    return *this;
  }

  Options&
  allow_unrecognised_options()
  {
    m_allow_unrecognised = true;
    return *this;
  }

  Options&
  set_width(std::size_t width)
  {
    m_width = width;
    return *this;
  }

  Options&
  set_tab_expansion(bool expansion=true)
  {
    m_tab_expansion = expansion;
    return *this;
  }

  ParseResult
  parse(int argc, const char* const* argv);

  OptionAdder
  add_options(std::string group = "");

  void
  add_options
  (
    const std::string& group,
    std::initializer_list<Option> options
  );

  void
  add_option
  (
    const std::string& group,
    const Option& option
  );

  void
  add_option
  (
    const std::string& group,
    const std::string& s,
    const OptionNames& l,
    std::string desc,
    const std::shared_ptr<const Value>& value,
    std::string arg_help
  );

  void
  add_option
  (
    const std::string& group,
    const std::string& short_name,
    const std::string& single_long_name,
    std::string desc,
    const std::shared_ptr<const Value>& value,
    std::string arg_help
  )
  {
    OptionNames long_names;
    long_names.emplace_back(single_long_name);
    add_option(group, short_name, long_names, desc, value, arg_help);
  }

  //parse positional arguments into the given option
  void
  parse_positional(std::string option);

  void
  parse_positional(std::vector<std::string> options);

  void
  parse_positional(std::initializer_list<std::string> options);

  template <typename Iterator>
  void
  parse_positional(Iterator begin, Iterator end) {
    parse_positional(std::vector<std::string>{begin, end});
  }

  std::string
  help(const std::vector<std::string>& groups = {}, bool print_usage=true) const;

  std::vector<std::string>
  groups() const;

  const HelpGroupDetails&
  group_help(const std::string& group) const;

  const std::string& program() const
  {
    return m_program;
  }

  private:

  void
  add_one_option
  (
    const std::string& option,
    const std::shared_ptr<OptionDetails>& details
  );

  String
  help_one_group(const std::string& group) const;

  void
  generate_group_help
  (
    String& result,
    const std::vector<std::string>& groups
  ) const;

  void
  generate_all_groups_help(String& result) const;

  std::string m_program{};
  String m_help_string{};
  std::string m_custom_help{};
  std::string m_positional_help{};
  bool m_show_positional;
  bool m_allow_unrecognised;
  std::size_t m_width;
  bool m_tab_expansion;

  std::shared_ptr<OptionMap> m_options;
  std::vector<std::string> m_positional{};
  std::unordered_set<std::string> m_positional_set{};

  //mapping from groups to help options
  std::map<std::string, HelpGroupDetails> m_help{};
};

class OptionAdder
{
  public:

  OptionAdder(Options& options, std::string group)
  : m_options(options), m_group(std::move(group))
  {
  }

  OptionAdder&
  operator()
  (
    const std::string& opts,
    const std::string& desc,
    const std::shared_ptr<const Value>& value
      = ::cxxopts::value<bool>(),
    std::string arg_help = ""
  );

  private:
  Options& m_options;
  std::string m_group;
};

namespace {
constexpr std::size_t OPTION_LONGEST = 30;
constexpr std::size_t OPTION_DESC_GAP = 2;

String
format_option
(
  const HelpOptionDetails& o
)
{
  const auto& s = o.s;
  const auto& l = first_or_empty(o.l);

  String result = "  ";

  if (!s.empty())
  {
    result += "-" + toLocalString(s);
    if (!l.empty())
    {
      result += ",";
    }
  }
  else
  {
    result += "   ";
  }

  if (!l.empty())
  {
    result += " --" + toLocalString(l);
  }

  auto arg = !o.arg_help.empty() ? toLocalString(o.arg_help) : "arg";

  if (!o.is_boolean)
  {
    if (o.has_implicit)
    {
      result += " [=" + arg + "(=" + toLocalString(o.implicit_value) + ")]";
    }
    else
    {
      result += " " + arg;
    }
  }

  return result;
}

String
format_description
(
  const HelpOptionDetails& o,
  std::size_t start,
  std::size_t allowed,
  bool tab_expansion
)
{
  auto desc = o.desc;

  if (o.has_default && (!o.is_boolean || o.default_value != "false"))
  {
    if(!o.default_value.empty())
    {
      desc += toLocalString(" (default: " + o.default_value + ")");
    }
    else
    {
      desc += toLocalString(" (default: \"\")");
    }
  }

  String result;

  if (tab_expansion)
  {
    String desc2;
    auto size = std::size_t{ 0 };
    for (auto c = std::begin(desc); c != std::end(desc); ++c)
    {
      if (*c == '\n')
      {
        desc2 += *c;
        size = 0;
      }
      else if (*c == '\t')
      {
        auto skip = 8 - size % 8;
        stringAppend(desc2, skip, ' ');
        size += skip;
      }
      else
      {
        desc2 += *c;
        ++size;
      }
    }
    desc = desc2;
  }

  desc += " ";

  auto current = std::begin(desc);
  auto previous = current;
  auto startLine = current;
  auto lastSpace = current;

  auto size = std::size_t{};

  bool appendNewLine;
  bool onlyWhiteSpace = true;

  while (current != std::end(desc))
  {
    appendNewLine = false;
    if (*previous == ' ' || *previous == '\t')
    {
      lastSpace = current;
    }
    if (*current != ' ' && *current != '\t')
    {
      onlyWhiteSpace = false;
    }

    while (*current == '\n')
    {
      previous = current;
      ++current;
      appendNewLine = true;
    }

    if (!appendNewLine && size >= allowed)
    {
      if (lastSpace != startLine)
      {
        current = lastSpace;
        previous = current;
      }
      appendNewLine = true;
    }

    if (appendNewLine)
    {
      stringAppend(result, startLine, current);
      startLine = current;
      lastSpace = current;

      if (*previous != '\n')
      {
        stringAppend(result, "\n");
      }

      stringAppend(result, start, ' ');

      if (*previous != '\n')
      {
        stringAppend(result, lastSpace, current);
      }

      onlyWhiteSpace = true;
      size = 0;
    }

    previous = current;
    ++current;
    ++size;
  }

  //append whatever is left but ignore whitespace
  if (!onlyWhiteSpace)
  {
    stringAppend(result, startLine, previous);
  }

  return result;
}

} // namespace

inline
void
Options::add_options
(
  const std::string &group,
  std::initializer_list<Option> options
)
{
 OptionAdder option_adder(*this, group);
 for (const auto &option: options)
 {
   option_adder(option.opts_, option.desc_, option.value_, option.arg_help_);
 }
}

inline
OptionAdder
Options::add_options(std::string group)
{
  return OptionAdder(*this, std::move(group));
}

inline
OptionAdder&
OptionAdder::operator()
(
  const std::string& opts,
  const std::string& desc,
  const std::shared_ptr<const Value>& value,
  std::string arg_help
)
{
  OptionNames option_names = values::parser_tool::split_option_names(opts);
    // Note: All names will be non-empty; but we must separate the short
    // (length-1) and longer names
  std::string short_name {""};
  auto first_short_name_iter =
    std::partition(option_names.begin(), option_names.end(),
      [&](const std::string& name) { return name.length() > 1; }
    );
  auto num_length_1_names = (option_names.end() - first_short_name_iter);
  switch(num_length_1_names) {
  case 1:
    short_name = *first_short_name_iter;
    option_names.erase(first_short_name_iter);
  case 0:
    break;
  default:
    throw_or_mimic<exceptions::invalid_option_format>(opts);
  };

  m_options.add_option
  (
    m_group,
    short_name,
    option_names,
    desc,
    value,
    std::move(arg_help)
  );

  return *this;
}

inline
void
OptionParser::parse_default(const std::shared_ptr<OptionDetails>& details)
{
  // TODO: remove the duplicate code here
  auto& store = m_parsed[details->hash()];
  store.parse_default(details);
  m_defaults.emplace_back(details->essential_name(), details->value().get_default_value());
}

inline
void
OptionParser::parse_no_value(const std::shared_ptr<OptionDetails>& details)
{
  auto& store = m_parsed[details->hash()];
  store.parse_no_value(details);
}

inline
void
OptionParser::parse_option
(
  const std::shared_ptr<OptionDetails>& value,
  const std::string& /*name*/,
  const std::string& arg
)
{
  auto hash = value->hash();
  auto& result = m_parsed[hash];
  result.parse(value, arg);

  m_sequential.emplace_back(value->essential_name(), arg);
}

inline
void
OptionParser::checked_parse_arg
(
  int argc,
  const char* const* argv,
  int& current,
  const std::shared_ptr<OptionDetails>& value,
  const std::string& name
)
{
  if (current + 1 >= argc)
  {
    if (value->value().has_implicit())
    {
      parse_option(value, name, value->value().get_implicit_value());
    }
    else
    {
      throw_or_mimic<exceptions::missing_argument>(name);
    }
  }
  else
  {
    if (value->value().has_implicit())
    {
      parse_option(value, name, value->value().get_implicit_value());
    }
    else
    {
      parse_option(value, name, argv[current + 1]);
      ++current;
    }
  }
}

inline
void
OptionParser::add_to_option(OptionMap::const_iterator iter, const std::string& option, const std::string& arg)
{
  parse_option(iter->second, option, arg);
}

inline
bool
OptionParser::consume_positional(const std::string& a, PositionalListIterator& next)
{
  while (next != m_positional.end())
  {
    auto iter = m_options.find(*next);
    if (iter != m_options.end())
    {
      if (!iter->second->value().is_container())
      {
        auto& result = m_parsed[iter->second->hash()];
        if (result.count() == 0)
        {
          add_to_option(iter, *next, a);
          ++next;
          return true;
        }
        ++next;
        continue;
      }
      add_to_option(iter, *next, a);
      return true;
    }
    throw_or_mimic<exceptions::no_such_option>(*next);
  }

  return false;
}

inline
void
Options::parse_positional(std::string option)
{
  parse_positional(std::vector<std::string>{std::move(option)});
}

inline
void
Options::parse_positional(std::vector<std::string> options)
{
  m_positional = std::move(options);

  m_positional_set.insert(m_positional.begin(), m_positional.end());
}

inline
void
Options::parse_positional(std::initializer_list<std::string> options)
{
  parse_positional(std::vector<std::string>(options));
}

inline
ParseResult
Options::parse(int argc, const char* const* argv)
{
  OptionParser parser(*m_options, m_positional, m_allow_unrecognised);

  return parser.parse(argc, argv);
}

inline ParseResult
OptionParser::parse(int argc, const char* const* argv)
{
  int current = 1;
  bool consume_remaining = false;
  auto next_positional = m_positional.begin();

  std::vector<std::string> unmatched;

  while (current != argc)
  {
    if (strcmp(argv[current], "--") == 0)
    {
      consume_remaining = true;
      ++current;
      break;
    }
    bool matched = false;
    values::parser_tool::ArguDesc argu_desc =
        values::parser_tool::ParseArgument(argv[current], matched);

    if (!matched)
    {
      //not a flag

      // but if it starts with a `-`, then it's an error
      if (argv[current][0] == '-' && argv[current][1] != '\0') {
        if (!m_allow_unrecognised) {
          throw_or_mimic<exceptions::invalid_option_syntax>(argv[current]);
        }
      }

      //if true is returned here then it was consumed, otherwise it is
      //ignored
      if (consume_positional(argv[current], next_positional))
      {
      }
      else
      {
        unmatched.emplace_back(argv[current]);
      }
      //if we return from here then it was parsed successfully, so continue
    }
    else
    {
      //short or long option?
      if (argu_desc.grouping)
      {
        const std::string& s = argu_desc.arg_name;

        for (std::size_t i = 0; i != s.size(); ++i)
        {
          std::string name(1, s[i]);
          auto iter = m_options.find(name);

          if (iter == m_options.end())
          {
            if (m_allow_unrecognised)
            {
              unmatched.push_back(std::string("-") + s[i]);
              continue;
            }
            //error
            throw_or_mimic<exceptions::no_such_option>(name);
          }

          auto value = iter->second;

          if (i + 1 == s.size())
          {
            //it must be the last argument
            checked_parse_arg(argc, argv, current, value, name);
          }
          else if (value->value().has_implicit())
          {
            parse_option(value, name, value->value().get_implicit_value());
          }
          else if (i + 1 < s.size())
          {
            std::string arg_value = s.substr(i + 1);
            parse_option(value, name, arg_value);
            break;
          }
          else
          {
            //error
            throw_or_mimic<exceptions::option_requires_argument>(name);
          }
        }
      }
      else if (argu_desc.arg_name.length() != 0)
      {
        const std::string& name = argu_desc.arg_name;

        auto iter = m_options.find(name);

        if (iter == m_options.end())
        {
          if (m_allow_unrecognised)
          {
            // keep unrecognised options in argument list, skip to next argument
            unmatched.emplace_back(argv[current]);
            ++current;
            continue;
          }
          //error
          throw_or_mimic<exceptions::no_such_option>(name);
        }

        auto opt = iter->second;

        //equals provided for long option?
        if (argu_desc.set_value)
        {
          //parse the option given

          parse_option(opt, name, argu_desc.value);
        }
        else
        {
          //parse the next argument
          checked_parse_arg(argc, argv, current, opt, name);
        }
      }

    }

    ++current;
  }

  for (auto& opt : m_options)
  {
    auto& detail = opt.second;
    const auto& value = detail->value();

    auto& store = m_parsed[detail->hash()];

    if (value.has_default()) {
      if (!store.count() && !store.has_default()) {
        parse_default(detail);
      }
    }
    else {
      parse_no_value(detail);
    }
  }

  if (consume_remaining)
  {
    while (current < argc)
    {
      if (!consume_positional(argv[current], next_positional)) {
        break;
      }
      ++current;
    }

    //adjust argv for any that couldn't be swallowed
    while (current != argc) {
      unmatched.emplace_back(argv[current]);
      ++current;
    }
  }

  finalise_aliases();

  ParseResult parsed(std::move(m_keys), std::move(m_parsed), std::move(m_sequential), std::move(m_defaults), std::move(unmatched));
  return parsed;
}

inline
void
OptionParser::finalise_aliases()
{
  for (auto& option: m_options)
  {
    auto& detail = *option.second;
    auto hash = detail.hash();
    m_keys[detail.short_name()] = hash;
    for(const auto& long_name : detail.long_names()) {
      m_keys[long_name] = hash;
    }

    m_parsed.emplace(hash, OptionValue());
  }
}

inline
void
Options::add_option
(
  const std::string& group,
  const Option& option
)
{
    add_options(group, {option});
}

inline
void
Options::add_option
(
  const std::string& group,
  const std::string& s,
  const OptionNames& l,
  std::string desc,
  const std::shared_ptr<const Value>& value,
  std::string arg_help
)
{
  auto stringDesc = toLocalString(std::move(desc));
  auto option = std::make_shared<OptionDetails>(s, l, stringDesc, value);

  if (!s.empty())
  {
    add_one_option(s, option);
  }

  for(const auto& long_name : l) {
    add_one_option(long_name, option);
  }

  //add the help details
  auto& options = m_help[group];

  options.options.emplace_back(HelpOptionDetails{s, l, stringDesc,
      value->has_default(), value->get_default_value(),
      value->has_implicit(), value->get_implicit_value(),
      std::move(arg_help),
      value->is_container(),
      value->is_boolean()});
}

inline
void
Options::add_one_option
(
  const std::string& option,
  const std::shared_ptr<OptionDetails>& details
)
{
  auto in = m_options->emplace(option, details);

  if (!in.second)
  {
    throw_or_mimic<exceptions::option_already_exists>(option);
  }
}

inline
String
Options::help_one_group(const std::string& g) const
{
  using OptionHelp = std::vector<std::pair<String, String>>;

  auto group = m_help.find(g);
  if (group == m_help.end())
  {
    return "";
  }

  OptionHelp format;

  std::size_t longest = 0;

  String result;

  if (!g.empty())
  {
    result += toLocalString(" " + g + " options:\n");
  }

  for (const auto& o : group->second.options)
  {
    if (o.l.size() &&
        m_positional_set.find(o.l.front()) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto s = format_option(o);
    longest = (std::max)(longest, stringLength(s));
    format.push_back(std::make_pair(s, String()));
  }
  longest = (std::min)(longest, OPTION_LONGEST);

  //widest allowed description -- min 10 chars for helptext/line
  std::size_t allowed = 10;
  if (m_width > allowed + longest + OPTION_DESC_GAP)
  {
    allowed = m_width - longest - OPTION_DESC_GAP;
  }

  auto fiter = format.begin();
  for (const auto& o : group->second.options)
  {
    if (o.l.size() &&
        m_positional_set.find(o.l.front()) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto d = format_description(o, longest + OPTION_DESC_GAP, allowed, m_tab_expansion);

    result += fiter->first;
    if (stringLength(fiter->first) > longest)
    {
      result += '\n';
      result += toLocalString(std::string(longest + OPTION_DESC_GAP, ' '));
    }
    else
    {
      result += toLocalString(std::string(longest + OPTION_DESC_GAP -
        stringLength(fiter->first),
        ' '));
    }
    result += d;
    result += '\n';

    ++fiter;
  }

  return result;
}

inline
void
Options::generate_group_help
(
  String& result,
  const std::vector<std::string>& print_groups
) const
{
  for (std::size_t i = 0; i != print_groups.size(); ++i)
  {
    const String& group_help_text = help_one_group(print_groups[i]);
    if (empty(group_help_text))
    {
      continue;
    }
    result += group_help_text;
    if (i < print_groups.size() - 1)
    {
      result += '\n';
    }
  }
}

inline
void
Options::generate_all_groups_help(String& result) const
{
  std::vector<std::string> all_groups;

  std::transform(
    m_help.begin(),
    m_help.end(),
    std::back_inserter(all_groups),
    [] (const std::map<std::string, HelpGroupDetails>::value_type& group)
    {
      return group.first;
    }
  );

  generate_group_help(result, all_groups);
}

inline
std::string
Options::help(const std::vector<std::string>& help_groups, bool print_usage) const
{
  String result = m_help_string;
  if(print_usage)
  {
    result+= "\nUsage:\n  " + toLocalString(m_program);
  }

  if (!m_custom_help.empty())
  {
    result += " " + toLocalString(m_custom_help);
  }

  if (!m_positional.empty() && !m_positional_help.empty()) {
    result += " " + toLocalString(m_positional_help);
  }

  result += "\n\n";

  if (help_groups.empty())
  {
    generate_all_groups_help(result);
  }
  else
  {
    generate_group_help(result, help_groups);
  }

  return toUTF8String(result);
}

inline
std::vector<std::string>
Options::groups() const
{
  std::vector<std::string> g;

  std::transform(
    m_help.begin(),
    m_help.end(),
    std::back_inserter(g),
    [] (const std::map<std::string, HelpGroupDetails>::value_type& pair)
    {
      return pair.first;
    }
  );

  return g;
}

inline
const HelpGroupDetails&
Options::group_help(const std::string& group) const
{
  return m_help.at(group);
}

} // namespace cxxopts

#endif //CXXOPTS_HPP_INCLUDED
