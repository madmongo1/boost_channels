//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_BASE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_BASE_HPP

#include <boost/channels/concepts/std_lockable.hpp>
#include <boost/channels/config.hpp>
#include <boost/channels/error_code.hpp>

#include <tuple>

namespace boost::channels::detail {

/// @brief The base class of any shared_select_handler_state.
///
/// The shared_select_handler_state holds the mutex, completion state and
/// completion handler for any select_like operation. That is, any set of
/// asynchronous operations only one of which is allowed to complete.
/// @tparam Mutex is the type of mutex used internally to prevent concurrent
/// completion by more than one simultaneous operation.
/// @note In any set of simultaneous operations, all operations must use the
/// same mutex type. This is enforced at compile time.
template < concepts::Lockable Mutex >
struct select_state_base
{
    /// @brief The type of mutex managing this shared state
    using mutex_type = Mutex;

    /// @brief The value type of the shared select state. This matches the
    /// concept @see concepts::select_handler
    using value_type = std::tuple< error_code, int >;

    /// @brief Get a mutable reference to the mutex guarding this shared state
    /// @return a reference to the mutex
    mutex_type &
    get_mutex()
    {
        return mutex_;
    }

    /// @brief Return the completd flag for the shared state.
    ///
    /// @note This function must only be called while the mutex is locked.
    /// @return true if the operation has completed, false if it is yet to
    /// complete.
    bool
    completed() const
    {
        return completed_;
    }

    /// @brief Cause the shared state to complete.
    ///
    /// @note This function must only be called while the lock is held.
    /// @note The intention is that the derived class will:
    /// - Move the protected member handler_ into a local variable
    /// - call @see set_complete in order to inform this object that it has
    /// completed, and finally:
    /// - invoke the moved handler passig the supplied value argument.
    /// @param value The value with which to invoke the handler.
    virtual void
    complete(value_type value) = 0;

  protected:
    /// @brief Called by the derived class prior to invoking the moved handler.
    ///
    /// @see complete
    void
    set_completed()
    {
        BOOST_CHANNELS_ASSERT(!completed_);
        completed_ = true;
    }

  private:
    Mutex mutex_;
    bool  completed_ = false;
};

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_SELECT_STATE_BASE_HPP
