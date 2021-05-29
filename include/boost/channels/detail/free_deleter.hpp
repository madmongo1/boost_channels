//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/boost_channels
//

#ifndef BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_FREE_DELETER_HPP
#define BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_FREE_DELETER_HPP

#include <cstdlib>

namespace boost::channels::detail {
struct free_deleter
{
    template<class T>
    void
    operator()(T *p) const noexcept
    {
        p->~T();
        std::free(p);
    }
};

}   // namespace boost::channels::detail
#endif   // BOOST_CHANNELS_INCLUDE_BOOST_CHANNELS_DETAIL_FREE_DELETER_HPP
