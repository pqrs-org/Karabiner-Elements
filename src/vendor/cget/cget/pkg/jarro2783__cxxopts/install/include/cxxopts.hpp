/*

Copyright (c) 2014, 2015, 2016, 2017 Jarryd Beck

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

#ifndef CXXOPTS_HPP_INCLUDED
#define CXXOPTS_HPP_INCLUDED

#include <cctype>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <algorithm>

#if defined(__GNUC__) && !defined(__clang__)
#  if (__GNUC__ * 10 + __GNUC_MINOR__) < 49
#    define CXXOPTS_NO_REGEX true
#  endif
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
#define CXXOPTS__VERSION_MINOR 0
#define CXXOPTS__VERSION_PATCH 0

#if (__GNUC__ < 10 || (__GNUC__ == 10 && __GNUC_MINOR__ < 1)) && __GNUC__ >= 6
  #define CXXOPTS_NULL_DEREF_IGNORE
#endif

namespace cxxopts
{
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

namespace cxxopts
{
  using String = icu::UnicodeString;

  inline
  String
  toLocalString(std::string s)
  {
    return icu::UnicodeString::fromUTF8(std::move(s));
  }

#if defined(__GNUC__)
// GNU GCC with -Weffc++ will issue a warning regarding the upcoming class, we want to silence it:
// warning: base class 'class std::enable_shared_from_this<cxxopts::Value>' has accessible non-virtual destructor
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Weffc++"
// This will be ignored under other compilers like LLVM clang.
#endif
  class UnicodeStringIterator : public
    std::iterator<std::forward_iterator_tag, int32_t>
  {
    public:

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
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  inline
  String&
  stringAppend(String&s, String a)
  {
    return s.append(std::move(a));
  }

  inline
  String&
  stringAppend(String& s, size_t n, UChar32 c)
  {
    for (size_t i = 0; i != n; ++i)
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
  size_t
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
}

namespace std
{
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
}

//ifdef CXXOPTS_USE_UNICODE
#else

namespace cxxopts
{
  using String = std::string;

  template <typename T>
  T
  toLocalString(T&& t)
  {
    return std::forward<T>(t);
  }

  inline
  size_t
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
  stringAppend(String& s, size_t n, char c)
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

namespace cxxopts
{
  namespace
  {
#ifdef _WIN32
    const std::string LQUOTE("\'");
    const std::string RQUOTE("\'");
#else
    const std::string LQUOTE("‘");
    const std::string RQUOTE("’");
#endif
  } // namespace

#if defined(__GNUC__)
// GNU GCC with -Weffc++ will issue a warning regarding the upcoming class, we want to silence it:
// warning: base class 'class std::enable_shared_from_this<cxxopts::Value>' has accessible non-virtual destructor
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Weffc++"
// This will be ignored under other compilers like LLVM clang.
#endif
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
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  class OptionException : public std::exception
  {
    public:
    explicit OptionException(std::string  message)
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

  class OptionSpecException : public OptionException
  {
    public:

    explicit OptionSpecException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class OptionParseException : public OptionException
  {
    public:
    explicit OptionParseException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class option_exists_error : public OptionSpecException
  {
    public:
    explicit option_exists_error(const std::string& option)
    : OptionSpecException("Option " + LQUOTE + option + RQUOTE + " already exists")
    {
    }
  };

  class invalid_option_format_error : public OptionSpecException
  {
    public:
    explicit invalid_option_format_error(const std::string& format)
    : OptionSpecException("Invalid option format " + LQUOTE + format + RQUOTE)
    {
    }
  };

  class option_syntax_exception : public OptionParseException {
    public:
    explicit option_syntax_exception(const std::string& text)
    : OptionParseException("Argument " + LQUOTE + text + RQUOTE +
        " starts with a - but has incorrect syntax")
    {
    }
  };

  class option_not_exists_exception : public OptionParseException
  {
    public:
    explicit option_not_exists_exception(const std::string& option)
    : OptionParseException("Option " + LQUOTE + option + RQUOTE + " does not exist")
    {
    }
  };

  class missing_argument_exception : public OptionParseException
  {
    public:
    explicit missing_argument_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " is missing an argument"
      )
    {
    }
  };

  class option_requires_argument_exception : public OptionParseException
  {
    public:
    explicit option_requires_argument_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " requires an argument"
      )
    {
    }
  };

  class option_not_has_argument_exception : public OptionParseException
  {
    public:
    option_not_has_argument_exception
    (
      const std::string& option,
      const std::string& arg
    )
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE +
        " does not take an argument, but argument " +
        LQUOTE + arg + RQUOTE + " given"
      )
    {
    }
  };

  class option_not_present_exception : public OptionParseException
  {
    public:
    explicit option_not_present_exception(const std::string& option)
    : OptionParseException("Option " + LQUOTE + option + RQUOTE + " not present")
    {
    }
  };

  class option_has_no_value_exception : public OptionException
  {
    public:
    explicit option_has_no_value_exception(const std::string& option)
    : OptionException(
        !option.empty() ?
        ("Option " + LQUOTE + option + RQUOTE + " has no value") :
        "Option has no value")
    {
    }
  };

  class argument_incorrect_type : public OptionParseException
  {
    public:
    explicit argument_incorrect_type
    (
      const std::string& arg
    )
    : OptionParseException(
        "Argument " + LQUOTE + arg + RQUOTE + " failed to parse"
      )
    {
    }
  };

  class option_required_exception : public OptionParseException
  {
    public:
    explicit option_required_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " is required but not present"
      )
    {
    }
  };

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

  namespace values
  {
    namespace parser_tool
    {
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
          throw_or_mimic<argument_incorrect_type>(text);
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
          throw_or_mimic<argument_incorrect_type>(text);
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

      inline std::pair<std::string, std::string> SplitSwitchDef(const std::string &text)
      {
        std::string short_sw, long_sw;
        const char *pdata = text.c_str();
        if (isalnum(*pdata) && *(pdata + 1) == ',') {
          short_sw = std::string(1, *pdata);
          pdata += 2;
        }
        while (*pdata == ' ') { pdata += 1; }
        if (isalnum(*pdata)) {
          const char *store = pdata;
          pdata += 1;
          while (isalnum(*pdata) || *pdata == '-' || *pdata == '_') {
            pdata += 1;
          }
          if (*pdata == '\0') {
            long_sw = std::string(store, pdata - store);
          } else {
            throw_or_mimic<invalid_option_format_error>(text);
          }
        }
        return std::pair<std::string, std::string>(short_sw, long_sw);
      }

      inline ArguDesc ParseArgument(const char *arg, bool &matched)
      {
        ArguDesc argu_desc;
        const char *pdata = arg;
        matched = false;
        if (strncmp(pdata, "--", 2) == 0)
        {
          pdata += 2;
          if (isalnum(*pdata))
          {
            argu_desc.arg_name.push_back(*pdata);
            pdata += 1;
            while (isalnum(*pdata) || *pdata == '-' || *pdata == '_')
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
          while (isalnum(*pdata))
          {
            argu_desc.arg_name.push_back(*pdata);
            pdata += 1;
          }
          matched = !argu_desc.arg_name.empty() && *pdata == '\0';
        }
        return argu_desc;
      }

#else  // CXXOPTS_NO_REGEX

      namespace
      {

        std::basic_regex<char> integer_pattern
          ("(-)?(0x)?([0-9a-zA-Z]+)|((0x)?0)");
        std::basic_regex<char> truthy_pattern
          ("(t|T)(rue)?|1");
        std::basic_regex<char> falsy_pattern
          ("(f|F)(alse)?|0");

        std::basic_regex<char> option_matcher
          ("--([[:alnum:]][-_[:alnum:]]+)(=(.*))?|-([[:alnum:]]+)");
        std::basic_regex<char> option_specifier
          ("(([[:alnum:]]),)?[ ]*([[:alnum:]][-_[:alnum:]]*)?");

      } // namespace

      inline IntegerDesc SplitInteger(const std::string &text)
      {
        std::smatch match;
        std::regex_match(text, match, integer_pattern);

        if (match.length() == 0)
        {
          throw_or_mimic<argument_incorrect_type>(text);
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

      inline std::pair<std::string, std::string> SplitSwitchDef(const std::string &text)
      {
        std::match_results<const char*> result;
        std::regex_match(text.c_str(), result, option_specifier);
        if (result.empty())
        {
          throw_or_mimic<invalid_option_format_error>(text);
        }

        const std::string& short_sw = result[2];
        const std::string& long_sw = result[3];

        return std::pair<std::string, std::string>(short_sw, long_sw);
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
  }

    namespace detail
    {
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
              throw_or_mimic<argument_incorrect_type>(text);
            }
          }
          else
          {
            if (u > static_cast<U>((std::numeric_limits<T>::max)()))
            {
              throw_or_mimic<argument_incorrect_type>(text);
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
      throw_or_mimic<argument_incorrect_type>(text);
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
          throw_or_mimic<argument_incorrect_type>(text);
        }

        const US next = static_cast<US>(result * base + digit);
        if (result > next)
        {
          throw_or_mimic<argument_incorrect_type>(text);
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
        throw_or_mimic<argument_incorrect_type>(text);
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

      throw_or_mimic<argument_incorrect_type>(text);
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
        throw_or_mimic<argument_incorrect_type>(text);
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
        set_default_and_implicit();
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

  class OptionDetails
  {
    public:
    OptionDetails
    (
      std::string short_,
      std::string long_,
      String desc,
      std::shared_ptr<const Value> val
    )
    : m_short(std::move(short_))
    , m_long(std::move(long_))
    , m_desc(std::move(desc))
    , m_value(std::move(val))
    , m_count(0)
    {
      m_hash = std::hash<std::string>{}(m_long + m_short);
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
    long_name() const
    {
      return m_long;
    }

    size_t
    hash() const
    {
      return m_hash;
    }

    private:
    std::string m_short{};
    std::string m_long{};
    String m_desc{};
    std::shared_ptr<const Value> m_value{};
    int m_count;

    size_t m_hash{};
  };

  struct HelpOptionDetails
  {
    std::string s;
    std::string l;
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
      m_long_name = &details->long_name();
    }

    void
    parse_default(const std::shared_ptr<const OptionDetails>& details)
    {
      ensure_value(details);
      m_default = true;
      m_long_name = &details->long_name();
      m_value->parse();
    }

    void
    parse_no_value(const std::shared_ptr<const OptionDetails>& details)
    {
      m_long_name = &details->long_name();
    }

#if defined(CXXOPTS_NULL_DEREF_IGNORE)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

    CXXOPTS_NODISCARD
    size_t
    count() const noexcept
    {
      return m_count;
    }

#if defined(CXXOPTS_NULL_DEREF_IGNORE)
#pragma GCC diagnostic pop
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
          throw_or_mimic<option_has_no_value_exception>(
              m_long_name == nullptr ? "" : *m_long_name);
      }

#ifdef CXXOPTS_NO_RTTI
      return static_cast<const values::standard_value<T>&>(*m_value).get();
#else
      return dynamic_cast<const values::standard_value<T>&>(*m_value).get();
#endif
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


    const std::string* m_long_name = nullptr;
    // Holding this pointer is safe, since OptionValue's only exist in key-value pairs,
    // where the key has the string we point to.
    std::shared_ptr<Value> m_value{};
    size_t m_count = 0;
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

  using ParsedHashMap = std::unordered_map<size_t, OptionValue>;
  using NameHashMap = std::unordered_map<std::string, size_t>;

  class ParseResult
  {
    public:

    ParseResult() = default;
    ParseResult(const ParseResult&) = default;

    ParseResult(NameHashMap&& keys, ParsedHashMap&& values, std::vector<KeyValue> sequential, std::vector<std::string>&& unmatched_args)
    : m_keys(std::move(keys))
    , m_values(std::move(values))
    , m_sequential(std::move(sequential))
    , m_unmatched(std::move(unmatched_args))
    {
    }

    ParseResult& operator=(ParseResult&&) = default;
    ParseResult& operator=(const ParseResult&) = default;

    size_t
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
        throw_or_mimic<option_not_present_exception>(option);
      }

      auto viter = m_values.find(iter->second);

      if (viter == m_values.end())
      {
        throw_or_mimic<option_not_present_exception>(option);
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

    private:
    NameHashMap m_keys{};
    ParsedHashMap m_values{};
    std::vector<KeyValue> m_sequential{};
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
    bool m_allow_unrecognised;

    ParsedHashMap m_parsed{};
    NameHashMap m_keys{};
  };

  class Options
  {
    public:

    explicit Options(std::string program, std::string help_string = "")
    : m_program(std::move(program))
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
    set_width(size_t width)
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
      const std::string& l,
      std::string desc,
      const std::shared_ptr<const Value>& value,
      std::string arg_help
    );

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
    help(const std::vector<std::string>& groups = {}) const;

    std::vector<std::string>
    groups() const;

    const HelpGroupDetails&
    group_help(const std::string& group) const;

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
    size_t m_width;
    bool m_tab_expansion;

    std::shared_ptr<OptionMap> m_options;
    std::vector<std::string> m_positional{};
    std::unordered_set<std::string> m_positional_set{};

    //mapping from groups to help options
    std::map<std::string, HelpGroupDetails> m_help{};

    std::list<OptionDetails> m_option_list{};
    std::unordered_map<std::string, decltype(m_option_list)::iterator> m_option_map{};
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

  namespace
  {
    constexpr size_t OPTION_LONGEST = 30;
    constexpr size_t OPTION_DESC_GAP = 2;

    String
    format_option
    (
      const HelpOptionDetails& o
    )
    {
      const auto& s = o.s;
      const auto& l = o.l;

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
      size_t start,
      size_t allowed,
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
        auto size = size_t{ 0 };
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

      auto size = size_t{};

      bool appendNewLine;
      bool onlyWhiteSpace = true;

      while (current != std::end(desc))
      {
        appendNewLine = false;

        if (std::isblank(*previous))
        {
          lastSpace = current;
        }

        if (!std::isblank(*current))
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
  std::string short_sw, long_sw;
  std::tie(short_sw, long_sw) = values::parser_tool::SplitSwitchDef(opts);

  if (!short_sw.length() && !long_sw.length())
  {
    throw_or_mimic<invalid_option_format_error>(opts);
  }
  else if (long_sw.length() == 1 && short_sw.length())
  {
    throw_or_mimic<invalid_option_format_error>(opts);
  }

  auto option_names = []
  (
    const std::string &short_,
    const std::string &long_
  )
  {
    if (long_.length() == 1)
    {
      return std::make_tuple(long_, short_);
    }
    return std::make_tuple(short_, long_);
  }(short_sw, long_sw);

  m_options.add_option
  (
    m_group,
    std::get<0>(option_names),
    std::get<1>(option_names),
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

  m_sequential.emplace_back(value->long_name(), arg);
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
      throw_or_mimic<missing_argument_exception>(name);
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
    throw_or_mimic<option_not_exists_exception>(*next);
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
          throw_or_mimic<option_syntax_exception>(argv[current]);
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
            throw_or_mimic<option_not_exists_exception>(name);
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
            throw_or_mimic<option_requires_argument_exception>(name);
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
          throw_or_mimic<option_not_exists_exception>(name);
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

  ParseResult parsed(std::move(m_keys), std::move(m_parsed), std::move(m_sequential), std::move(unmatched));
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
    m_keys[detail.long_name()] = hash;

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
  const std::string& l,
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

  if (!l.empty())
  {
    add_one_option(l, option);
  }

  m_option_list.push_front(*option.get());
  auto iter = m_option_list.begin();
  m_option_map[s] = iter;
  m_option_map[l] = iter;

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
    throw_or_mimic<option_exists_error>(option);
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

  size_t longest = 0;

  String result;

  if (!g.empty())
  {
    result += toLocalString(" " + g + " options:\n");
  }

  for (const auto& o : group->second.options)
  {
    if (m_positional_set.find(o.l) != m_positional_set.end() &&
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
  size_t allowed = 10;
  if (m_width > allowed + longest + OPTION_DESC_GAP)
  {
    allowed = m_width - longest - OPTION_DESC_GAP;
  }

  auto fiter = format.begin();
  for (const auto& o : group->second.options)
  {
    if (m_positional_set.find(o.l) != m_positional_set.end() &&
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
  for (size_t i = 0; i != print_groups.size(); ++i)
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
Options::help(const std::vector<std::string>& help_groups) const
{
  String result = m_help_string + "\nUsage:\n  " +
    toLocalString(m_program) + " " + toLocalString(m_custom_help);

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
