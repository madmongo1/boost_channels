//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_ERROR_CODE_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_ERROR_CODE_HPP

#include <string_view>

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>

namespace boost::channels {

using ::boost::system::error_category;
using ::boost::system::error_code;

struct errors
{
    enum channel_errors
    {
        channel_null   = 1,   //! The channel does not have an implementation
        channel_closed = 2,   //! The channel has been closed
    };

    struct channel_category final : error_category
    {
        const char *
        name() const noexcept override
        {
            return "boost::channel::channel_errors";
        }

        virtual std::string
        message(int ev) const override
        {
            auto const source = get_message(ev);
            return std::string(source.begin(), source.end());
        }

        char const *
        message(int ev, char *buffer, std::size_t len) const noexcept override
        {
            auto const source = get_message(ev);

            // Short circuit when buffer of size zero is offered.
            // Note that this is safe because the string_view's data actually
            // points to a c-style string.
            if (len == 0)
                return source.data();

            auto to_copy = (std::min)(len - 1, source.size());
            std::copy(source.data(), source.data() + to_copy, buffer);
            buffer[len] = '\0';
            return buffer;
        }

      private:
        std::string_view
        get_message(int code) const
        {
            static const std::string_view messages[] = { "Invalid code",
                                                         "Channel is null",
                                                         "Channel is closed" };

            auto ubound =
                static_cast< int >(std::extent_v< decltype(messages) >);
            if (code < 1 || code >= ubound)
                code = 0;

            return messages[code];
        }
    };
};

inline errors::channel_category const &
channel_category()
{
    constinit static const errors::channel_category cat;
    return cat;
};

inline error_code
make_error_code(errors::channel_errors code)
{
    return error_code(static_cast< int >(code), channel_category());
}

}   // namespace boost::channels

namespace boost::system {
template <>
struct is_error_code_enum< ::boost::channels::errors::channel_errors >
: std::true_type
{
};
}   // namespace boost::system
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_ERROR_CODE_HPP
