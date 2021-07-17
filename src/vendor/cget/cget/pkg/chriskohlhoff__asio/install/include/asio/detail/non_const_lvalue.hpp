//
// detail/non_const_lvalue.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_NON_CONST_LVALUE_HPP
#define ASIO_DETAIL_NON_CONST_LVALUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename T>
struct non_const_lvalue
{
#if defined(ASIO_HAS_MOVE)
  explicit non_const_lvalue(T& t)
    : value(static_cast<typename conditional<
        is_same<T, typename decay<T>::type>::value,
          typename decay<T>::type&, T&&>::type>(t))
  {
  }

  typename conditional<is_same<T, typename decay<T>::type>::value,
      typename decay<T>::type&, typename decay<T>::type>::type value;
#else // defined(ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  explicit non_const_lvalue(const typename decay<T>::type& t)
    : value(t)
  {
  }

  typename decay<T>::type value;
#endif // defined(ASIO_HAS_MOVE)
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_NON_CONST_LVALUE_HPP
