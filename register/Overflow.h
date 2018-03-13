//! Overflow check implementation.
/**
 * @file      Overflow.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 */


#ifndef CPPREG_OVERFLOW_H
#define CPPREG_OVERFLOW_H


#include "Traits.h"
#include <type_traits>


//! cppreg::internals namespace.
namespace cppreg {
namespace internals {


    //! Overflow check implementation.
    /**
     * @tparam W Width of the register or field type.
     * @tparam value Value to check.
     * @tparam limit Overflow limit value.
     *
     * This structure defines a type result set to std::true_type if there is
     * no overflow and set to std::false_type if there is overflow.
     * There is overflow if value if strictly larger than limit.
     */
    template <
        Width_t W,
        typename RegisterType<W>::type value,
        typename RegisterType<W>::type limit
    >
    struct check_overflow : std::integral_constant<bool, value <= limit> {};


}
}


#endif  // CPPREG_OVERFLOW_H
