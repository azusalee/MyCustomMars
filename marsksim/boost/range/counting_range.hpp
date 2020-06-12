//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_COUNTING_RANGE_HPP_INCLUDED
#define BOOST_RANGE_COUNTING_RANGE_HPP_INCLUDED

#include <boost/config.hpp>
#if BOOST_MSVC >= 1400
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#include <boost/range/iterator_range_core.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>

namespace mars_boost_ksim {} namespace boost_ksim = mars_boost_ksim; namespace mars_boost_ksim
{
    template<class Value>
    inline iterator_range<counting_iterator<Value> >
    counting_range(Value first, Value last)
    {
        typedef counting_iterator<Value> counting_iterator_t;
        typedef iterator_range<counting_iterator_t> result_t;
        return result_t(counting_iterator_t(first),
                        counting_iterator_t(last));
    }

    template<class Range>
    inline iterator_range<
        counting_iterator<
            BOOST_DEDUCED_TYPENAME range_iterator<const Range>::type
        >
    >
    counting_range(const Range& rng)
    {
        typedef counting_iterator<
            BOOST_DEDUCED_TYPENAME range_iterator<const Range>::type
        > counting_iterator_t;

        typedef iterator_range<counting_iterator_t> result_t;

        return result_t(counting_iterator_t(mars_boost_ksim::begin(rng)),
                        counting_iterator_t(mars_boost_ksim::end(rng)));
    }

    template<class Range>
    inline iterator_range<
        counting_iterator<
            BOOST_DEDUCED_TYPENAME range_iterator<Range>::type
        >
    >
    counting_range(Range& rng)
    {
        typedef counting_iterator<
            BOOST_DEDUCED_TYPENAME range_iterator<Range>::type
        > counting_iterator_t;

        typedef iterator_range<counting_iterator_t> result_t;

        return result_t(counting_iterator_t(mars_boost_ksim::begin(rng)),
                        counting_iterator_t(mars_boost_ksim::end(rng)));
    }
} // namespace mars_boost_ksim {} namespace boost_ksim = mars_boost_ksim; namespace mars_boost_ksim

#if BOOST_MSVC >= 1400
#pragma warning(pop)
#endif

#endif // include guard
