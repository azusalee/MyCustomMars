/*
(c) 2014 Glen Joseph Fernandes
<glenjofe -at- gmail.com>

Distributed under the Boost Software
License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef BOOST_ALIGN_DETAIL_ADDRESS_HPP
#define BOOST_ALIGN_DETAIL_ADDRESS_HPP

#include <boost/cstdint.hpp>
#include <cstddef>

namespace mars_boost_ksim {} namespace boost_ksim = mars_boost_ksim; namespace mars_boost_ksim {
namespace alignment {
namespace detail {

#if defined(BOOST_HAS_INTPTR_T)
typedef mars_boost_ksim::uintptr_t address;
#else
typedef std::size_t address;
#endif

} /* .detail */
} /* .alignment */
} /* .boost */

#endif
