//
// execution.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_EXECUTION_HPP
#define ASIO_EXECUTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/execution/allocator.hpp"
#include "asio/execution/any_executor.hpp"
#include "asio/execution/bad_executor.hpp"
#include "asio/execution/blocking.hpp"
#include "asio/execution/blocking_adaptation.hpp"
#include "asio/execution/bulk_execute.hpp"
#include "asio/execution/bulk_guarantee.hpp"
#include "asio/execution/connect.hpp"
#include "asio/execution/context.hpp"
#include "asio/execution/context_as.hpp"
#include "asio/execution/execute.hpp"
#include "asio/execution/executor.hpp"
#include "asio/execution/invocable_archetype.hpp"
#include "asio/execution/mapping.hpp"
#include "asio/execution/occupancy.hpp"
#include "asio/execution/operation_state.hpp"
#include "asio/execution/outstanding_work.hpp"
#include "asio/execution/prefer_only.hpp"
#include "asio/execution/receiver.hpp"
#include "asio/execution/receiver_invocation_error.hpp"
#include "asio/execution/relationship.hpp"
#include "asio/execution/schedule.hpp"
#include "asio/execution/scheduler.hpp"
#include "asio/execution/sender.hpp"
#include "asio/execution/set_done.hpp"
#include "asio/execution/set_error.hpp"
#include "asio/execution/set_value.hpp"
#include "asio/execution/start.hpp"
#include "asio/execution/submit.hpp"

#endif // ASIO_EXECUTION_HPP
