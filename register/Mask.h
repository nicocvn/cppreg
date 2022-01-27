//! Bit mask implementation.
/**
 * @file      Mask.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 *
 * The implementation is designed to compute masks prior to runtime by
 * relying on constexpr function. This will work as intended if the function
 * argument is known at compile time, otherwise it will be evaluated at runtime.
 */


#ifndef CPPREG_MASK_H
#define CPPREG_MASK_H


#include "cppreg_Defines.h"


namespace cppreg {


//! Mask constexpr function.
/**
 * @tparam Mask Mask data type (will be derived from register).
 * @param width Mask width.
 * @return The mask value.
 */
template <typename Mask>
constexpr Mask make_mask(const FieldWidth width) noexcept {
    return width == 0U
               ? static_cast<Mask>(0U)
               : static_cast<Mask>(
                   static_cast<Mask>(make_mask<Mask>(FieldWidth(
                                         static_cast<FieldWidth>(width - 1U)))
                                     << 1U)
                   | 1U);
}


//! Shifted mask constexpr function.
/**
 * @tparam Mask Mask data type (will be derived from register).
 * @param width Mask width.
 * @param offset Mask offset.
 * @return The mask value.
 */
template <typename Mask>
constexpr Mask make_shifted_mask(const FieldWidth width,
                                 const FieldOffset offset) noexcept {
    return static_cast<Mask>(make_mask<Mask>(width) << offset);
}


}    // namespace cppreg


#endif    // CPPREG_MASK_H
