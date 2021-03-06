
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ASYNC_HPP
#define BOOST_FIBERS_ASYNC_HPP

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/packaged_task.hpp>

namespace boost {
namespace fibers {

template< typename Fn, typename ... Args >
future<
    typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type
>
async( Fn && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

template< typename StackAllocator, typename Fn, typename ... Args >
future<
    typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type
>
async( std::allocator_arg_t, StackAllocator salloc, Fn && fn, Args && ... args) {
    typedef typename std::result_of<
        typename std::decay< Fn >::type( typename std::decay< Args >::type ... )
    >::type     result_t;

    packaged_task< result_t( typename std::decay< Args >::type ... ) > pt{
        std::allocator_arg, salloc, std::forward< Fn >( fn) };
    future< result_t > f{ pt.get_future() };
    fiber{ std::move( pt), std::forward< Args >( args) ... }.detach();
    return f;
}

}}

#endif // BOOST_FIBERS_ASYNC_HPP
