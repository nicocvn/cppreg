//! Internals implementation.
/**
 * @file      Internals.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 *
 * This header collects various implementations which are required for cppreg
 * implementation but not intended to be fully exposed to the user.
 */


#ifndef CPPREG_INTERNALS_H
#define CPPREG_INTERNALS_H


#include "Traits.h"


namespace cppreg {
namespace internals {


//! Overflow check implementation.
/**
 * @tparam T Data type.
 * @tparam value Value to check.
 * @tparam limit Overflow limit value.
 *
 * This structure defines a type result set to std::true_type if there is
 * no overflow and set to std::false_type if there is overflow.
 * There is overflow if value if strictly larger than limit.
 */
template <typename T, T value, T limit>
struct check_overflow    // NOLINT
    : std::integral_constant<bool, value <= limit> {};


//! is_aligned implementation.
/**
 * @tparam address Address to be checked for alignment.
 * @tparam alignment Alignment boundary in bytes.
 *
 * This will only derived from std::true_type if the address is aligned.
 */
template <Address address, std::size_t alignment>
struct is_aligned    // NOLINT
    : std::integral_constant<
          bool,
          (static_cast<std::size_t>(address) & (alignment - 1)) == 0> {};


}    // namespace internals
}    // namespace cppreg


#endif    // CPPREG_INTERNALS_H
