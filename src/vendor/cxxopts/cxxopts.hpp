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

#include <cstring>
#include <cctype>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cxxopts
{
  static constexpr struct {
    uint8_t major, minor, patch;
  } version = {2, 0, 0};
}

//when we ask cxxopts to use Unicode, help strings are processed using ICU,
//which results in the correct lengths being computed for strings when they
//are formatted for the help output
//it is necessary to make sure that <unicode/unistr.h> can be found by the
//compiler, and that icu-uc is linked in to the binary.

#ifdef CXXOPTS_USE_UNICODE
#include <unicode/unistr.h>

namespace cxxopts
{
  typedef icu::UnicodeString String;

  inline
  String
  toLocalString(std::string s)
  {
    return icu::UnicodeString::fromUTF8(std::move(s));
  }

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

  inline
  String&
  stringAppend(String&s, String a)
  {
    return s.append(std::move(a));
  }

  inline
  String&
  stringAppend(String& s, int n, UChar32 c)
  {
    for (int i = 0; i != n; ++i)
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
  typedef std::string String;

  template <typename T>
  T
  toLocalString(T&& t)
  {
    return t;
  }

  inline
  size_t
  stringLength(const String& s)
  {
    return s.length();
  }

  inline
  String&
  stringAppend(String&s, String a)
  {
    return s.append(std::move(a));
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
}

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
  }

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

    virtual bool
    is_boolean() const = 0;
  };

  class OptionException : public std::exception
  {
    public:
    OptionException(const std::string& message)
    : m_message(message)
    {
    }

    virtual const char*
    what() const noexcept
    {
      return m_message.c_str();
    }

    private:
    std::string m_message;
  };

  class OptionSpecException : public OptionException
  {
    public:

    OptionSpecException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class OptionParseException : public OptionException
  {
    public:
    OptionParseException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class option_exists_error : public OptionSpecException
  {
    public:
    option_exists_error(const std::string& option)
    : OptionSpecException(u8"Option " + LQUOTE + option + RQUOTE + u8" already exists")
    {
    }
  };

  class invalid_option_format_error : public OptionSpecException
  {
    public:
    invalid_option_format_error(const std::string& format)
    : OptionSpecException(u8"Invalid option format " + LQUOTE + format + RQUOTE)
    {
    }
  };

  class option_not_exists_exception : public OptionParseException
  {
    public:
    option_not_exists_exception(const std::string& option)
    : OptionParseException(u8"Option " + LQUOTE + option + RQUOTE + u8" does not exist")
    {
    }
  };

  class missing_argument_exception : public OptionParseException
  {
    public:
    missing_argument_exception(const std::string& option)
    : OptionParseException(
        u8"Option " + LQUOTE + option + RQUOTE + u8" is missing an argument"
      )
    {
    }
  };

  class option_requires_argument_exception : public OptionParseException
  {
    public:
    option_requires_argument_exception(const std::string& option)
    : OptionParseException(
        u8"Option " + LQUOTE + option + RQUOTE + u8" requires an argument"
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
        u8"Option " + LQUOTE + option + RQUOTE +
        u8" does not take an argument, but argument " +
        LQUOTE + arg + RQUOTE + " given"
      )
    {
    }
  };

  class option_not_present_exception : public OptionParseException
  {
    public:
    option_not_present_exception(const std::string& option)
    : OptionParseException(u8"Option " + LQUOTE + option + RQUOTE + u8" not present")
    {
    }
  };

  class argument_incorrect_type : public OptionParseException
  {
    public:
    argument_incorrect_type
    (
      const std::string& arg
    )
    : OptionParseException(
        u8"Argument " + LQUOTE + arg + RQUOTE + u8" failed to parse"
      )
    {
    }
  };

  class option_required_exception : public OptionParseException
  {
    public:
    option_required_exception(const std::string& option)
    : OptionParseException(
        u8"Option " + LQUOTE + option + RQUOTE + u8" is required but not present"
      )
    {
    }
  };

  namespace values
  {
    namespace
    {
      std::basic_regex<char> integer_pattern
        ("(-)?(0x)?([1-9a-zA-Z][0-9a-zA-Z]*)|((0x)?0)");
      std::basic_regex<char> truthy_pattern
        ("t|true|T|True");
      std::basic_regex<char> falsy_pattern
        ("(f|false|F|False)?");
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
            if (u > static_cast<U>(-std::numeric_limits<T>::min()))
            {
              throw argument_incorrect_type(text);
            }
          }
          else
          {
            if (u > static_cast<U>(std::numeric_limits<T>::max()))
            {
              throw argument_incorrect_type(text);
            }
          }
        }
      };

      template <typename T>
      struct SignedCheck<T, false>
      {
        template <typename U>
        void
        operator()(bool, U, const std::string&) {}
      };

      template <typename T, typename U>
      void
      check_signed_range(bool negative, U value, const std::string& text)
      {
        SignedCheck<T, std::numeric_limits<T>::is_signed>()(negative, value, text);
      }
    }

    template <typename R, typename T>
    R
    checked_negate(T&& t, const std::string&, std::true_type)
    {
      // if we got to here, then `t` is a positive number that fits into
      // `R`. So to avoid MSVC C4146, we first cast it to `R`.
      // See https://github.com/jarro2783/cxxopts/issues/62 for more details.
      return -static_cast<R>(t);
    }

    template <typename R, typename T>
    T
    checked_negate(T&&, const std::string& text, std::false_type)
    {
      throw argument_incorrect_type(text);
    }

    template <typename T>
    void
    integer_parser(const std::string& text, T& value)
    {
      std::smatch match;
      std::regex_match(text, match, integer_pattern);

      if (match.length() == 0)
      {
        throw argument_incorrect_type(text);
      }

      if (match.length(4) > 0)
      {
        value = 0;
        return;
      }

      using US = typename std::make_unsigned<T>::type;

      constexpr auto umax = std::numeric_limits<US>::max();
      constexpr bool is_signed = std::numeric_limits<T>::is_signed;
      const bool negative = match.length(1) > 0;
      const auto base = match.length(2) > 0 ? 16 : 10;

      auto value_match = match[3];

      US result = 0;

      for (auto iter = value_match.first; iter != value_match.second; ++iter)
      {
        int digit = 0;

        if (*iter >= '0' && *iter <= '9')
        {
          digit = *iter - '0';
        }
        else if (base == 16 && *iter >= 'a' && *iter <= 'f')
        {
          digit = *iter - 'a' + 10;
        }
        else if (base == 16 && *iter >= 'A' && *iter <= 'F')
        {
          digit = *iter - 'A' + 10;
        }
        else
        {
          throw argument_incorrect_type(text);
        }

        if (umax - digit < result * base)
        {
          throw argument_incorrect_type(text);
        }

        result = result * base + digit;
      }

      detail::check_signed_range<T>(negative, result, text);

      if (negative)
      {
        value = checked_negate<T>(result,
          text,
          std::integral_constant<bool, is_signed>());
      }
      else
      {
        value = result;
      }
    }

    template <typename T>
    void stringstream_parser(const std::string& text, T& value)
    {
      std::stringstream in(text);
      in >> value;
      if (!in) {
        throw argument_incorrect_type(text);
      }
    }

    inline
    void
    parse_value(const std::string& text, uint8_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, int8_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, uint16_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, int16_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, uint32_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, int32_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, uint64_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, int64_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const std::string& text, bool& value)
    {
      std::smatch result;
      std::regex_match(text, result, truthy_pattern);

      if (!result.empty())
      {
        value = true;
        return;
      }

      std::regex_match(text, result, falsy_pattern);
      if (!result.empty())
      {
        value = false;
        return;
      }

      throw argument_incorrect_type(text);
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
    template <typename T>
    void
    parse_value(const std::string& text, T& value) {
      stringstream_parser(text, value);
    }

    template <typename T>
    void
    parse_value(const std::string& text, std::vector<T>& value)
    {
      T v;
      parse_value(text, v);
      value.push_back(v);
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

      abstract_value(T* t)
      : m_store(t)
      {
      }

      virtual ~abstract_value() = default;

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
      parse(const std::string& text) const
      {
        parse_value(text, *m_store);
      }

      bool
      is_container() const
      {
        return type_is_container<T>::value;
      }

      void
      parse() const
      {
        parse_value(m_default_value, *m_store);
      }

      bool
      has_default() const
      {
        return m_default;
      }

      bool
      has_implicit() const
      {
        return m_implicit;
      }

      std::shared_ptr<Value>
      default_value(const std::string& value)
      {
        m_default = true;
        m_default_value = value;
        return shared_from_this();
      }

      std::shared_ptr<Value>
      implicit_value(const std::string& value)
      {
        m_implicit = true;
        m_implicit_value = value;
        return shared_from_this();
      }

      std::string
      get_default_value() const
      {
        return m_default_value;
      }

      std::string
      get_implicit_value() const
      {
        return m_implicit_value;
      }

      bool
      is_boolean() const
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
        else
        {
          return *m_store;
        }
      }

      protected:
      std::shared_ptr<T> m_result;
      T* m_store;

      bool m_default = false;
      bool m_implicit = false;

      std::string m_default_value;
      std::string m_implicit_value;
    };

    template <typename T>
    class standard_value : public abstract_value<T>
    {
      public:
      using abstract_value<T>::abstract_value;

      std::shared_ptr<Value>
      clone() const
      {
        return std::make_shared<standard_value<T>>(*this);
      }
    };

    template <>
    class standard_value<bool> : public abstract_value<bool>
    {
      public:
      ~standard_value() = default;

      standard_value()
      {
        set_implicit();
      }

      standard_value(bool* b)
      : abstract_value(b)
      {
        set_implicit();
      }

      std::shared_ptr<Value>
      clone() const
      {
        return std::make_shared<standard_value<bool>>(*this);
      }

      private:

      void
      set_implicit()
      {
        m_implicit = true;
        m_implicit_value = "true";
      }
    };
  }

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
      const std::string& short_name,
      const std::string& long_name,
      const String& desc,
      std::shared_ptr<const Value> val
    )
    : m_short(short_name)
    , m_long(long_name)
    , m_desc(desc)
    , m_value(val)
    , m_count(0)
    {
    }

    OptionDetails(const OptionDetails& rhs)
    : m_desc(rhs.m_desc)
    , m_count(rhs.m_count)
    {
      m_value = rhs.m_value->clone();
    }

    OptionDetails(OptionDetails&& rhs) = default;

    const String&
    description() const
    {
      return m_desc;
    }

    const Value& value() const {
        return *m_value;
    }

    std::shared_ptr<Value>
    make_storage() const
    {
      return m_value->clone();
    }

    const std::string&
    short_name() const
    {
      return m_short;
    }

    const std::string&
    long_name() const
    {
      return m_long;
    }

    private:
    std::string m_short;
    std::string m_long;
    String m_desc;
    std::shared_ptr<const Value> m_value;
    int m_count;
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
    std::string name;
    std::string description;
    std::vector<HelpOptionDetails> options;
  };

  class OptionValue
  {
    public:
    void
    parse
    (
      std::shared_ptr<const OptionDetails> details,
      const std::string& text
    )
    {
      ensure_value(details);
      ++m_count;
      m_value->parse(text);
    }

    void
    parse_default(std::shared_ptr<const OptionDetails> details)
    {
      ensure_value(details);
      m_value->parse();
      m_count++;
    }

    size_t
    count() const
    {
      return m_count;
    }

    template <typename T>
    const T&
    as() const
    {
#ifdef CXXOPTS_NO_RTTI
      return static_cast<const values::standard_value<T>&>(*m_value).get();
#else
      return dynamic_cast<const values::standard_value<T>&>(*m_value).get();
#endif
    }

    private:
    void
    ensure_value(std::shared_ptr<const OptionDetails> details)
    {
      if (m_value == nullptr)
      {
        m_value = details->make_storage();
      }
    }

    std::shared_ptr<Value> m_value;
    size_t m_count = 0;
  };

  class KeyValue
  {
    public:
    KeyValue(std::string key, std::string value)
    : m_key(std::move(key))
    , m_value(std::move(value))
    {
    }

    const
    std::string&
    key() const
    {
      return m_key;
    }

    const std::string
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

  class ParseResult
  {
    public:

    ParseResult(
      const std::unordered_map<std::string, std::shared_ptr<OptionDetails>>&,
      std::vector<std::string>,
      int&, char**&);

    int
    count(const std::string& o) const
    {
      auto iter = m_options.find(o);
      if (iter == m_options.end())
      {
        return 0;
      }

      auto riter = m_results.find(iter->second);

      return riter->second.count();
    }

    const OptionValue&
    operator[](const std::string& option) const
    {
      auto iter = m_options.find(option);

      if (iter == m_options.end())
      {
        throw option_not_present_exception(option);
      }

      auto riter = m_results.find(iter->second);

      return riter->second;
    }

    const std::vector<KeyValue>&
    arguments() const
    {
      return m_sequential;
    }

    private:

    OptionValue&
    get_option(std::shared_ptr<OptionDetails>);

    void
    parse(int& argc, char**& argv);

    void
    add_to_option(const std::string& option, const std::string& arg);

    bool
    consume_positional(std::string a);

    void
    parse_option
    (
      std::shared_ptr<OptionDetails> value,
      const std::string& name,
      const std::string& arg = ""
    );

    void
    parse_default(std::shared_ptr<OptionDetails> details);

    void
    checked_parse_arg
    (
      int argc,
      char* argv[],
      int& current,
      std::shared_ptr<OptionDetails> value,
      const std::string& name
    );

    const std::unordered_map<std::string, std::shared_ptr<OptionDetails>>
      &m_options;
    std::vector<std::string> m_positional;
    std::vector<std::string>::iterator m_next_positional;
    std::unordered_set<std::string> m_positional_set;
    std::unordered_map<std::shared_ptr<OptionDetails>, OptionValue> m_results;

    std::vector<KeyValue> m_sequential;
  };

  class Options
  {
    public:

    Options(std::string program, std::string help_string = "")
    : m_program(std::move(program))
    , m_help_string(toLocalString(std::move(help_string)))
    , m_positional_help("positional parameters")
    , m_show_positional(false)
    , m_next_positional(m_positional.end())
    {
    }

    Options&
    positional_help(std::string help_text)
    {
      m_positional_help = std::move(help_text);
      return *this;
    }

    Options&
    show_positional_help()
    {
      m_show_positional = true;
      return *this;
    }

    ParseResult
    parse(int& argc, char**& argv);

    OptionAdder
    add_options(std::string group = "");

    void
    add_option
    (
      const std::string& group,
      const std::string& s,
      const std::string& l,
      std::string desc,
      std::shared_ptr<const Value> value,
      std::string arg_help
    );

    //parse positional arguments into the given option
    void
    parse_positional(std::string option);

    void
    parse_positional(std::vector<std::string> options);

    std::string
    help(const std::vector<std::string>& groups = {""}) const;

    const std::vector<std::string>
    groups() const;

    const HelpGroupDetails&
    group_help(const std::string& group) const;

    private:

    void
    add_one_option
    (
      const std::string& option,
      std::shared_ptr<OptionDetails> details
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

    std::string m_program;
    String m_help_string;
    std::string m_positional_help;
    bool m_show_positional;

    std::unordered_map<std::string, std::shared_ptr<OptionDetails>> m_options;
    std::vector<std::string> m_positional;
    std::vector<std::string>::iterator m_next_positional;
    std::unordered_set<std::string> m_positional_set;

    //mapping from groups to help options
    std::map<std::string, HelpGroupDetails> m_help;
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
      std::shared_ptr<const Value> value
        = ::cxxopts::value<bool>(),
      std::string arg_help = ""
    );

    private:
    Options& m_options;
    std::string m_group;
  };

  namespace
  {
    constexpr int OPTION_LONGEST = 30;
    constexpr int OPTION_DESC_GAP = 2;

    std::basic_regex<char> option_matcher
      ("--([[:alnum:]][-_[:alnum:]]+)(=(.*))?|-([[:alnum:]]+)");

    std::basic_regex<char> option_specifier
      ("(([[:alnum:]]),)?[ ]*([[:alnum:]][-_[:alnum:]]*)?");

    String
    format_option
    (
      const HelpOptionDetails& o
    )
    {
      auto& s = o.s;
      auto& l = o.l;

      String result = "  ";

      if (s.size() > 0)
      {
        result += "-" + toLocalString(s) + ",";
      }
      else
      {
        result += "   ";
      }

      if (l.size() > 0)
      {
        result += " --" + toLocalString(l);
      }

      auto arg = o.arg_help.size() > 0 ? toLocalString(o.arg_help) : "arg";

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
      size_t width
    )
    {
      auto desc = o.desc;

      if (o.has_default)
      {
        desc += toLocalString(" (default: " + o.default_value + ")");
      }

      String result;

      auto current = std::begin(desc);
      auto startLine = current;
      auto lastSpace = current;

      auto size = size_t{};

      while (current != std::end(desc))
      {
        if (*current == ' ')
        {
          lastSpace = current;
        }

        if (size > width)
        {
          if (lastSpace == startLine)
          {
            stringAppend(result, startLine, current + 1);
            stringAppend(result, "\n");
            stringAppend(result, start, ' ');
            startLine = current + 1;
            lastSpace = startLine;
          }
          else
          {
            stringAppend(result, startLine, lastSpace);
            stringAppend(result, "\n");
            stringAppend(result, start, ' ');
            startLine = lastSpace + 1;
          }
          size = 0;
        }
        else
        {
          ++size;
        }

        ++current;
      }

      //append whatever is left
      stringAppend(result, startLine, current);

      return result;
    }
  }

inline
ParseResult::ParseResult
(
  const std::unordered_map<std::string, std::shared_ptr<OptionDetails>>& options,
  std::vector<std::string> positional,
  int& argc, char**& argv
)
: m_options(options)
, m_positional(std::move(positional))
, m_next_positional(m_positional.begin())
{
  parse(argc, argv);
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
  std::shared_ptr<const Value> value,
  std::string arg_help
)
{
  std::match_results<const char*> result;
  std::regex_match(opts.c_str(), result, option_specifier);

  if (result.empty())
  {
    throw invalid_option_format_error(opts);
  }

  const auto& short_match = result[2];
  const auto& long_match = result[3];

  if (!short_match.length() && !long_match.length())
  {
    throw invalid_option_format_error(opts);
  } else if (long_match.length() == 1 && short_match.length())
  {
    throw invalid_option_format_error(opts);
  }

  auto option_names = []
  (
    const std::sub_match<const char*>& short_,
    const std::sub_match<const char*>& long_
  )
  {
    if (long_.length() == 1)
    {
      return std::make_tuple(long_.str(), short_.str());
    }
    else
    {
      return std::make_tuple(short_.str(), long_.str());
    }
  }(short_match, long_match);

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
ParseResult::parse_default(std::shared_ptr<OptionDetails> details)
{
  m_results[details].parse_default(details);
}

inline
void
ParseResult::parse_option
(
  std::shared_ptr<OptionDetails> value,
  const std::string& /*name*/,
  const std::string& arg
)
{
  auto& result = m_results[value];
  result.parse(value, arg);

  m_sequential.emplace_back(value->long_name(), arg);
}

inline
void
ParseResult::checked_parse_arg
(
  int argc,
  char* argv[],
  int& current,
  std::shared_ptr<OptionDetails> value,
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
      throw missing_argument_exception(name);
    }
  }
  else
  {
    if (argv[current + 1][0] == '-' && value->value().has_implicit())
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
ParseResult::add_to_option(const std::string& option, const std::string& arg)
{
  auto iter = m_options.find(option);

  if (iter == m_options.end())
  {
    throw option_not_exists_exception(option);
  }

  parse_option(iter->second, option, arg);
}

inline
bool
ParseResult::consume_positional(std::string a)
{
  while (m_next_positional != m_positional.end())
  {
    auto iter = m_options.find(*m_next_positional);
    if (iter != m_options.end())
    {
      auto& result = m_results[iter->second];
      if (!iter->second->value().is_container()) 
      {
        if (result.count() == 0)
        {
          add_to_option(*m_next_positional, a);
          ++m_next_positional;
          return true;
        }
        else
        {
          ++m_next_positional;
          continue;
        }
      }
      else
      {
        add_to_option(*m_next_positional, a);
        return true;
      }
    }
    ++m_next_positional;
  }

  return false;
}

inline
void
Options::parse_positional(std::string option)
{
  parse_positional(std::vector<std::string>{option});
}

inline
void
Options::parse_positional(std::vector<std::string> options)
{
  m_positional = std::move(options);
  m_next_positional = m_positional.begin();

  m_positional_set.insert(m_positional.begin(), m_positional.end());
}

inline
ParseResult
Options::parse(int& argc, char**& argv)
{
  ParseResult result(m_options, m_positional, argc, argv);
  return result;
}

inline
void
ParseResult::parse(int& argc, char**& argv)
{
  int current = 1;

  int nextKeep = 1;

  bool consume_remaining = false;

  while (current != argc)
  {
    if (strcmp(argv[current], "--") == 0)
    {
      consume_remaining = true;
      ++current;
      break;
    }

    std::match_results<const char*> result;
    std::regex_match(argv[current], result, option_matcher);

    if (result.empty())
    {
      //not a flag

      //if true is returned here then it was consumed, otherwise it is
      //ignored
      if (consume_positional(argv[current]))
      {
      }
      else
      {
        argv[nextKeep] = argv[current];
        ++nextKeep;
      }
      //if we return from here then it was parsed successfully, so continue
    }
    else
    {
      //short or long option?
      if (result[4].length() != 0)
      {
        const std::string& s = result[4];

        for (std::size_t i = 0; i != s.size(); ++i)
        {
          std::string name(1, s[i]);
          auto iter = m_options.find(name);

          if (iter == m_options.end())
          {
            throw option_not_exists_exception(name);
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
          else
          {
            //error
            throw option_requires_argument_exception(name);
          }
        }
      }
      else if (result[1].length() != 0)
      {
        const std::string& name = result[1];

        auto iter = m_options.find(name);

        if (iter == m_options.end())
        {
          throw option_not_exists_exception(name);
        }

        auto opt = iter->second;

        //equals provided for long option?
        if (result[3].length() != 0)
        {
          //parse the option given

          parse_option(opt, name, result[3]);
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
    auto& value = detail->value();

    auto& store = m_results[detail];

    if(!store.count() && value.has_default()){
      parse_default(detail);
    }
  }

  if (consume_remaining)
  {
    while (current < argc)
    {
      if (!consume_positional(argv[current])) {
        break;
      }
      ++current;
    }

    //adjust argv for any that couldn't be swallowed
    while (current != argc) {
      argv[nextKeep] = argv[current];
      ++nextKeep;
      ++current;
    }
  }

  argc = nextKeep;

}

inline
void
Options::add_option
(
  const std::string& group,
  const std::string& s,
  const std::string& l,
  std::string desc,
  std::shared_ptr<const Value> value,
  std::string arg_help
)
{
  auto stringDesc = toLocalString(std::move(desc));
  auto option = std::make_shared<OptionDetails>(s, l, stringDesc, value);

  if (s.size() > 0)
  {
    add_one_option(s, option);
  }

  if (l.size() > 0)
  {
    add_one_option(l, option);
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
  std::shared_ptr<OptionDetails> details
)
{
  auto in = m_options.emplace(option, details);

  if (!in.second)
  {
    throw option_exists_error(option);
  }
}

inline
String
Options::help_one_group(const std::string& g) const
{
  typedef std::vector<std::pair<String, String>> OptionHelp;

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
    if (o.is_container &&
        m_positional_set.find(o.l) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto s = format_option(o);
    longest = std::max(longest, stringLength(s));
    format.push_back(std::make_pair(s, String()));
  }

  longest = std::min(longest, static_cast<size_t>(OPTION_LONGEST));

  //widest allowed description
  auto allowed = size_t{76} - longest - OPTION_DESC_GAP;

  auto fiter = format.begin();
  for (const auto& o : group->second.options)
  {
    if (o.is_container &&
        m_positional_set.find(o.l) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto d = format_description(o, longest + OPTION_DESC_GAP, allowed);

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
  all_groups.reserve(m_help.size());

  for (auto& group : m_help)
  {
    all_groups.push_back(group.first);
  }

  generate_group_help(result, all_groups);
}

inline
std::string
Options::help(const std::vector<std::string>& help_groups) const
{
  String result = m_help_string + "\nUsage:\n  " +
    toLocalString(m_program) + " [OPTION...]";

  if (m_positional.size() > 0) {
    result += " " + toLocalString(m_positional_help);
  }

  result += "\n\n";

  if (help_groups.size() == 0)
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
const std::vector<std::string>
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

}

#endif //CXXOPTS_HPP_INCLUDED
