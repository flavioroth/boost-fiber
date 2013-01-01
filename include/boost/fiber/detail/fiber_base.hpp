
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_DETAIL_FIBER_BASE_H
#define BOOST_FIBERS_DETAIL_FIBER_BASE_H

#include <cstddef>
#include <iostream>
#include <vector>

#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/flags.hpp>
#include <boost/fiber/detail/spin_mutex.hpp>
#include <boost/fiber/detail/states.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {
namespace detail {

class BOOST_FIBERS_DECL fiber_base : private noncopyable
{
public:
    typedef intrusive_ptr< fiber_base >           ptr_t;

private:
    template< typename X, typename Y, typename Z >
    friend class fiber_object;

    atomic< std::size_t >   use_count_;
    atomic< state_t >       state_;
    atomic< int >           priority_;
    context::fcontext_t     caller_;
    context::fcontext_t *   callee_;
    int                     flags_;
    exception_ptr           except_;
    spin_mutex              mtx_;
    std::vector< ptr_t >    joining_;

protected:
    virtual void deallocate_object() = 0;
    virtual void unwind_stack() = 0;

public:
    class id
    {
    private:
        friend class fiber_base;

        fiber_base::ptr_t   impl_;

        explicit id( fiber_base::ptr_t const& impl) BOOST_NOEXCEPT :
            impl_( impl)
        {}

    public:
        id() BOOST_NOEXCEPT :
            impl_()
        {}

        bool operator==( id const& other) const BOOST_NOEXCEPT
        { return impl_ == other.impl_; }

        bool operator!=( id const& other) const BOOST_NOEXCEPT
        { return impl_ != other.impl_; }
        
        bool operator<( id const& other) const BOOST_NOEXCEPT
        { return impl_ < other.impl_; }
        
        bool operator>( id const& other) const BOOST_NOEXCEPT
        { return other.impl_ < impl_; }
        
        bool operator<=( id const& other) const BOOST_NOEXCEPT
        { return ! ( * this > other); }
        
        bool operator>=( id const& other) const BOOST_NOEXCEPT
        { return ! ( * this < other); }

        template< typename charT, class traitsT >
        friend std::basic_ostream< charT, traitsT > &
        operator<<( std::basic_ostream< charT, traitsT > & os, id const& other)
        {
            if ( 0 != other.impl_)
                return os << other.impl_;
            else
                return os << "{not-valid}";
        }

        operator bool() const BOOST_NOEXCEPT
        { return 0 != impl_; }

        bool operator!() const BOOST_NOEXCEPT
        { return 0 == impl_; }
    };

    fiber_base( context::fcontext_t *, bool);

    virtual ~fiber_base() {}

    id get_id() const BOOST_NOEXCEPT
    { return id( ptr_t( const_cast< fiber_base * >( this) ) ); }

    int priority() const BOOST_NOEXCEPT
    { return priority_.load(); }

    void priority( int prio) BOOST_NOEXCEPT
    { priority_.store( prio); }

    void resume();

    void suspend();

    void yield();

    void terminate();

    void join( ptr_t const&);

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool preserve_fpu() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_preserve_fpu); }

    bool is_terminated() const BOOST_NOEXCEPT
    { return state_terminated == state_.load(); }

    bool is_ready() const BOOST_NOEXCEPT
    { return state_ready == state_.load(); }

    bool is_running() const BOOST_NOEXCEPT
    { return state_running == state_.load(); }

    bool is_waiting() const BOOST_NOEXCEPT
    { return state_waiting == state_.load(); }

    bool set_terminated() BOOST_NOEXCEPT
    {
        state_t expected = state_running;
        return state_.compare_exchange_weak( expected, state_terminated, memory_order_release);
    }

    bool set_ready() BOOST_NOEXCEPT
    {
        state_t expected = state_waiting;
        bool result = state_.compare_exchange_weak( expected, state_ready, memory_order_release);
        if ( ! result && state_running == expected)
            result = state_.compare_exchange_weak( expected, state_ready, memory_order_release);
        return result;
    }

    bool set_running() BOOST_NOEXCEPT
    {
        state_t expected = state_ready;
        return state_.compare_exchange_weak( expected, state_running, memory_order_release);
    }

    bool set_waiting() BOOST_NOEXCEPT
    {
        state_t expected = state_running;
        return state_.compare_exchange_weak( expected, state_waiting, memory_order_release);
    }

    friend inline void intrusive_ptr_add_ref( fiber_base * p) BOOST_NOEXCEPT
    { p->use_count_.fetch_add( 1, memory_order_relaxed); }

    friend inline void intrusive_ptr_release( fiber_base * p)
    {
        if ( 1 == p->use_count_.fetch_sub( 1, memory_order_release) )
        {
            atomic_thread_fence( memory_order_acquire);
            p->deallocate_object();
        }
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_DETAIL_FIBER_BASE_H