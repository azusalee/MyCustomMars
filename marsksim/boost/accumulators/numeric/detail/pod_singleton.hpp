// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_DETAIL_POD_SINGLETON_DWA200655_HPP
# define BOOST_DETAIL_POD_SINGLETON_DWA200655_HPP

namespace mars_boost_ksim {} namespace boost_ksim = mars_boost_ksim; namespace mars_boost_ksim { namespace detail {

template<typename T>
struct pod_singleton
{
    static T instance;
};

template<typename T>
T pod_singleton<T>::instance;

}} // namespace mars_boost_ksim::detail

#endif // BOOST_DETAIL_POD_SINGLETON_DWA200655_HPP
